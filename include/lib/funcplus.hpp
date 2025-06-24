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

#ifndef GRAPHFUZZ_FUNCPLUS_HPP
#define GRAPHFUZZ_FUNCPLUS_HPP

#include <optional>
#include <string>
#include <vector>

#include "lib/ctrlflow.hpp"
#include "lib/dbgutils.hpp"
#include "lib/lang.hpp"
#include "lib/logger.hpp"
#include "lib/ubfexec.hpp"

/// FunPlus is a function generator which populates each the whole function with real statements
class FunPlus {
public:
  explicit FunPlus(
      std::string name, int numParams, int numBBs, int maxNumLoops, int maxNumBblsPerLoop
  ) :
      cfg(numBBs), name(std::move(name)), numParams(numParams), maxNumLoops(maxNumLoops),
      maxNumBblsPerLoop(maxNumBblsPerLoop) {}

  // Get the name of the function
  [[nodiscard]] const std::string &GetName() const { return name; }

  // Get the number of parameters used by the function
  [[nodiscard]] int NumParams() const { return numParams; }

  // Get the number of basic blocks in the function
  [[nodiscard]] int NumBbls() const { return cfg.NumBbls(); }

  // Get the underlying control flow graph
  [[nodiscard]] const auto &GetCfg() const { return cfg; }

  [[nodiscard]] symir::Func *GetFun() const {
    Assert(fun != nullptr, "The function has not yet been generated");
    return fun.get();
  }

  // Get the symbols defined by the function
  [[nodiscard]] auto GetSymbols() const {
    Assert(fun != nullptr, "The function has not yet been generated");
    return fun->GetSymbols();
  }

  // Generate the function with random control-flow, statements, and symbols
  void Generate();

  // Sample an execution of the function.
  std::vector<int> SampleExec(int execStep, bool consistent);

private:
  // Generate a new basic block with random statements and symbols
  void generateBasicBlock(symir::FuncBuilder *funBd, int bblId, const BblSketch &bblSkt);

private:
  CfgSketch cfg; // The sketch of our CFG, where each bbl maps to a blockgen in bbs

  std::string name;      // The name of the function
  int numParams;         // The number of parameters that the function has
  int maxNumLoops;       // The maximum number of loops that the function can have
  int maxNumBblsPerLoop; // The maximum number of basic blocks in a loop

  std::unique_ptr<symir::Func> fun = nullptr; // The built function in SymIR
};


#endif // GRAPHFUZZ_FUNCPLUS_HPP
