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

#ifndef GRAPHFUZZ_FUNCTION_HPP
#define GRAPHFUZZ_FUNCTION_HPP

#include <optional>
#include <string>
#include <vector>

#include "lib/block.hpp"
#include "lib/ctrlflow.hpp"
#include "lib/dbgutils.hpp"
#include "lib/ubfexec.hpp"
#include "logger.hpp"

class BlkGen;
class UBFreeExec;

/// FunGen is a function generator which populates each the whole function with real statements
class FunGen {
public:
  explicit FunGen(int numBBs, int numParams, int maxNumLoops, int maxNumBblsPerLoop) :
      cfg(numBBs), numParams(numParams), maxNumLoops(maxNumLoops),
      maxNumBblsPerLoop(maxNumBblsPerLoop) {}

  // FunGen(const FunGen &) = delete;
  // FunGen &operator=(const FunGen &) = delete;

  // Get the underlying control flow graph
  [[nodiscard]] const auto &GetCfg() const { return cfg; }

  // Get the basic block generators in the function
  [[nodiscard]] const auto &GetBbls() const { return bblGens; }

  // Get the basic block generator for a specific basic block
  [[nodiscard]] const auto &GetBbl(int bblNo) const { return bblGens[bblNo]; }

  // Get the symbols defined by the function
  [[nodiscard]] const auto &GetSymbols() const { return symbols; }

  // Get the number of basic blocks in the function
  [[nodiscard]] int NumBbls() const { return cfg.NumBbls(); }

  // Get the number of parameters used by the function
  [[nodiscard]] int NumParams() const { return numParams; }

  // Define a symbol used by the function
  void DefSymbol(const std::string &name) {
    Assert(!symbols.contains(name), "Symbol with name %s was already defined!", name.c_str());
    symbols.insert(name);
    Log::Get().Out() << "Define symbol into function: " << name << std::endl;
  }

  // Generate the function with random control-flow, statements, and symbols
  void Generate();

  // Sample an execution of the function.
  std::vector<int> SampleExec(int execStep, bool consistent);

  // Generate the function code of the function for a given execution
  std::string GenerateFunCode(const std::string &funName, const UBFreeExec &exec) const;

  // Generate the main code of the function for a given execution
  std::string
  GenerateMainCode(const std::string &funName, const UBFreeExec &exec, bool debug = false) const;

  // Generate the map of initialisation-finalisation for a given execution
  static std::string GenerateMappingCode(const UBFreeExec &exec);

private:
  CfgSketch cfg; // The sketch of our CFG, where each bbl maps to a blockgen in bbs

  int numParams;         // The number of parameters that the function has
  int maxNumLoops;       // The maximum number of loops that the function can have
  int maxNumBblsPerLoop; // The maximum number of basic blocks in a loop

  std::vector<BlkGen> bblGens{}; // The basic block generators (in the same order as basic blocks)
  std::set<std::string> symbols; // The symbols (i.e., coefficient) defined by this function.
};


#endif // GRAPHFUZZ_FUNCTION_HPP
