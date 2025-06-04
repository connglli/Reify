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
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "cxxopts.hpp"
#include "global.hpp"
#include "lib/strutils.hpp"

/////////////////////////////////////////////////
///////// Checksum
/////////////////////////////////////////////////
///
const int getWithinRange = INT32_MAX;
std::vector<uint32_t> crc32_table(256);

void generate_crc32_table(std::vector<uint32_t> &crc32_table) {
  const uint32_t poly = 0xEDB88320UL;
  for (uint32_t i = 0; i < 256; i++) {
    uint32_t crc = i;
    for (int j = 8; j > 0; j--) {
      if (crc & 1) {
        crc = (crc >> 1) ^ poly;
      } else {
        crc >>= 1;
      }
    }
    crc32_table[i] = crc;
  }
}

int32_t uint32_to_int32(uint32_t u) { return u % getWithinRange; }

static uint32_t context_free_crc32_byte(uint32_t context, uint8_t b) {
  return ((context >> 8) & 0x00FFFFFF) ^ crc32_table[(context ^ b) & 0xFF];
}

static uint32_t context_free_crc32_4bytes(uint32_t context, uint32_t val) {
  context = context_free_crc32_byte(context, (val >> 0) & 0xFF);
  context = context_free_crc32_byte(context, (val >> 8) & 0xFF);
  context = context_free_crc32_byte(context, (val >> 16) & 0xFF);
  context = context_free_crc32_byte(context, (val >> 24) & 0xFF);
  return context;
}

std::string generateCodeForChecksumFunction(int checksumType = CHECKSUM_TYPE) {
  std::stringstream code;
  // a C-style va-list function to take a variable number of arguments
  switch (checksumType) {
    case 0:
      code << "#include <string.h>\n";
      code << "uint32_t crc32_tab[256];\n";
      code << "int32_t uint32_to_int32(uint32_t u){\n";
      code << "    return u%" << getWithinRange << ";\n";
      code << "}\n";
      code << "void generate_crc32_table(uint32_t* crc32_table) {\n";
      code << "    const uint32_t poly = 0xEDB88320UL;\n";
      code << "    for (uint32_t i = 0; i < 256; i++) {\n";
      code << "        uint32_t crc = i;\n";
      code << "        for (int j = 8; j > 0; j--) {\n";
      code << "            if (crc & 1) {\n";
      code << "                crc = (crc >> 1) ^ poly;\n";
      code << "            } else {\n";
      code << "                crc >>= 1;\n";
      code << "            }\n";
      code << "        }\n";
      code << "        crc32_table[i] = crc;\n";
      code << "    }\n";
      code << "}\n";
      code << "uint32_t context_free_crc32_byte(uint32_t context, uint8_t b) {\n";
      code << "    return ((context >> 8) & 0x00FFFFFF) ^ crc32_tab[(context ^ "
              "b) & 0xFF];\n";
      code << "}\n";
      code << "uint32_t context_free_crc32_4bytes(uint32_t context, uint32_t "
              "val) {\n";
      code << "    context = context_free_crc32_byte(context, (val >> 0) & "
              "0xFF);\n";
      code << "    context = context_free_crc32_byte(context, (val >> 8) & "
              "0xFF);\n";
      code << "    context = context_free_crc32_byte(context, (val >> 16) & "
              "0xFF);\n";
      code << "    context = context_free_crc32_byte(context, (val >> 24) & "
              "0xFF);\n";
      code << "    return context;\n";
      code << "}\n";
      break;
    default:
      assert(false && "Invalid checksum type");
  }
  code << "int computeStatelessChecksum(int num_args, ...)\n";
  code << "{\n";
  code << "    va_list args;\n";
  code << "    va_start(args, num_args);\n";
  switch (checksumType) {
    case 0:
      code << "    uint32_t checksum = 0xFFFFFFFFUL;\n";
      code << "    for (int i = 0; i < num_args; ++i) {\n";
      code << "        int arg = va_arg(args, int);\n";
      code << "        checksum = context_free_crc32_4bytes(checksum, arg);\n";
      code << "    }\n";
      code << "    checksum = uint32_to_int32(checksum ^ 0xFFFFFFFFUL);\n";
      break;
    case 1:
      code << "    int mod = 1000000007;\n";
      code << "    long checksum = 0;\n";
      code << "    for (int i = 0; i < num_args; ++i) {\n";
      code << "        int arg = va_arg(args, int);\n";
      code << "        checksum = (checksum + (long)arg) % (long)mod;\n";
      code << "        checksum = (checksum + (long)mod) % (long)mod;\n";
      code << "    }\n";
      code << "    checksum = (checksum + (long)mod) % (long)mod;\n";
      break;
    default:
      assert(false && "Invalid checksum type");
  }
  code << "    va_end(args);\n";
  code << "    return checksum;\n";
  code << "}\n";
  return code.str();
}

static uint32_t computeStatelessChecksum(std::vector<int> initialisation, int checksumType = CHECKSUM_TYPE) {
  switch (checksumType) {
    case 0: {
      uint32_t checksum = 0xFFFFFFFFUL;
      for (size_t i = 0; i < initialisation.size(); ++i) {
        checksum = context_free_crc32_4bytes(checksum, initialisation[i]);
      }
      return uint32_to_int32(checksum ^ 0xFFFFFFFFUL);
    }
    case 1: {
      long checksum = 0;
      for (auto i: initialisation) {
        checksum = (checksum + (long) i) % (long) CHECKSUM_MOD_PRM;
        checksum = (checksum + (long) CHECKSUM_MOD_PRM) % (long) CHECKSUM_MOD_PRM;
      }
      checksum = (checksum + (long) CHECKSUM_MOD_PRM) % (long) CHECKSUM_MOD_PRM;
      return checksum;
    }
    default: {
      assert(false && "Invalid checksum type");
      return 0;
    }
  }
}

class Statement {
public:
  enum class Type { ASSIGNMENT, IF, FLUFF };
  Type type;
  virtual std::string generateCode() const = 0;
  virtual Type getType() const = 0;
  virtual bool
  replaceCoefficient(std::string procedureName, std::vector<int> initialisation, std::vector<int> finalization) = 0;

  virtual int getNumCoefficients() const { return 0; };

  // Statement() = default;
};

class FluffStatement : public Statement {
  std::string code;

public:
  FluffStatement(std::string code) : code(code) {}

  std::string generateCode() const override { return code; }

  Type getType() const override { return Type::FLUFF; }

  bool replaceCoefficient(std::string procedureName, std::vector<int> initialisation, std::vector<int> finalization) {
    return false;
  }
};

using Coefficient = std::string;
using Variable = std::string;
using isCoefficientMutated = bool;

template <typename T1, typename T2, typename T3>
struct triple {
  T1 first;
  T2 second;
  T3 third;
};

class AssignmentStatement : public Statement {
  std::string lhsVar;
  std::vector<triple<Coefficient, Variable, isCoefficientMutated>>
      rhsTerms; // coefficient_value, is_mutated, variable

public:
  AssignmentStatement(std::string statement) {
    size_t equalPos = statement.find('=');
    lhsVar = statement.substr(0, equalPos);
    std::string rhs = statement.substr(equalPos + 1);
    std::stringstream ss(rhs);
    std::string token;
    while (std::getline(ss, token, '+') || std::getline(ss, token, ';')) {
      size_t mulPos = token.find('*');
      if (mulPos != std::string::npos) {
        Coefficient coeff = token.substr(0, mulPos);
        Variable var = token.substr(mulPos + 1);
        bool is_mutated = false; // Placeholder for mutation check
        rhsTerms.push_back({coeff, var, is_mutated});
      } else {
        mulPos = token.find(';');
        if (mulPos != std::string::npos) {
          token = token.substr(0, mulPos);
        }
        Coefficient coeff = token;
        Variable var = "1";
        bool is_mutated = false;                             // Placeholder for mutation check
        rhsTerms.push_back({coeff, var, is_mutated}); // Default coefficient of 1
      }
    }
  }

  std::string generateCode() const override {
    std::string code = lhsVar + " = ";
    for (size_t i = 0; i < rhsTerms.size(); ++i) {
      code += "(" + rhsTerms[i].first + ") * " + rhsTerms[i].second;
      if (i != rhsTerms.size() - 1) {
        code += " + ";
      }
    }
    code += ";";
    return code;
  }

  Type getType() const override { return Type::ASSIGNMENT; }

  int getNumCoefficients() const override { return rhsTerms.size(); }

  bool replaceCoefficient(std::string procedureName, std::vector<int> initialisation, std::vector<int> finalization) {
    for (auto &term: rhsTerms) {
      if (!term.third) {
        // replace the coefficient with a call to the procedure
        int coefficient = std::stoi(term.first);
        int checksum = computeStatelessChecksum(finalization);
        std::string replacement = "check_checksum(" + std::to_string(checksum) + ", " + procedureName + "(";
        for (int i = 0; i < initialisation.size() - 1; ++i) {
          replacement += std::to_string(initialisation[i]) + ", ";
        }
        replacement += std::to_string(initialisation[initialisation.size() - 1]);
        replacement += "))";
        long long diff = (long long) coefficient - (long long) checksum; // To avoid UBs
        if (diff >= (long long) INT32_MIN && diff <= (long long) INT32_MAX) {
          term.first = replacement + " + " + std::to_string(diff);
        } else {
          term.first = "(int) ((long long)" + replacement + " + " + std::to_string(diff) + "L)";
        }
        term.third = true;
        return true;
      }
    }
    return false;
  }
};

class IfStatement : public Statement {
  std::vector<triple<Coefficient, Variable, isCoefficientMutated>> condition;
  std::string gotoStatement;

public:
  IfStatement(std::string statement) {
    size_t ifPos = statement.find("if");
    size_t openParen = statement.find('(', ifPos);
    size_t closeParen = statement.find(')', openParen);
    std::string condition_str = statement.substr(openParen + 1, closeParen - openParen - 1);
    std::stringstream ss(condition_str);
    std::string token;
    while (std::getline(ss, token, '+') || std::getline(ss, token, ';')) {
      size_t mulPos = token.find('*');
      if (mulPos != std::string::npos) {
        Coefficient coeff = token.substr(0, mulPos);
        Variable var = token.substr(mulPos + 1);
        bool is_mutated = false; // Placeholder for mutation check
        condition.push_back({coeff, var, is_mutated});
      } else {
        mulPos = token.find(">=");
        if (mulPos != std::string::npos) {
          token = token.substr(0, mulPos);
        }
        Coefficient coeff = token;
        Variable var = "1";
        bool is_mutated = false;                         // Placeholder for mutation check
        condition.push_back({coeff, var, is_mutated}); // Default coefficient of 1
      }
    }
    gotoStatement = statement.substr(closeParen + 1);
  }

  int getNumCoefficients() const { return condition.size(); }

  //
  bool replaceCoefficient(std::string procedureName, std::vector<int> initialisation, std::vector<int> finalization) {
    for (auto &term: condition) {
      if (!term.third && term.first != "pass_counter ") {
        // replace the coefficient with a call to the procedure
        int coefficient = std::stoi(term.first);
        int checksum = computeStatelessChecksum(finalization);
        std::string replacement = "check_checksum(" + std::to_string(checksum) + ", " + procedureName + "(";
        for (int i = 0; i < initialisation.size() - 1; ++i) {
          replacement += std::to_string(initialisation[i]) + ", ";
        }
        replacement += std::to_string(initialisation[initialisation.size() - 1]);
        replacement += "))";
        long long diff = (long long) coefficient - (long long) checksum;
        if (diff >= (long long) INT32_MIN && diff <= (long long) INT32_MAX) {
          term.first = replacement + " + " + std::to_string(diff);
        } else {
          assert((int)((long long)checksum + diff) == coefficient && "Not same");
          term.first = "(int) ((long long)" + replacement + " + " + std::to_string(diff) + "L)";
        }
        term.third = true;
        return true;
      }
    }
    return false;
  }

  std::string generateCode() const override {
    std::string code = "if (";
    for (size_t i = 0; i < condition.size(); ++i) {
      code += "(" + condition[i].first + ") * " + condition[i].second;
      if (i != condition.size() - 1) {
        code += " + ";
      }
    }
    code += " >= 0) " + gotoStatement;
    return code;
  }

  Type getType() const override { return Type::IF; }
};

class Procedure {
private:
  std::string name;
  int num_input_vars;
  int total_num_coefficients = 0;
  std::unordered_map<std::string, int> parameters;
  std::vector<std::unique_ptr<Statement>> statements;
  std::vector<std::vector<int>> initialisations;
  std::vector<std::vector<int>> finalizations;

public:
  Procedure() {}

  Procedure(std::ifstream &procedureFile, std::ifstream &mappingFile) {
    std::string line;
    // Parse the procedure
    while (std::getline(procedureFile, line)) {
      if (line.find("if") != std::string::npos) {
        if (line.find("pass_counter") != std::string::npos) {
          statements.push_back(std::make_unique<FluffStatement>(line));
        } else {
          statements.push_back(std::make_unique<IfStatement>(line));
          total_num_coefficients += statements.back()->getNumCoefficients();
        }
      } else if (line.find("int") != std::string::npos) {
        if (line.find("pass_counter") != std::string::npos) {
          statements.push_back(std::make_unique<FluffStatement>(line));
          continue;
        }
        name = line.substr(line.find("int") + 4, line.find('(') - line.find("int") - 4);
        size_t start = line.find('(') + 1;
        size_t end = line.find(')');
        std::string params = line.substr(start, end - start);
        num_input_vars = std::count(params.begin(), params.end(), ',') + 1;
        statements.push_back(std::make_unique<FluffStatement>(line));
      } else if (line.find("return") != std::string::npos) {
        // End of the procedure
        // modify the return computeStatelessChecksum function to take in
        // num_input_vars as an argument
        std::string newLine = "return computeStatelessChecksum(" + std::to_string(num_input_vars);
        for (int i = 0; i < num_input_vars; ++i) {
          newLine += ", var_" + std::to_string(i);
        }
        newLine += ");";
        statements.push_back(std::make_unique<FluffStatement>(newLine));
        // statements.push_back(std::make_unique<FluffStatement>(line));
      } else if (line.find("BB") != std::string::npos) {
        // Start a new basic block
        statements.push_back(std::make_unique<FluffStatement>(line));

      } else if (line.find("=") != std::string::npos) {
        // Add statement to the current basic block
        statements.push_back(std::make_unique<AssignmentStatement>(line));
        total_num_coefficients += statements.back()->getNumCoefficients();
      } else {
        // Add fluff statement
        statements.push_back(std::make_unique<FluffStatement>(line));
      }
    }
    // Parse its mapping
    while (std::getline(mappingFile, line)) {
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

  bool replaceCoefficient(std::string procedureName, std::vector<int> initialisation, std::vector<int> finalization) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 1);
    auto probability = dis(gen);
    int trials = 1000;
    // Sample a statement from the list of statements for replacement
    while (true && trials > 0) {
      int index = rand() % statements.size();
      // Idea: 80% of the replacements should be in the IF statements, and 20%  in the assignment statements
      if (statements[index]->getType() == Statement::Type::IF && probability < 0.8) {
        if (statements[index]->replaceCoefficient(procedureName, initialisation, finalization)) {
          return true;
        }
      } else if (statements[index]->getType() == Statement::Type::ASSIGNMENT && probability > 0.8) {
        if (statements[index]->replaceCoefficient(procedureName, initialisation, finalization)) {
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
      code += statement->generateCode() + "\n";
    }
    return code;
  }

  void getMapping(std::vector<int> &foreign_initialisation, std::vector<int> &foreign_finalization) const {
    // sample a random index from initialisations list
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, initialisations.size() - 1);
    int index = dis(gen);
    foreign_initialisation = initialisations[index];
    foreign_finalization = finalizations[index];
  }

  int calculateNumCoefficientsToReplace() const { return total_num_coefficients * REPLACEMENT_PROBABILITY; }
};

void generateCodeForInterproceduralBlock(
    const std::filesystem::path filename, const std::vector<Procedure> &procedures, bool debug = false, bool staticModifier = false,
    int checksumType = CHECKSUM_TYPE) {
  std::ofstream outFile(filename);
  outFile << "#include <stdint.h>\n";
  outFile << "#include <stdio.h>\n";
  outFile << "#include <stdarg.h>\n";
  outFile << generateCodeForChecksumFunction(checksumType);
  if (debug) {
    outFile << "#include <assert.h>\n";
    outFile << "static inline int check_checksum(int expected, int actual) { assert(expected==actual && \"Checksum not equal\"); return actual; }\n";
  } else {
    outFile << "#define check_checksum(expected, actual) (actual)\n";
  }
  outFile << "\n";
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dis(0, 1);
  for (int i = PROCEDURE_DEPTH - 1; i >= 0; --i) {
    // output inline with 60% probability
    auto probability = dis(gen);
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
    outFile << procedures[i].generateCode() << "\n";
  }
  outFile << "int main() {\n";
  if (checksumType == 0) {
    outFile << "    generate_crc32_table(crc32_tab);\n";
  }
  std::vector<int> initialisation;
  std::vector<int> finalization;
  procedures[0].getMapping(initialisation, finalization);
  int checksum = computeStatelessChecksum(finalization);
  outFile << "    printf(\"%d,\", check_checksum(" << checksum << ", " << procedures[0].getName() << "(";
  for (int i = 0; i < initialisation.size(); ++i) {
    outFile << initialisation[i];
    if (i != initialisation.size() - 1) {
      outFile << ", ";
    }
  }
  outFile << ")));\n";
  outFile << "    return 0;\n";
  outFile << "}\n";
  outFile.close();
  // std::cout << "Code generated successfully in " << filename << std::endl;
}


struct CliOpts {
  std::string procedures;
  std::string mappings;
  std::string uuid;
  int limits;
  bool debug;
};

CliOpts parseCliOpts(int argc, char **argv) {
  cxxopts::Options options("pgen");
  // clang-format off
  options.add_options()
    ("uuid", "An UUID identifier", cxxopts::value<std::string>())
    ("p,procedures", "The directory saving the seed procedures", cxxopts::value<std::string>())
    ("m,mappings", "The directory saving the mappings for the the seed procedures", cxxopts::value<std::string>())
    ("l,limit", "The number of new procedures to generate (0 for unlimited generation)", cxxopts::value<int>()->default_value("0"))
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

  std::string procedures;
  if (!args.count("procedures")) {
    std::cerr << "Error: The procedures directory (--procedures) is not given." << std::endl;
    exit(1);
  } else {
    procedures = args["procedures"].as<std::string>();
  }

  std::string mappings;
  if (!args.count("mappings")) {
    std::cerr << "Error: The mappings directory (--mappings) is not given." << std::endl;
    exit(1);
  } else {
    mappings = args["mappings"].as<std::string>();
  }

  int limit = 0;
  if (args.count("limit")) {
    limit = args["limit"].as<int>();
  }

  bool debug = false;
  if (args.count("debug")) {
      debug = true;
  }

  return {.procedures = procedures, .mappings = mappings, .uuid = uuid, .limits = limit, .debug = debug};
}

int main(int argc, char *argv[]) {
  CliOpts cliOpts = parseCliOpts(argc, argv);

  std::filesystem::path procedureDirectory(cliOpts.procedures);
  std::filesystem::path mappingDirectory(cliOpts.mappings);
  std::string new_procedure_uuid = cliOpts.uuid;
  int generateLimit = cliOpts.limits;
  bool enableDebug = cliOpts.debug;

  // Read all files from procedureDirectory
  std::vector<std::filesystem::path> allProcFiles;
  // Open the directory and read all files
  for (const auto &entry: std::filesystem::directory_iterator(procedureDirectory)) {
    if (entry.is_regular_file()) {
      allProcFiles.push_back(entry.path());
    }
  }

  generate_crc32_table(crc32_table);
  std::vector<std::filesystem::path *> selProcFiles;
  selProcFiles.resize(PROCEDURE_DEPTH);
  std::vector<Procedure> selProcedures;
  selProcedures.resize(PROCEDURE_DEPTH);

  for (int sampNo = 0; generateLimit == 0 || sampNo < generateLimit; ++sampNo) {
    std::cout << "[" << sampNo << "] Generating ... " << std::endl;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, allProcFiles.size() - 1);

    // Choose the procedure files randomly
    // Fixed Unsafe memset(vector<string>, 0, sizeof(string)!!
    // std::string saves its data in the heap. That said, clears the vector's internal data
    // does not lead to the auto deletion of the string's internal data. Small string optimization does not
    // work here as the path of each our procedure is indeed more than 25 characters.
    memset(selProcFiles.data(), 0, selProcFiles.size() * sizeof(std::filesystem::path *));
    for (int i = 0; i < PROCEDURE_DEPTH; ++i) {
      while (true) {
        int index = dis(gen);
        if (std::find(selProcFiles.begin(), selProcFiles.end(), &allProcFiles[index]) == selProcFiles.end()) {
          selProcFiles[i] = &allProcFiles[index];
          break;
        }
      }
    }

    // Parse all selected procedure files
    for (int i = 0; i < PROCEDURE_DEPTH; ++i) {
      std::filesystem::path procFile = *selProcFiles[i];
      std::filesystem::path mapFile = mappingDirectory / GetMappingNameForProcedureName(procFile.stem().string());
      std::cout << "[" << sampNo << "] Opening procedure file: " << procFile << std::endl;
      std::ifstream procIFS(procFile);
      std::cout << "[" << sampNo << "] Opening mapping file: " << mapFile << std::endl;
      std::ifstream mapIFS(mapFile);
      if (!procIFS.is_open() || !mapIFS.is_open()) {
        std::cerr << "Error opening file: " << procFile.stem() << std::endl;
        continue; // TODO: Continue??? The i-th procedure remains the one in the last sampling?
      }
      selProcedures[i] = Procedure(procIFS, mapIFS);
      // std::cout << "Procedure name: " << procedures[i].getName() <<
      // std::endl;
      procIFS.close();
      mapIFS.close();
    }

    std::vector<int> initialisation;
    std::vector<int> finalization;
    // Now replace the mappings in the procedures with the calls to the other procedures
    for (int i = 0; i < PROCEDURE_DEPTH; ++i) {
      if (i == PROCEDURE_DEPTH - 1) {
        break;
      }
      // Sample a procedure from i + 1 to PROCEDURE_DEPTH
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<> dis(i + 1, PROCEDURE_DEPTH - 1);

      // Calculate all replaceable coefficients
      int num_mappings_to_replace = selProcedures[i].calculateNumCoefficientsToReplace();
      // std::cout << "Number of mappings to replace: " << num_mappings_to_replace << std::endl;

      // For each coefficient that can be replaced, we find a procedure to replace it
      for (int k = 0; k < num_mappings_to_replace; ++k) {
        int index = dis(gen);
        selProcedures[index].getMapping(initialisation, finalization);
        if (!selProcedures[i].replaceCoefficient(selProcedures[index].getName(), initialisation, finalization)) {
          break; // TODO: break or continue? We can continue to the next, can't we?
        }
        // std::cout<<"Replacing mapping for procedure: " <<
        // procedures[index].getName() << " in procedure: " <<
        // procedures[i].getName() << std::endl;
      }
    }

    // std::cout<< "Now generating code for interprocedural block" << std::endl;
    std::string newProcedureName = new_procedure_uuid + "_" + std::to_string(sampNo);
    generateCodeForInterproceduralBlock(NEW_PROCEDURES_DIR / (newProcedureName + ".c"), selProcedures, enableDebug);
    generateCodeForInterproceduralBlock(NEW_PROCEDURES_DIR / (newProcedureName + "_static.c"), selProcedures, enableDebug, true);
  }
}
