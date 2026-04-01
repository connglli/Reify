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

#include <fstream>
#include <string>

#include "lib/chksum.hpp"
#include "lib/parsers.hpp"
#include "lib/program.hpp"
#include "lib/random.hpp"
#include "lib/fcallembed.hpp"
#include "lib/varstate.hpp"

ProgPlus::ProgPlus(std::string uuid, const int sno, const std::vector<std::string> &funPaths) :
    uuid(std::move(uuid)), sno(std::to_string(sno)) {
  // Parse all selected function files
  int idx = 0;
  std::set<std::string> funNames;
  for (const auto &funPath: funPaths) {
    FunArts arts(funPath);
    fs::path sexpPath = arts.GetSexpPath();
    if (!fs::exists(sexpPath)) {
      Panic(
          "S Expression file not found for file: %s. Did you run rysmith with --sexpression?",
          sexpPath.c_str()
      );
    }
    // Assume no function name and struct name conflicts across different files
    auto func = FunPlus::ParseFunSexpCode(sexpPath);
    Assert(
        !funNames.contains(func->GetName()), "Function name conflict: %s", func->GetName().c_str()
    );
    funNames.insert(func->GetName());
    this->functions.push_back(std::move(func));
    auto mapping = FunPlus::ParseMappingCode(arts.GetMapPath());
    this->mappings.push_back(std::move(mapping));
    this->varStates.push_back(varstate::allFromJsonFile(arts.GetVarStatePath()));

    Assert(functions.back() != nullptr, "The function for \"%s\" is nullptr", funPath.c_str());
    idx++;
  }
}

void ProgPlus::Generate() {
  const int numFuns = static_cast<int>(functions.size());

  // Now replace the mappings in the functions with the calls to the other functions
  for (int i = 0; i < numFuns - 1; ++i) {
    auto host = functions[i].get();
    int numCoeffs = host->GetSymbols().size();

    Log::Get().OpenSection("Host function (" + std::to_string(i) + "): " + host->GetName());
    Log::Get().Out() << "num_replaceable=" << numCoeffs << std::endl;

    auto emb = RandomFCallEmbedder(host);
    //auto strat = LiteralFCallStrategy();
    auto strat = PrimeInterpFCallStrategy();
    emb.setStrategy(&strat);
    Log::Get().Out() << "Embed Strategy: " << strat.getStrategyName() << std::endl;


    // Random Generator to sample a function from i + 1 to the end
    auto rand = Random::Get().Uniform(i + 1, numFuns - 1);
    auto randU = Random::Get().UniformReal();
    // TODO: since we are only replacing on path coeffs using numCoeffs forces ReplaceProba to be really low (e.g. 0.05)
    // Hence we need a better way to come up with a way to generate a random number of repacements
    int randNum = Random::Get().Binomial(numCoeffs - 1, GlobalOptions::Get().ReplaceProba)();

    // we replace randNum coeffs with function calls
    for (int k = 0; k < randNum; ++k) {
      int j = rand();
      auto guest = functions[j].get();
      Assert(guest != nullptr, "The guest function is nullptr for index %d", j);
      auto guestMap = mappings[j];

      int index = Random::Get().Uniform(0, static_cast<int>(guestMap.first.size()) - 1)();
      std::vector<ArgPlus<int>> *init = &guestMap.first[index];
      std::vector<ArgPlus<int>> *fina = &guestMap.second[index];
      emb.setVarStateQueries(&this->varStates[i]);
      emb.createPathBlockWhitelist();

      Log::Get().OpenSection("Embedding " + guest->GetName());
      Log::Get().Out() << "Initials: ";
      for (size_t i = 0 ; i < init->size(); i++) {
        Log::Get().Out() << (*init)[i].ToCxStr() << ", ";
      }
      Log::Get().Out() << (*init).back().ToCxStr() << std::endl;

      if (emb.embedGuest(guest, init, fina)) {
        Log::Get().Out() << "Embed: " << k << "/" << randNum 
                         << " Success: " << "func#" << j << ": "
                         << guest->GetName() << std::endl;
      } else {
        Log::Get().Out() << "Embed: " << k << "/" << randNum 
                         << " Failed: " << "func#" << j << ": "
                         << guest->GetName() << std::endl;
      }
      Log::Get().CloseSection();
    }

    functions[i] = emb.finalize();
    Log::Get().CloseSection();
  }
}

void ProgPlus::GenerateCode(const ProgArts &arts) const {
  // Generate the checksum code
  std::ofstream chksumFile(arts.GetChksumPath());
  chksumFile << StatelessChecksum::GetRawCode() << std::endl;
  chksumFile.close();

  // Generate all functions' prototypes
  std::ofstream protoFile(arts.GetProtoPath());
  protoFile << "#ifndef PROTOTYPES_H" << std::endl;
  protoFile << "#define PROTOTYPES_H" << std::endl << std::endl;
  protoFile << "#define RM(var, mod) ((var % mod + mod) % mod)" << std::endl << std::endl;
  protoFile << StatelessChecksum::GetCheckChksumCode(/*debug=*/true) << std::endl;
  protoFile << "extern " << StatelessChecksum::GetCrc32InitPrototype() << ";" << std::endl;
  protoFile << "extern " << StatelessChecksum::GetComputePrototype() << ";" << std::endl;
  for (const auto &fun: functions) {
    protoFile << symir::SymCxLower::GetFunPrototype(*fun) << ";" << std::endl;
  }
  protoFile << std::endl << "#endif" << std::endl;
  protoFile.close();

  // Generate all functions
  for (const auto &fun: functions) {
    std::ofstream funFile(arts.GetFunPath(fun->GetName()));
    funFile << "#include \"" << FILENAME_PROTOTYPES_H << "\"" << std::endl << std::endl;
    symir::SymCxLower lower(funFile, false);
    lower.Lower(*fun);
    funFile.close();
  }

  // Generate the main function
  std::ofstream mainFile(arts.GetMainPath());
  mainFile << "#include <stdio.h>" << std::endl;
  mainFile << "#include \"" << FILENAME_PROTOTYPES_H << "\"" << std::endl;
  mainFile << "int main() {" << std::endl;
  mainFile << "    " << StatelessChecksum::GetCrc32InitName() << "();" << std::endl;

  // Use the first function (index 0) as the entry point
  int index = Random::Get().Uniform(0, static_cast<int>(mappings[0].first.size()) - 1)();
  const std::vector<ArgPlus<int>> *init = &(mappings[0].first[index]);
  const std::vector<ArgPlus<int>> *fina = &(mappings[0].second[index]);
  int checksum = StatelessChecksum::Compute(*fina);

  mainFile << "    printf(\"%d\\n\", " << StatelessChecksum::GetCheckChksumName() << "(" << checksum
           << ", " << functions[0]->GetName() << "(";

  const auto &params = functions[0]->GetParams();
  for (size_t i = 0; i < init->size(); ++i) {
    auto arg = (*init)[i];
    // Use the new type-safe GetTypeCastStr method
    mainFile << arg.GetTypeCastStr(params[i]) << arg.ToCxStr();
    if (i != init->size() - 1) {
      mainFile << ", ";
    }
  }
  mainFile << ")));" << std::endl;
  mainFile << "    return 0;" << std::endl;
  mainFile << "}" << std::endl;
  mainFile.close();
}
