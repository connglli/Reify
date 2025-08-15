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

#include "jnif/jnif.hpp"
#include "lib/ctrlflow.hpp"
#include "lib/dbgutils.hpp"
#include "lib/lang.hpp"
#include "lib/logger.hpp"
#include "lib/ubfexec.hpp"

class UBFreeExec;

/// FunPlus is a generator which populates the generates a function with
/// random contrl flow, variables, and statements
class FunPlus {
public:
  using InitFinaMap = std::pair<std::vector<std::vector<int>>, std::vector<std::vector<int>>>;

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

  // Get the ID of the entry basic block of the function
  [[nodiscard]] int GetEntryBblId() const {
    return 0; // Following our CFG, the entry block is always the first basic block
  }

  // Get the ID of the exit basic block of the function
  [[nodiscard]] int GetExitBblId() const {
    return NumBbls() - 1; // Following our CFG, the exit block is always the last basic block
  }

  // Get the build function from the generator
  [[nodiscard]] symir::Funct *GetFun() const {
    Assert(fun != nullptr, "The function has not yet been generated");
    return fun.get();
  }

  // Set a graph set for sampling control-flow graphs
  void SetGraphSet(std::string file) { cfg.SetGraphSet(std::move(file)); }

  // Generate the function with random control-flow, statements, and symbols
  void Generate();

  // Sample an execution of the function.
  std::vector<int> SampleExec(int execStep, bool consistent);

  // Generate the C code of the function for a given execution
  [[nodiscard]] std::string GenerateFunCode(const UBFreeExec &exec) const;

  // Generate the C main() code of the function for a given execution
  [[nodiscard]] std::string GenerateMainCode(const UBFreeExec &exec, bool debug = false) const;

  // Generate the S Expression code of the function for a given execution
  [[nodiscard]] std::string GenerateFunSexpCode(const UBFreeExec &exec) const;

  // Generate the Java code of the function for a given execution with or without main()
  [[nodiscard]] std::unique_ptr<jnif::ClassFile> GenerateFunJavaCode(
      const UBFreeExec &exec, const std::string &className, bool main = false, bool debug = false
  ) const;

  // Generate the map of initialisation-finalisation for a given execution
  [[nodiscard]] std::string GenerateMappingCode(const UBFreeExec &exec) const;

  // Parse the S Expression code of a function and return the Funct object
  [[nodiscard]] static std::unique_ptr<symir::Funct> ParseFunSexpCode(const std::string &funPath);

  // Parse the map of initialisation-finalisation and return them
  [[nodiscard]] static InitFinaMap ParseMappingCode(const std::string &mapPath);

private:
  // Generate a new basic block with random statements and symbols
  void generateBasicBlock(symir::FunctBuilder *funBd, int bblId, const BblSketch &bblSkt);

private:
  CfgSketch cfg; // The sketch of our CFG, where each bbl maps to a blockgen in bbs

  std::string name;      // The name of the function
  int numParams;         // The number of parameters that the function has
  int maxNumLoops;       // The maximum number of loops that the function can have
  int maxNumBblsPerLoop; // The maximum number of basic blocks in a loop

  std::unique_ptr<symir::Funct> fun = nullptr; // The built function in SymIR
};


#endif // GRAPHFUZZ_FUNCTION_HPP
