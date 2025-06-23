// MIT License
//
// Copyright (c) 2025
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

#include "lib/program.hpp"

#include <fstream>

#include "lib/logger.hpp"

std::unique_ptr<Function> Function::Of(const fs::path &funPath, const fs::path &mapPath) {
  std::string tmpLine;

  std::ostringstream foss;
  std::ifstream fifs(funPath);
  Assert(fifs.is_open(), "Error: failed to open file: %s", funPath.string().c_str());
  while (std::getline(fifs, tmpLine)) {
    foss << tmpLine << std::endl;
  }
  fifs.close();

  std::ostringstream moss;
  std::ifstream mifs(mapPath);
  Assert(mifs.is_open(), "Error: failed to open file: %s", mapPath.string().c_str());
  while (std::getline(mifs, tmpLine)) {
    moss << tmpLine << std::endl;
  }
  mifs.close();

  auto fun = Create<Function>(foss.str());
  fun->ParseMapping(moss.str());

  return fun;
}

void Function::ParseMapping(const std::string &mapping) {
  std::istringstream iss(mapping);
  std::string line;
  while (std::getline(iss, line)) {
    std::vector<std::string> iniFin = SplitStr(line, " : ", true);
    Assert(
        iniFin.size() == 2,
        "Invalid mapping line, each line should be in the format of initialisation : finalization"
    );

    std::vector<std::string> ini = SplitStr(iniFin[0], ",", true);
    std::vector<std::string> fin = SplitStr(iniFin[1], ",", true);
    Assert(ini.size() == fin.size(), "The size of initialisation and finalization is different");

    initialisations.emplace_back(ini.size());
    std::ranges::transform(ini, initialisations.back().begin(), [](const auto &s) -> int {
      return std::stoi(s);
    });
    finalizations.emplace_back(fin.size());
    std::ranges::transform(fin, finalizations.back().begin(), [](const auto &s) -> int {
      return std::stoi(s);
    });
  }
}

[[nodiscard]] bool Function::RepFirstCoeff(
    const std::string &funName, const std::vector<int> &initialisation,
    const std::vector<int> &finalization
) const {
  const auto rand = Random::Get().Uniform(0, (int) statements.size() - 1);
  const auto probability = Random::Get().UniformReal()();
  // Sample a statement from the list of statements for replacement
  for (int tries = 0; tries < 1000; tries++) {
    const int index = rand();
    const auto &stmt = statements[index];
    // Idea: 80% of the replacements should be in the IF statements, and 20% in assignments
    double threshold;
    switch (stmt->GetStmtType()) {
      case Statement::Type::IFGOTO:
      case Statement::Type::WHILE:
        threshold = 0.4;
        break;
      case Statement::Type::ASSIGN:
        threshold = 0.2;
        break;
      default:
        threshold = 0;
        break;
    }
    if (probability < threshold && stmt->RepFirstCoeff(funName, initialisation, finalization)) {
      return true;
    }
  }
  return false;
}

void Function::Parse(const std::string &code) {
  std::istringstream iss(code);
  std::string line;
  // Parse the function line by line
  while (std::getline(iss, line)) {
    if (line.find("pass_counter") != std::string::npos) {
      // Our replacement respect pass_counter
      statements.push_back(Create<FluffStmt>(line));
    } else if (line.find("do {") != std::string::npos) {
      statements.push_back(Create<FluffStmt>(line));
    } else if (line.find("} while") != std::string::npos) {
      statements.push_back(Create<WhileStmt>(line));
      numCoeffs += statements.back()->GetNumCoeffs();
    } else if (line.find("if") != std::string::npos) {
      statements.push_back(Create<IfGotoStmt>(line));
      numCoeffs += statements.back()->GetNumCoeffs();
    } else if (line.find("int function") != std::string::npos) {
      // Start of the function
      auto nameStart = line.find("function");
      auto paramStart = line.find('(');
      auto paramEnd = line.find(')');
      name = line.substr(nameStart, paramStart - nameStart);
      std::string params = line.substr(paramStart + 1, paramEnd - paramStart);
      numParams = static_cast<int>(std::ranges::count(params, ',')) + 1;
      statements.push_back(Create<FluffStmt>(line));
    } else if (line.find("return") != std::string::npos) {
      statements.push_back(Create<FluffStmt>(line));
    } else if (line.find("BB") != std::string::npos) {
      // Start a new basic block
      statements.push_back(Create<FluffStmt>(line));
    } else if (line.find('=') != std::string::npos) {
      // Add statement to the current basic block
      statements.push_back(Create<AssignStmt>(line));
      numCoeffs += statements.back()->GetNumCoeffs();
    } else {
      // Add it as a fluff statement
      statements.push_back(Create<FluffStmt>(line));
    }
  }
}

void ProgGen::Generate() const {
  const int numFuns = static_cast<int>(functions.size());
  Assert(
      numFuns == GlobalOptions::Get().FunctionDepth,
      "The number of selected functions is not FUNCTION_DEPTH"
  );

  std::vector<int> initialisation;
  std::vector<int> finalization;

  // Now replace the mappings in the functions with the calls to the other functions
  for (int i = 0; i < numFuns - 1; ++i) {
    auto host = functions[i].get();
    int numRepCoeffs = host->GetNumRepCoeffs();

    Log::Get().Out() << "[" << sno << "] Replacing function" << ": index=" << i
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
      Log::Get().Out() << "[" << sno << "]   var#" << k << " -> func#" << j << ": "
                       << guest->GetName() << std::endl;
    }

    Log::Get().Out() << "[" << sno << "]   Done" << std::endl;
  }
}

void ProgGen::GenerateCode(const fs::path &file, bool debug, bool staticModifier) const {
  std::ofstream oss(file);
  oss << StatelessChecksum::GetRawCode() << std::endl;
  oss << "#include <stdio.h>" << std::endl;
  oss << StatelessChecksum::GetCheckChksumCode(debug) << std::endl;
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
  oss << "    printf(\"%d,\", " << StatelessChecksum::GetCheckChksumName() << "(" << checksum
      << ", " << functions[0]->GetName() << "(";
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
