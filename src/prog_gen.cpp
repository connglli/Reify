// MIT License
//
// Copyright (c) 2025-2025
//
// Kavya Chopra (chopra.kavya04@gmail.com)
// Cong Li (cong.li@inf.ethz.ch)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "cxxopts.hpp"
#include "global.hpp"
#include "lib/chksum.hpp"
#include "lib/function.hpp"
#include "lib/random.hpp"
#include "lib/strutils.hpp"


namespace fs = std::filesystem;

////////////////////////////////////////////////////////////
////// IR Definition
////////////////////////////////////////////////////////////

class MyIR {
public:
  template<typename MyIRType>
  static std::unique_ptr<MyIRType> Create(const std::string &code) {
    auto node = std::make_unique<MyIRType>();
    node->Parse(code);
    return node;
  }

public:
  virtual ~MyIR() = default;

  // Generate the C code for this node
  [[nodiscard]] virtual std::string GenerateCode() const = 0;

  // Parse the given code to fill out this node
  virtual void Parse(const std::string &code) = 0;
};

class Expression : public MyIR {
public:
  // Get the number of coefficients within this expression
  virtual size_t GetNumCoeffs() const = 0;
  // Replace the first replaceable coefficent with the function call, return true for success
  virtual bool RepFirstCoeff(
      const std::string &funName, const std::vector<int> &initialisation,
      const std::vector<int> &finalization
  ) = 0;
};

// An Term is an expression in the form of c * v,
// where c is an coefficient and v a variable or 1.
class Term : public Expression {
public:
  size_t GetNumCoeffs() const override { return 1; }

  const std::string &GetCoeff() { return c; }

  const std::string &GetVar() { return v; }

  bool IsMutated() { return mutated; }

  void SetCoeff(const std::string &new_coeff) {
    c = new_coeff;
    mutated = true;
  }

  void Parse(const std::string &code) override {
    std::vector<std::string> tokens = SplitStr(code, "*");
    assert((tokens.size() == 1 || tokens.size() == 2) && "Invalid CMVExpr");
    c = StripStr(tokens[0]);
    v = tokens.size() > 1 ? StripStr(tokens[1]) : "1";
    mutated = false;
  }

  std::string GenerateCode() const override { return "(" + c + ") * " + v; };

  bool RepFirstCoeff(
      const std::string &funName, const std::vector<int> &initialisation,
      const std::vector<int> &finalization
  ) override {
    if (IsMutated()) {
      return false;
    }
    // Replace the coefficient with a call to the function
    int coeff_val = std::stoi(GetCoeff());
    int chk_val = StatelessChecksum::Compute(finalization);
    std::ostringstream chk_oss;
    chk_oss << "check_checksum(" << chk_val << ", " << funName << "(";
    for (int i = 0; i < static_cast<int>(initialisation.size()) - 1; ++i) {
      chk_oss << initialisation[i] << ", ";
    }
    chk_oss << initialisation[initialisation.size() - 1] << "))";
    // To avoid UBs, we'd use an upper type to save the result: long long here
    long long diff = static_cast<long long>(coeff_val) - static_cast<long long>(chk_val);
    if (diff >= static_cast<long long>(INT32_MIN) && diff <= static_cast<long long>(INT32_MAX)) {
      SetCoeff(chk_oss.str() + " + " + std::to_string(diff));
    } else {
      SetCoeff("(int) ((long long)" + chk_oss.str() + " + " + std::to_string(diff) + "L)");
    }
    return true;
  }

private:
  std::string c, v;
  bool mutated = false;
};

// A TermSum is the sum of a series of terms: s = t1 + ... + tn.
class TermSum : public Expression {
public:
  size_t GetNumCoeffs() const override {
    // Each term has exactly 1 coeff
    return terms.size();
  }

  std::string GenerateCode() const override {
    std::vector<std::string> tmp(terms.size());
    std::transform(terms.begin(), terms.end(), tmp.begin(), [](const auto &t) {
      return t->GenerateCode();
    });
    return JoinStr(tmp, " + ");
  }

  bool RepFirstCoeff(
      const std::string &funName, const std::vector<int> &initialisation,
      const std::vector<int> &finalization
  ) {
    for (auto &term: terms) {
      if (term->RepFirstCoeff(funName, initialisation, finalization)) {
        return true;
      }
    }
    return false;
  }

  void Parse(const std::string &code) override {
    // We append it with a "+" to facilitate parsing
    std::istringstream iss(code + "+");
    std::string tm;
    while (std::getline(iss, tm, '+')) {
      terms.push_back(Term::Create<Term>(tm));
    }
  }

private:
  std::vector<std::unique_ptr<Term>> terms;
};

class Statement : public MyIR {
public:
  enum Type { ASSIGN, IFGOTO, FLUFF };

public:
  // Return the type of the statement
  virtual Type GetStmtType() const = 0;
  // Return the number of coefficient within this statement
  virtual size_t GetNumCoeffs() const = 0;
  // Replace the first replaceable coefficent with the function call, return true for success
  virtual bool RepFirstCoeff(
      const std::string &funName, const std::vector<int> &initialisation,
      const std::vector<int> &finalization
  ) = 0;
};

// An AssignStmt is an assign of an expression to a variable: v = expr;.
class AssignStmt : public Statement {
public:
  void Parse(const std::string &code) override {
    auto equal = code.find('=');
    auto semicolon = code.find(';');
    lhsVar = code.substr(0, equal);
    StripStr(lhsVar);
    // TODO: Currently, we only support TermSum
    rhsExpr = TermSum::Create<TermSum>(code.substr(equal + 1, semicolon - equal - 1));
  }

public:
  Type GetStmtType() const override { return Type::ASSIGN; }

  size_t GetNumCoeffs() const override { return rhsExpr->GetNumCoeffs(); }

  bool RepFirstCoeff(
      const std::string &funName, const std::vector<int> &initialisation,
      const std::vector<int> &finalization
  ) override {
    return rhsExpr->RepFirstCoeff(funName, initialisation, finalization);
  }

  std::string GenerateCode() const override {
    return "    " + lhsVar + " = " + rhsExpr->GenerateCode() + ";";
  }

private:
  std::string lhsVar;
  std::unique_ptr<Expression> rhsExpr;
};

// An IfGotoStmt is an if-cond-goto statement: if (...) goto BB;
class IfGotoStmt : public Statement {
public:
  void Parse(const std::string &code) {
    auto ifKeyword = code.find("if");
    auto openParen = code.find('(', ifKeyword);
    auto gtThEq = code.find(">=", openParen);
    auto closeParen = code.find(')', gtThEq);
    auto semicolon = code.find(';', closeParen);
    // TODO: Currently, we only support TermSum
    condExpr = TermSum::Create<TermSum>(code.substr(openParen + 1, gtThEq - 1 - openParen - 1));
    gotoStmt = code.substr(closeParen + 1, semicolon - closeParen - 1);
  }

public:
  Type GetStmtType() const override { return Type::IFGOTO; }

  size_t GetNumCoeffs() const override { return condExpr->GetNumCoeffs(); }

  bool RepFirstCoeff(
      const std::string &funName, const std::vector<int> &initialisation,
      const std::vector<int> &finalization
  ) override {
    return condExpr->RepFirstCoeff(funName, initialisation, finalization);
  }

  std::string GenerateCode() const override {
    return "    if (" + condExpr->GenerateCode() + " >= 0) " + gotoStmt + ";";
  }

private:
  std::unique_ptr<Expression> condExpr;
  std::string gotoStmt;
};

// All other statements that are classified FluffStmt.
class FluffStmt : public Statement {
public:
  void Parse(const std::string &code) override { stmt = code; }

public:
  Type GetStmtType() const override { return Type::FLUFF; }

  size_t GetNumCoeffs() const override { return 0; }

  bool RepFirstCoeff(
      const std::string &funName, const std::vector<int> &initialisation,
      const std::vector<int> &finalization
  ) override {
    return false;
  }

  std::string GenerateCode() const override { return stmt; }

private:
  std::string stmt;
};

// This represents a Function that are generated by fgen
class Function : public MyIR {
public:
  static std::unique_ptr<Function> Of(const fs::path &funPath, const fs::path &mapPath) {
    std::string tmpLine;

    std::ostringstream foss;
    std::ifstream fifs(funPath);
    if (!fifs.is_open()) {
      std::cerr << "Error: failed to open file: " << funPath << std::endl;
      exit(1);
    }
    while (std::getline(fifs, tmpLine)) {
      foss << tmpLine << std::endl;
    }
    fifs.close();

    std::ostringstream moss;
    std::ifstream mifs(mapPath);
    if (!mifs.is_open()) {
      std::cerr << "Error failed to open file: " << mapPath << std::endl;
      exit(1);
    }
    while (std::getline(mifs, tmpLine)) {
      moss << tmpLine << std::endl;
    }
    mifs.close();

    auto fun = Function::Create<Function>(foss.str());
    fun->ParseMapping(moss.str());

    return fun;
  }

  const std::string &GetName() { return name; }

  int GetNumRepCoeffs() const { return numCoeffs * GlobalOptions::Get().ReplaceProba; }

  void ExtractMapping(
      std::vector<int> &foreign_initialisation, std::vector<int> &foreign_finalization
  ) const {
    // sample a random index from initialisations list
    int index = Random::Get().Uniform(0, (int) initialisations.size() - 1)();
    foreign_initialisation = initialisations[index];
    foreign_finalization = finalizations[index];
  }

  void ParseMapping(const std::string &mapping) {
    std::istringstream iss(mapping);
    std::string line;
    while (std::getline(iss, line)) {
      std::vector<std::string> iniFin = SplitStr(line, " : ", true);
      assert(iniFin.size() == 2 && "invalid mapping line");

      std::vector<std::string> ini = SplitStr(iniFin[0], ",", true);
      std::vector<std::string> fin = SplitStr(iniFin[1], ",", true);
      assert(
          ini.size() == fin.size() && "the size of initialisation and finalization is different"
      );

      initialisations.push_back(std::vector<int>(ini.size()));
      std::transform(
          ini.begin(), ini.end(), initialisations.back().begin(),
          [](const auto &s) -> int { return std::stoi(s); }
      );
      finalizations.push_back(std::vector<int>(fin.size()));
      std::transform(
          fin.begin(), fin.end(), finalizations.back().begin(),
          [](const auto &s) -> int { return std::stoi(s); }
      );
    }
  }

  void AddMapping(const std::vector<int> &initialisation, const std::vector<int> &finalization) {
    initialisations.push_back(initialisation);
    finalizations.push_back(finalization);
  }

  bool RepFirstCoeff(
      const std::string &funName, const std::vector<int> &initialisation,
      const std::vector<int> &finalization
  ) {
    auto rand = Random::Get().Uniform(0, (int) statements.size() - 1);
    auto probability = Random::Get().UniformReal()();
    int trials = 1000;
    // Sample a statement from the list of statements for replacement
    while (trials > 0) {
      int index = rand();
      // Idea: 80% of the replacements should be in the IF statements, and 20% in assignments
      if (statements[index]->GetStmtType() == Statement::Type::IFGOTO && probability < 0.8) {
        if (statements[index]->RepFirstCoeff(funName, initialisation, finalization)) {
          return true;
        }
      } else if (statements[index]->GetStmtType() == Statement::Type::ASSIGN && probability > 0.8) {
        if (statements[index]->RepFirstCoeff(funName, initialisation, finalization)) {
          return true;
        }
      }
      trials--;
    }
    return false;
  }

  std::string GenerateCode() const override {
    std::ostringstream oss;
    for (const auto &statement: statements) {
      oss << statement->GenerateCode() << std::endl;
    }
    return oss.str();
  }

  void Parse(const std::string &code) override {
    std::istringstream iss(code);
    std::string line;
    // Parse the function line by line
    while (std::getline(iss, line)) {
      if (line.find("pass_counter") != std::string::npos) {
        // Our replacement respect pass_counter
        statements.push_back(FluffStmt::Create<FluffStmt>(line));
      } else if (line.find("if") != std::string::npos) {
        statements.push_back(IfGotoStmt::Create<IfGotoStmt>(line));
        numCoeffs += statements.back()->GetNumCoeffs();
      } else if (line.find("int function") != std::string::npos) {
        // Start of the function
        auto nameStart = line.find("function");
        auto paramStart = line.find("(");
        auto paramEnd = line.find(")");
        name = line.substr(nameStart, paramStart - nameStart);
        std::string params = line.substr(paramStart + 1, paramEnd - paramStart);
        numParams = std::count(params.begin(), params.end(), ',') + 1;
        statements.push_back(FluffStmt::Create<FluffStmt>(line));
      } else if (line.find("return") != std::string::npos) {
        statements.push_back(FluffStmt::Create<FluffStmt>(line));
      } else if (line.find("BB") != std::string::npos) {
        // Start a new basic block
        statements.push_back(FluffStmt::Create<FluffStmt>(line));
      } else if (line.find("=") != std::string::npos) {
        // Add statement to the current basic block
        statements.push_back(AssignStmt::Create<AssignStmt>(line));
        numCoeffs += statements.back()->GetNumCoeffs();
      } else {
        // Add it as a fluff statement
        statements.push_back(FluffStmt::Create<FluffStmt>(line));
      }
    }
  }

private:
  std::string name;
  std::vector<std::unique_ptr<Statement>> statements;

  int numParams; // The number of function parameters/arguments
  int numCoeffs; // The number of coefficients in this function

  std::vector<std::vector<int>> initialisations;
  std::vector<std::vector<int>> finalizations;
};

class Program {
public:
  Program(const std::string &uuid, int sno, const std::vector<std::string> &funPaths) :
      uuid(uuid), sno(std::to_string(sno)) {
    // Parse all selected function files
    for (const auto &funPath: funPaths) {
      fs::path mapPath = GetMappingPathForFunctionPath(funPath);
      functions.push_back(Function::Of(funPath, mapPath));
    }
  }

  std::string GetName() { return uuid + "_" + sno; }

  void Generate() {
    int numFuns = static_cast<int>(functions.size());
    assert(
        numFuns == GlobalOptions::Get().FunctionDepth &&
        "The number of selected functions is not FUNCTION_DEPTH"
    );

    std::vector<int> initialisation;
    std::vector<int> finalization;

    // Now replace the mappings in the functions with the calls to the other functions
    for (int i = 0; i < numFuns - 1; ++i) {
      auto host = functions[i].get();
      int numRepCoeffs = host->GetNumRepCoeffs();

      std::cout << "[" << sno << "] Replacing function" << ": index=" << i
                << ", name=" << host->GetName() << ", num_replaceable=" << numRepCoeffs
                << std::endl;

      // Sample a function from i + 1 to the end
      auto rand = Random::Get().Uniform(i + 1, numFuns - 1);

      // For each coefficient that can be replaced, we find a function to replace it
      for (int k = 0; k < numRepCoeffs; ++k) {
        int j = rand();
        auto guest = functions[j].get();
        guest->ExtractMapping(initialisation, finalization);
        if (!host->RepFirstCoeff(guest->GetName(), initialisation, finalization)) {
          break; // TODO: break or continue? We can continue to the next, can't we?
        }
        std::cout << "[" << sno << "]   var#" << k << " -> func#" << j << ": " << guest->GetName()
                  << std::endl;
      }

      std::cout << "[" << sno << "]   Done" << std::endl;
    }
  }

  void GenerateCode(const fs::path &file, bool debug = false, bool staticModifier = false) {
    std::ofstream oss(file);
    oss << StatelessChecksum::GetRawCode() << std::endl;
    oss << "#include <stdio.h>" << std::endl;
    if (debug) {
      oss << "#include <assert.h>" << std::endl << std::endl;
      oss << "#define check_checksum(expected, actual) (assert((expected)==(actual) && \"Checksum "
             "not equal\"), (actual))"
          << std::endl
          << std::endl;
    } else {
      oss << "#define check_checksum(expected, actual) (actual)" << std::endl << std::endl;
    }
    auto rand = Random::Get().UniformReal();
    for (int i = static_cast<int>(functions.size()) - 1; i >= 0; --i) {
      // Output inline with 60% probability
      auto probability = rand();
      if (staticModifier) {
        oss << "static ";
      }
      if (probability < 0.6) {
        if (!staticModifier) {
          oss << "static inline ";
        } else {
          oss << "inline ";
        }
      }
      oss << functions[i]->GenerateCode() << std::endl;
    }
    oss << "int main() {" << std::endl;
    std::vector<int> initialisation;
    std::vector<int> finalization;
    functions[0]->ExtractMapping(initialisation, finalization);
    int checksum = StatelessChecksum::Compute(finalization);
    oss << "    printf(\"%d,\", check_checksum(" << checksum << ", " << functions[0]->GetName()
        << "(";
    for (int i = 0; i < initialisation.size(); ++i) {
      oss << initialisation[i];
      if (i != initialisation.size() - 1) {
        oss << ", ";
      }
    }
    oss << ")));" << std::endl;
    oss << "    return 0;" << std::endl;
    oss << "}" << std::endl;
    oss.close();
  }

private:
  std::string uuid, sno;
  std::vector<std::unique_ptr<Function>> functions;
};

struct ProgGenOpts {
  std::string uuid;
  std::string input;
  int limits;
  bool debug;

  static ProgGenOpts Parse(int argc, char **argv) {
    cxxopts::Options options("pgen");
    // clang-format off
    options.add_options()
      ("uuid", "An UUID identifier", cxxopts::value<std::string>())
      ("i,input", "The directory saving the seed functions and mappings", cxxopts::value<std::string>())
      ("l,limit", "The number of new programs to generate (0 for unlimited generation)", cxxopts::value<int>()->default_value("0"))
      ("s,seed", "The seed for random sampling (negative values for truly random)", cxxopts::value<int>()->default_value("-1"))
      ("debug", "Enable debugging mode which add checksum check assertions", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("h,help", "Print help message", cxxopts::value<bool>()->default_value("false")->implicit_value("true"));
    options.parse_positional("uuid");
    options.positional_help("UUID");
    // clang-format on

    cxxopts::ParseResult args;
    try {
      args = options.parse(argc, argv);
    } catch (cxxopts::exceptions::exception &e) {
      std::cerr << "Error: " << e.what() << std::endl;
      exit(1);
    }

    if (args.count("help")) {
      std::cout << options.help() << std::endl;
      exit(0);
    }

    std::string uuid;
    if (!args.count("uuid")) {
      std::cerr << "Error: The UUID identifier (UUID) is not given." << std::endl;
      exit(1);
    } else {
      uuid = args["uuid"].as<std::string>();
      // Replace uuid's non-alphanumeric characters with underscore
      // used a UUID so that i could throw everything from different runs
      // into the same directory without worrying about name clashes
      std::replace_if(
          uuid.begin(), uuid.end(), [](auto c) -> bool { return !std::isalnum(c); }, '_'
      );
    }

    std::string input;
    if (!args.count("input")) {
      std::cerr << "Error: The input directory (--input) is not given." << std::endl;
      exit(1);
    } else {
      input = args["input"].as<std::string>();
      if (!fs::exists(input)) {
        std::cerr << "Error: The input directory (--input) does not exist." << std::endl;
        exit(1);
      }
    }

    int limit = 0;
    if (args.count("limit")) {
      limit = args["limit"].as<int>();
      if (limit < 0) {
        std::cerr << "Error: The limit (--limit) must be greater than or equal to 0." << std::endl;
        exit(1);
      }
    }

    if (args.count("seed")) {
      int seed = args["seed"].as<int>();
      if (seed >= 0) {
        Random::Get().Seed(seed);
      }
    }

    bool debug = false;
    if (args.count("debug")) {
      debug = true;
    }

    return {.uuid = uuid, .input = input, .limits = limit, .debug = debug};
  }
};

int main(int argc, char *argv[]) {
  auto cliOpts = ProgGenOpts::Parse(argc, argv);

  std::string progUuid = cliOpts.uuid;

  fs::path inputDir = cliOpts.input;
  fs::path funsDir(GetFunctionsDir(inputDir));

  int genLimit = cliOpts.limits;
  bool enableDebug = cliOpts.debug;

  // Read all function files from the input directory
  std::vector<std::string> allFunPaths;
  // Open the directory and read all files
  for (const auto &entry: fs::directory_iterator(funsDir)) {
    if (entry.is_regular_file()) {
      allFunPaths.push_back(entry.path());
    }
  }
  // Without a sorting, our results may not be deterministic as
  // the order of directory_iterator is not decidable.
  std::sort(allFunPaths.begin(), allFunPaths.end());

  for (int sampNo = 0; genLimit == 0 || sampNo < genLimit; ++sampNo) {
    std::cout << "[" << sampNo << "] Generating ... " << std::endl;

    // Randomly select FUNCTION_DEPTH functions for the new program
    std::set<int> selFunInds;
    std::vector<std::string> selFunPaths;

    int numAll = static_cast<int>(allFunPaths.size()), numSel = GlobalOptions::Get().FunctionDepth;
    auto rand = Random::Get().Uniform(0, static_cast<int>(numAll - 1));

    while (selFunInds.size() < static_cast<size_t>(numSel)) {
      int index = rand();
      if (selFunInds.contains(index)) {
        continue;
      }
      std::cout << "[" << sampNo << "] Selecting func#" << index << ": " << allFunPaths[index]
                << std::endl;
      selFunInds.insert(index);
      selFunPaths.push_back(allFunPaths[index]);
    }

    // Now we construct our new program
    auto prog = std::make_unique<Program>(progUuid, sampNo, selFunPaths);
    prog->Generate();

    std::cout << "[" << sampNo << "] Storing" << std::endl;

    prog->GenerateCode(GetProgramsDir(inputDir) / (prog->GetName() + ".c"), enableDebug);
    prog->GenerateCode(
        GetProgramsDir(inputDir) / (prog->GetName() + "_static.c"), enableDebug, true
    );
  }
}
