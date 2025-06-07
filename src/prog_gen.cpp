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
#include "lib/random.hpp"
#include "lib/strutils.hpp"

struct CVMTerm {
  std::string coeff;
  std::string var;
  bool mutated = false;

  std::string GenerateCode() const { return "(" + coeff + ") * " + var; };
};

class Statement {

public:
  enum class Type { ASSIGNMENT, IF, FLUFF };

public:
  virtual std::string GenerateCode() const = 0;
  virtual Type GetType() const = 0;
  virtual bool RepFirstCoeff(std::string funcName, std::vector<int> initialisation, std::vector<int> finalization) = 0;

  virtual int GetNumCoeffs() const { return 0; };
};

class FluffStmt : public Statement {
public:
  FluffStmt(const std::string &stmt) : stmt(stmt) {}

  std::string GenerateCode() const override { return stmt; }

  Type GetType() const override { return Type::FLUFF; }

  bool RepFirstCoeff(std::string funcName, std::vector<int> initialisation, std::vector<int> finalization) override {
    return false;
  }

private:
  std::string stmt;
};

class AssignStmt : public Statement {
  std::string lhsVar;
  std::vector<CVMTerm> rhsTerms; // coefficient_value, is_mutated, variable

public:
  AssignStmt(const std::string &statement) {
    size_t equalPos = statement.find('=');
    lhsVar = statement.substr(0, equalPos);
    std::string rhs = statement.substr(equalPos + 1);
    std::stringstream ss(rhs);
    std::string token;
    while (std::getline(ss, token, '+') || std::getline(ss, token, ';')) {
      size_t mulPos = token.find('*');
      if (mulPos != std::string::npos) {
        std::string coeff = token.substr(0, mulPos);
        std::string var = token.substr(mulPos + 1);
        rhsTerms.push_back({.coeff = coeff, .var = var, .mutated = false});
      } else {
        mulPos = token.find(';');
        if (mulPos != std::string::npos) {
          token = token.substr(0, mulPos);
        }
        rhsTerms.push_back({.coeff = token, .var = "1", .mutated = false});
      }
    }
  }

  std::string GenerateCode() const override {
    std::vector<std::string> terms(rhsTerms.size());
    std::transform(rhsTerms.begin(), rhsTerms.end(), terms.begin(), [](auto &c) { return c.GenerateCode(); });
    return lhsVar + " = " + JoinStr(terms, " + ") + ";";
  }

  Type GetType() const override { return Type::ASSIGNMENT; }

  int GetNumCoeffs() const override { return rhsTerms.size(); }

  bool RepFirstCoeff(std::string funcName, std::vector<int> initialisation, std::vector<int> finalization) {
    for (auto &term: rhsTerms) {
      if (!term.mutated) {
        // replace the coefficient with a call to the function
        int coefficient = std::stoi(term.coeff);
        int checksum = StatelessChecksum::Compute(finalization);
        std::string replacement = "check_checksum(" + std::to_string(checksum) + ", " + funcName + "(";
        for (int i = 0; i < initialisation.size() - 1; ++i) {
          replacement += std::to_string(initialisation[i]) + ", ";
        }
        replacement += std::to_string(initialisation[initialisation.size() - 1]);
        replacement += "))";
        long long diff = (long long) coefficient - (long long) checksum; // To avoid UBs
        if (diff >= (long long) INT32_MIN && diff <= (long long) INT32_MAX) {
          term.coeff = replacement + " + " + std::to_string(diff);
        } else {
          term.coeff = "(int) ((long long)" + replacement + " + " + std::to_string(diff) + "L)";
        }
        term.mutated = true;
        return true;
      }
    }
    return false;
  }
};

class IfStmt : public Statement {
  std::vector<CVMTerm> condTerms;
  std::string gotoStmt;

public:
  IfStmt(std::string statement) {
    size_t ifPos = statement.find("if");
    size_t openParen = statement.find('(', ifPos);
    size_t closeParen = statement.find(')', openParen);
    std::string condition_str = statement.substr(openParen + 1, closeParen - openParen - 1);
    std::stringstream ss(condition_str);
    std::string token;
    while (std::getline(ss, token, '+') || std::getline(ss, token, ';')) {
      size_t mulPos = token.find('*');
      if (mulPos != std::string::npos) {
        condTerms.push_back({token.substr(0, mulPos), token.substr(mulPos + 1), false});
      } else {
        mulPos = token.find(">=");
        if (mulPos != std::string::npos) {
          token = token.substr(0, mulPos);
        }
        condTerms.push_back({token, "1", false}); // Default coefficient of 1
      }
    }
    gotoStmt = statement.substr(closeParen + 1);
  }

  int GetNumCoeffs() const { return condTerms.size(); }

  bool RepFirstCoeff(std::string funcName, std::vector<int> initialisation, std::vector<int> finalization) {
    for (auto &term: condTerms) {
      if (!term.mutated && term.coeff != "pass_counter ") {
        // replace the coefficient with a call to the function
        int coefficient = std::stoi(term.coeff);
        int checksum = StatelessChecksum::Compute(finalization);
        std::string replacement = "check_checksum(" + std::to_string(checksum) + ", " + funcName + "(";
        for (int i = 0; i < initialisation.size() - 1; ++i) {
          replacement += std::to_string(initialisation[i]) + ", ";
        }
        replacement += std::to_string(initialisation[initialisation.size() - 1]);
        replacement += "))";
        long long diff = (long long) coefficient - (long long) checksum;
        if (diff >= (long long) INT32_MIN && diff <= (long long) INT32_MAX) {
          term.coeff = replacement + " + " + std::to_string(diff);
        } else {
          assert((int) ((long long) checksum + diff) == coefficient && "Not same");
          term.coeff = "(int) ((long long)" + replacement + " + " + std::to_string(diff) + "L)";
        }
        term.mutated = true;
        return true;
      }
    }
    return false;
  }

  std::string GenerateCode() const override {
    std::vector<std::string> terms(condTerms.size());
    std::transform(condTerms.begin(), condTerms.end(), terms.begin(), [](auto &c) { return c.GenerateCode(); });
    return "if (" + JoinStr(terms, " + ") + " >= 0) " + gotoStmt;
  }

  Type GetType() const override { return Type::IF; }
};

class Function {

public:
  explicit Function() {}

  Function(std::ifstream &funcFile, std::ifstream &mapFile) {
    std::string line;
    // Parse the func
    while (std::getline(funcFile, line)) {
      if (line.find("pass_counter") != std::string::npos) {
        statements.push_back(std::make_unique<FluffStmt>(line));
      } else if (line.find("if") != std::string::npos) {
        statements.push_back(std::make_unique<IfStmt>(line));
        numCoeffs += statements.back()->GetNumCoeffs();
      } else if (line.find("int") != std::string::npos) {
        name = line.substr(line.find("int") + 4, line.find('(') - line.find("int") - 4);
        size_t start = line.find('(') + 1;
        size_t end = line.find(')');
        std::string params = line.substr(start, end - start);
        numInputVars = std::count(params.begin(), params.end(), ',') + 1;
        statements.push_back(std::make_unique<FluffStmt>(line));
      } else if (line.find("return") != std::string::npos) {
        // End of the func
        statements.push_back(std::make_unique<FluffStmt>(line));
        // statements.push_back(std::make_unique<FluffStatement>(line));
      } else if (line.find("BB") != std::string::npos) {
        // Start a new basic block
        statements.push_back(std::make_unique<FluffStmt>(line));

      } else if (line.find("=") != std::string::npos) {
        // Add statement to the current basic block
        statements.push_back(std::make_unique<AssignStmt>(line));
        numCoeffs += statements.back()->GetNumCoeffs();
      } else {
        // Add fluff statement
        statements.push_back(std::make_unique<FluffStmt>(line));
      }
    }
    // Parse its mapping
    while (std::getline(mapFile, line)) {
      std::vector<std::string> iniFin = SplitStr(line, " : ", true);
      assert(iniFin.size() == 2 && "invalid mapping line");

      std::vector<std::string> ini = SplitStr(iniFin[0], ",", true);
      std::vector<std::string> fin = SplitStr(iniFin[1], ",", true);
      assert(ini.size() == fin.size() && "the size of initialisation and finalization is different");

      initialisations.push_back(std::vector<int>(ini.size()));
      std::transform(ini.begin(), ini.end(), initialisations.back().begin(), [](const auto &s) -> int {
        return std::stoi(s);
      });
      finalizations.push_back(std::vector<int>(fin.size()));
      std::transform(fin.begin(), fin.end(), finalizations.back().begin(), [](const auto &s) -> int {
        return std::stoi(s);
      });
    }
  }

  std::string getName() const { return name; }

  bool RepFirstCoeff(std::string funcName, std::vector<int> initialisation, std::vector<int> finalization) {
    auto rand = Random::Get().Uniform(0, (int) statements.size() - 1);
    auto probability = Random::Get().UniformReal()();
    int trials = 1000;
    // Sample a statement from the list of statements for replacement
    while (trials > 0) {
      int index = rand();
      // Idea: 80% of the replacements should be in the IF statements, and 20%  in the assignment statements
      if (statements[index]->GetType() == Statement::Type::IF && probability < 0.8) {
        if (statements[index]->RepFirstCoeff(funcName, initialisation, finalization)) {
          return true;
        }
      } else if (statements[index]->GetType() == Statement::Type::ASSIGNMENT && probability > 0.8) {
        if (statements[index]->RepFirstCoeff(funcName, initialisation, finalization)) {
          return true;
        }
      }
      trials--;
    }
    return false;
  }

  std::string generateCode() const {
    std::string code;
    for (const auto &statement: statements) {
      code += statement->GenerateCode() + "\n";
    }
    return code;
  }

  void getMapping(std::vector<int> &foreign_initialisation, std::vector<int> &foreign_finalization) const {
    // sample a random index from initialisations list
    int index = Random::Get().Uniform(0, (int) initialisations.size() - 1)();
    foreign_initialisation = initialisations[index];
    foreign_finalization = finalizations[index];
  }

  int calculateNumCoefficientsToReplace() const { return numCoeffs * REPLACEMENT_PROBABILITY; }

private:
  std::string name;
  std::vector<std::unique_ptr<Statement>> statements;

  int numInputVars;
  int numCoeffs;
  std::unordered_map<std::string, int> parameters;

  std::vector<std::vector<int>> initialisations;
  std::vector<std::vector<int>> finalizations;
};

class Program {

public:
  static void GenerateCode(
      const std::filesystem::path fileName, const std::vector<Function> &functions, bool debug = false,
      bool staticModifier = false
  ) {
    std::ofstream outFile(fileName);
    outFile << StatelessChecksum::GetRawCode() << std::endl;
    outFile << "#include <stdio.h>" << std::endl;
    if (debug) {
      outFile << "#include <assert.h>" << std::endl << std::endl;
      outFile << "#define check_checksum(expected, actual) (assert((expected)==(actual) && \"Checksum not "
                 "equal\"), (actual))"
              << std::endl
              << std::endl;
    } else {
      outFile << "#define check_checksum(expected, actual) (actual)" << std::endl << std::endl;
    }
    auto rand = Random::Get().UniformReal();
    for (int i = FUNCTION_DEPTH - 1; i >= 0; --i) {
      // output inline with 60% probability
      auto probability = rand();
      if (staticModifier) {
        outFile << "static ";
      }
      if (probability < 0.6) {
        if (!staticModifier) {
          outFile << "static inline ";
        } else {
          outFile << "inline ";
        }
      }
      outFile << functions[i].generateCode() << std::endl;
    }
    outFile << "int main() {" << std::endl;
    std::vector<int> initialisation;
    std::vector<int> finalization;
    functions[0].getMapping(initialisation, finalization);
    int checksum = StatelessChecksum::Compute(finalization);
    outFile << "    printf(\"%d,\", check_checksum(" << checksum << ", " << functions[0].getName() << "(";
    for (int i = 0; i < initialisation.size(); ++i) {
      outFile << initialisation[i];
      if (i != initialisation.size() - 1) {
        outFile << ", ";
      }
    }
    outFile << ")));" << std::endl;
    outFile << "    return 0;" << std::endl;
    outFile << "}" << std::endl;
    outFile.close();
    // std::cout << "Code generated successfully in " << filename << std::endl;
  }
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
      std::replace_if(uuid.begin(), uuid.end(), [](auto c) -> bool { return !std::isalnum(c); }, '_');
    }

    std::string input;
    if (!args.count("input")) {
      std::cerr << "Error: The input directory (--input) is not given." << std::endl;
      exit(1);
    } else {
      input = args["input"].as<std::string>();
      if (!std::filesystem::exists(input)) {
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

  std::filesystem::path inputDir = cliOpts.input;
  std::filesystem::path funcsDir(GetFunctionsDir(inputDir));

  int genLimit = cliOpts.limits;
  bool enableDebug = cliOpts.debug;

  // Read all files from funcsDir
  std::vector<std::filesystem::path> allFuncPaths;
  // Open the directory and read all files
  for (const auto &entry: std::filesystem::directory_iterator(funcsDir)) {
    if (entry.is_regular_file()) {
      allFuncPaths.push_back(entry.path());
    }
  }

  // Save the functions that we select for generating a program
  std::vector<std::filesystem::path *> selFuncPaths;
  selFuncPaths.resize(FUNCTION_DEPTH);
  std::vector<Function> selFuncs;
  selFuncs.resize(FUNCTION_DEPTH);

  for (int sampNo = 0; genLimit == 0 || sampNo < genLimit; ++sampNo) {
    std::string progName = progUuid + "_" + std::to_string(sampNo);

    std::cout << "[" << sampNo << "] Generating " << progName << "... " << std::endl;

    // Choose the function files randomly
    // Fixed Unsafe memset(vector<string>, 0, sizeof(string)!!
    // std::string saves its data in the heap. That said, clears the vector's internal data
    // does not lead to the auto deletion of the string's internal data. Small string optimization does not
    // work here as the path of each our function is indeed more than 25 characters.
    memset(selFuncPaths.data(), 0, selFuncPaths.size() * sizeof(std::filesystem::path *));
    auto rand = Random::Get().Uniform(0, (int) allFuncPaths.size() - 1);
    for (int i = 0; i < FUNCTION_DEPTH; ++i) {
      while (true) {
        int index = rand();
        if (std::find(selFuncPaths.begin(), selFuncPaths.end(), &allFuncPaths[index]) == selFuncPaths.end()) {
          selFuncPaths[i] = &allFuncPaths[index];
          break;
        }
      }
    }

    // Parse all selected function files
    for (int i = 0; i < FUNCTION_DEPTH; ++i) {
      std::filesystem::path funcFile = *selFuncPaths[i];
      std::filesystem::path mapFile = GetMappingPathForFunctionPath(funcFile);
      std::cout << "[" << sampNo << "] Opening func#" << i << "'s function: " << funcFile << std::endl;
      std::ifstream funcIfs(funcFile);
      std::cout << "[" << sampNo << "] Opening func#" << i << "'s mapping: " << mapFile << std::endl;
      std::ifstream mapIfs(mapFile);
      if (!funcIfs.is_open() || !mapIfs.is_open()) {
        std::cerr << "Error opening file: " << funcFile.stem() << std::endl;
        exit(1);
      }
      selFuncs[i] = Function(funcIfs, mapIfs);
      funcIfs.close();
      mapIfs.close();
    }

    std::vector<int> initialisation;
    std::vector<int> finalization;
    // Now replace the mappings in the functions with the calls to the other functions
    for (int i = 0; i < FUNCTION_DEPTH - 1; ++i) {
      // Sample a function from i + 1 to PROCEDURE_DEPTH
      auto thisRand = Random::Get().Uniform(i + 1, FUNCTION_DEPTH - 1);

      // Calculate all replaceable coefficients
      int num_mappings_to_replace = selFuncs[i].calculateNumCoefficientsToReplace();

      std::cout << "[" << sampNo << "] Replacing function"
                << ": index=" << i << ", name=" << selFuncPaths[i]->stem()
                << ", num_replaceable=" << num_mappings_to_replace << std::endl;

      // For each coefficient that can be replaced, we find a function to replace it
      for (int k = 0; k < num_mappings_to_replace; ++k) {
        int j = thisRand();
        selFuncs[j].getMapping(initialisation, finalization);
        if (!selFuncs[i].RepFirstCoeff(selFuncs[j].getName(), initialisation, finalization)) {
          break; // TODO: break or continue? We can continue to the next, can't we?
        }
        std::cout << "[" << sampNo << "]   var#" << k << " -> func#" << j << ": " << selFuncPaths[j]->stem()
                  << std::endl;
      }

      std::cout << "[" << sampNo << "]   Done" << std::endl;
    }

    std::cout << "[" << sampNo << "] Storing" << std::endl;
    Program::GenerateCode(GetProgramsDir(inputDir) / (progName + ".c"), selFuncs, enableDebug);
    Program::GenerateCode(GetProgramsDir(inputDir) / (progName + "_static.c"), selFuncs, enableDebug, true);
  }
}
