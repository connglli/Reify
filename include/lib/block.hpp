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
#include "lib/function.hpp"
#include "lib/graphplus.hpp"
#include "z3++.h"

class FunGen;

class BlkGen {

public:
  BlkGen(FunGen &f, int bblNo, BblSketch bblSkt) : f(f), bblNo(bblNo), bblSkt(std::move(bblSkt)) {}

  void Generate();

  void SetPassCounter(int value) {
    needPassCounter = true;
    passCounterValue = value;
  }

  ///////////////////////////////////////////////////////////////////
  /////// Constraint Generation
  ///////////////////////////////////////////////////////////////////

public:
  void GenerateConstraints(
      int target, z3::solver &solver, z3::context &c, std::unordered_map<std::string, int> &versions
  );

private:
  z3::expr createParameterExpr(std::string name, z3::context &ctx);

  z3::expr makeCoefficientsInteresting(const std::vector<z3::expr> &coeffs, z3::context &c);

  z3::expr boundCoefficients(z3::context &c, const std::vector<z3::expr> &coeffs);

  ///////////////////////////////////////////////////////////////////
  /////// Code Generation
  ///////////////////////////////////////////////////////////////////

public:
  std::string GenerateCode() const;

private:
  std::string generateConditionalConstraint(
      int blockno, int target, std::vector<int> conditionalVariables
  ) const;

  std::string generateUnconditionalGoto(int target) const;

private:
  FunGen &f;        // The residing function of this basic block
  int bblNo;        // The identifier of this basic block
  BblSketch bblSkt; // The sketch of this basic block

  std::vector<int> assignmentOrder; // The order of the definition of each variable
  std::map<int, std::vector<int>>
      defUsedVars;               // The list of used variables for each defined variables
  std::vector<int> condUsedVars; // The list of used variables for the conditional

  // MOTIVATION: We need to generate a pass counter name for the block in case we
  // end up in a consistent walk with a loop which never reaches the end node
  bool needPassCounter = false;
  int passCounterValue = 0;
};

#endif // GRAPHFUZZ_BLOCK_HPP
