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
#include <z3++.h>

#include "lib/block.hpp"
#include "lib/ctrlflow.hpp"
#include "lib/dbgutils.hpp"

class BlkGen;

/// FunGen is a function generator which populates each the whole function with real statements
class FunGen {
public:
  explicit FunGen(int numBBs, int numVars, int maxNumLoops, int maxNumBblsPerLoop) :
      cfg(numBBs), numVars(numVars), maxNumLoops(maxNumLoops),
      maxNumBblsPerLoop(maxNumBblsPerLoop) {}

  FunGen(const FunGen &) = delete;
  FunGen &operator=(const FunGen &) = delete;

  [[nodiscard]] const auto &GetCfg() const { return cfg; }

  [[nodiscard]] int NumBbls() const { return cfg.NumBbls(); }

  [[nodiscard]] int NumVars() const { return numVars; }

  [[nodiscard]] const auto &GetBbls() const { return bblGens; }

  void Generate();

  [[nodiscard]] std::vector<int> SampleExec(int execStep, bool consistent);

  ///////////////////////////////////////////////////////////////////
  /////// Constraint Initialization and Finalization
  ///////////////////////////////////////////////////////////////////

  [[nodiscard]] bool ParamDefined(const std::string &name) {
    return state.find(name) != state.end() && state[name].has_value();
  }

  [[nodiscard]] int GetParamVal(const std::string &name) {
    Assert(ParamDefined(name), "Parameter %s not defined", name.c_str());
    return state[name].value();
  }

  void InitParam(const std::string &name) { state[name] = std::nullopt; }

  void DefineParam(const std::string &name, int val) { state[name] = val; }

  z3::expr MakeInitialisationsInteresting(z3::context &c) const;

  z3::expr AddRandomInitialisations(z3::context &c);

  z3::expr DifferentInitialisationConstraint(std::vector<int> initialisation, z3::context &c) const;

  // Function to extract parameters from the model, so that we can store the
  // coefficients and constants that the solver found an interpretation for
  void ExtractParametersFromModel(z3::model &model, z3::context &ctx);

  // Extract the initial values of each variable that's an input to the function from the model
  std::vector<int> ExtractInitialisationsFromModel(z3::model &model, z3::context &ctx) const;

  // i wanted a bit of flexibility for the checksum function in the future so i
  // decided to just store the final values of all the variables (at the end basic
  // block) instead of just the checksum
  std::vector<int> ExtractFinalizationsFromModel(
      z3::model &model, z3::context &ctx, std::unordered_map<std::string, int> &versions
  ) const;

  ///////////////////////////////////////////////////////////////////
  /////// Code Generation
  ///////////////////////////////////////////////////////////////////

  std::string GenerateFunCode(const std::string &sno, const std::string &uuid) const;

  std::string GenerateMainCode(
      const std::string &sno, const std::string &uuid,
      const std::vector<std::vector<int>> &initialisations,
      const std::vector<std::vector<int>> &finalizations, bool debug = false
  ) const;

  static std::string
  GenerateMapping(const std::vector<int> &initialisation, const std::vector<int> &finalisation);

private:
  CfgSketch cfg; // The sketch of our CFG, where each bbl maps to a blockgen in bbs

  int numVars;           // The number of variables that the function can define
  int maxNumLoops;       // The maximum number of loops that the function can have
  int maxNumBblsPerLoop; // The maximum number of basic blocks in a loop

  std::vector<BlkGen> bblGens{}; // The basic block generators (in the same order as basic blocks)

  std::unordered_map<std::string, std::optional<int>> state{
  }; // Value of variables, coefficients or constants computed so far
};


#endif // GRAPHFUZZ_FUNCTION_HPP
