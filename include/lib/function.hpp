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

#include <cassert>
#include <optional>
#include <string>
#include <vector>
#include "lib/block.hpp"
#include "lib/graph.hpp"
#include "z3++.h"

class BB;

class Func {
public:
  explicit Func(int numBBs, int numVars) : numBBs(numBBs), g(numBBs), numVars(numVars) {}

  const auto &GetUdlyGraph() const { return g; }

  const int GetNumBBs() const { return numBBs; }

  const int NumVars() const { return numVars; }

  const auto &GetBBs() const { return bbs; }

  void GenCFG();

  const std::vector<int> SampleExec(int execStep, bool consistent);

  ///////////////////////////////////////////////////////////////////
  /////// Constraint Initialization and Finalization
  ///////////////////////////////////////////////////////////////////

  const bool ParamDefined(const std::string &name) {
    return parameters.find(name) != parameters.end() && parameters[name].has_value();
  }

  const int GetParamVal(const std::string &name) {
    assert(ParamDefined(name) && "Parameter not defined");
    return parameters[name].value();
  }

  void InitParam(const std::string &name) { parameters[name] = std::nullopt; }

  void DefineParam(const std::string &name, int val) { parameters[name] = val; }

  z3::expr MakeInitialisationsInteresting(z3::context &c) const;

  z3::expr AddRandomInitialisations(z3::context &c);

  z3::expr DifferentInitialisationConstraint(std::vector<int> initialisation, z3::context &c);

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

  std::string GenerateCode(int sno, std::string uuid);

private:
  int numBBs;          // The number of basic blocks the function has
  Graph g;             // Nodes in g maps to that in bbs one-by-one
  std::vector<BB> bbs; // The basic blocks (in the same order as g)

  // TODO: Move parameters to each basic block
  int numVars; // The number of variables that the function can define
  std::unordered_map<std::string, std::optional<int>>
      parameters; // Value of variables, coefficients or constants computed so far
};


#endif // GRAPHFUZZ_FUNCTION_HPP
