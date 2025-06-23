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

#include "lib/function.hpp"

#include <sstream>

#include "global.hpp"
#include "lib/chksum.hpp"
#include "lib/logger.hpp"
#include "lib/naming.hpp"
#include "lib/random.hpp"

void FunGen::Generate() {
  Log::Get().OpenSection("FunGen: Function Generation");
  // Generate the sketch of our control flow graph
  cfg.Generate();
  for (int i = 0; i < Random::Get().Uniform(0, maxNumLoops)(); i++) {
    cfg.GenerateReduLoop(maxNumBblsPerLoop);
  }
  cfg.Print();
  // Map each basic block in the CFG to a basic block generator
  for (int i = 0; i < NumBbls(); i++) {
    bblGens.emplace_back(*this, i, cfg.GetBbl(i));
    bblGens.back().Generate();
  }
  Log::Get().CloseSection();
}

std::vector<int> FunGen::SampleExec(int execStep, bool consistent) {
  return cfg.SampleExec(execStep, consistent);
}

std::string FunGen::GenerateFunCode(const std::string &funName, const UBFreeExec &exec) const {
  std::ostringstream code;
  code << "int " << funName << "(";
  for (int i = 0; i < numParams; ++i) {
    code << "int " << NameVar(i);
    if (i < numParams - 1) {
      code << ", ";
    }
  }
  code << ")" << std::endl;
  code << "{" << std::endl;
  if (exec.NeedPassCounter()) {
    code << "    int " << NamePassCounter() << " = 0;" << std::endl;
  } else {
    code << "    // No pass counter needed" << std::endl;
  }

  for (const auto &bbl: bblGens) {
    code << bbl.GenerateCode(exec) << std::endl;
  }
  code << "}" << std::endl;
  return code.str();
}

std::string
FunGen::GenerateMainCode(const std::string &funName, const UBFreeExec &exec, bool debug) const {
  const auto &initializations = exec.GetInitializations();
  const auto &finalizations = exec.GetFinalizations();
  Assert(
      initializations.size() == finalizations.size(),
      "Initializations and finalizations must have the same size"
  );
  std::ostringstream main;
  main << StatelessChecksum::GetCheckChksumCode(debug) << std::endl;
  main << "int main(int argc, int* argv[])" << std::endl;
  main << "{" << std::endl;
  for (auto i = 0; i < initializations.size(); i++) {
    const auto &init = initializations[i];
    const auto &fina = finalizations[i];
    const auto numParams = static_cast<int>(init.size());
    std::ostringstream chk_oss;
    main << "    " << StatelessChecksum::GetCheckChksumName() << "("
         << StatelessChecksum::Compute(fina) << ", " << funName << "(";
    for (auto j = 0; j < numParams; j++) {
      main << init[j];
      if (j < numParams - 1) {
        main << ", ";
      }
    }
    main << "));" << std::endl;
  }
  main << "  return 0;" << std::endl;
  main << "}" << std::endl;
  return main.str();
}

std::string FunGen::GenerateMappingCode(const UBFreeExec &exec) {
  std::ostringstream mapping;
  for (int i = 0; i < exec.GetInitializations().size(); i++) {
    for (auto x: exec.GetInitializations()[i]) {
      mapping << x << ",";
    }
    mapping << " : ";
    for (auto x: exec.GetFinalizations()[i]) {
      mapping << x << ",";
    }
    mapping << std::endl;
  }
  return mapping.str();
}
