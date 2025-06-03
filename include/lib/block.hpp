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


#ifndef GRAPHFUZZ_BLOCK_HPP
#define GRAPHFUZZ_BLOCK_HPP

#include <map>
#include <set>
#include <string>
#include <vector>
#include "global.hpp"
#include "lib/function.hpp"
#include "z3++.h"

typedef int VarIndex;

class Func;

class BB {

public:
  Func &f;
  int blockno;
  int numStatements;
  std::map<VarIndex, std::vector<VarIndex>> statementMappings;
  std::vector<VarIndex> assignmentOrder;
  std::vector<int> blockTargets;
  std::vector<VarIndex> conditionalVariables;
  // MOTIVATION: We need to generate a pass counter name for the block in case we
  // end up in a consistent walk with a loop which never reaches the end node
  bool needPassCounter = false;
  int passCounterValue = 0;

public:
  BB(Func &f, int blockno, const std::set<int> &graphTargets) :
      f(f), blockno(blockno), numStatements(ASSIGNMENTS_PER_BB),
      blockTargets(graphTargets.begin(), graphTargets.end()) {}

  void Generate();

  void setPassCounter(int value) {
    needPassCounter = true;
    passCounterValue = value;
  }

  ///////////////////////////////////////////////////////////////////
  /////// Constraint Generation
  ///////////////////////////////////////////////////////////////////

private:
  z3::expr createParameterExpr(std::string name, z3::context &ctx);

  z3::expr makeCoefficientsInteresting(const std::vector<z3::expr> &coeffs, z3::context &c);

  z3::expr boundCoefficients(z3::context &c, const std::vector<z3::expr> &coeffs);

public:
  void
  GenerateConstraints(int target, z3::solver &solver, z3::context &c, std::unordered_map<std::string, int> &versions);

  ///////////////////////////////////////////////////////////////////
  /////// Code Generation
  ///////////////////////////////////////////////////////////////////

private:
  std::string generateConditionalConstraint(int blockno, int target, std::vector<VarIndex> conditionalVariables) const;

  std::string generateUnconditionalGoto(int target) const;

public:
  std::string GenerateCode() const;
};

#endif // GRAPHFUZZ_BLOCK_HPP
