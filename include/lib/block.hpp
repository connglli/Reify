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
#include <string>
#include <vector>

#include "lib/ctrlflow.hpp"
#include "lib/function.hpp"

class FunGen;
class UBFreeExec;

/// BlkGen is a basic block generator which populates each basic block with real statements
class BlkGen {

public:
  BlkGen(FunGen &fun, int bblNo, BblSketch bblSkt) :
      fun(fun), bblNo(bblNo), bblSkt(std::move(bblSkt)) {}

  // Get the ID of the basic block
  [[nodiscard]] int GetBblNo() const { return bblNo; }

  // Get the underlying basic block
  [[nodiscard]] const BblSketch &GetBblSketch() const { return bblSkt; }

  // Get all the assigned variables in this basic block in order
  [[nodiscard]] const std::vector<int> &GetDefs() const { return assignments; }

  // Get the dependencies of an assigned variable
  [[nodiscard]] const std::vector<int> &GetDeps(int def) const {
    Assert(defUsedVars.contains(def), "Variable %d is not defined in this block", def);
    return defUsedVars.find(def)->second;
  }

  // Get the dependencies of the conditional
  [[nodiscard]] const std::vector<int> &GetCondDeps() const { return condUsedVars; }

  // Get the successors of this basic block
  [[nodiscard]] const std::vector<int> &GetSuccessors() const { return bblSkt.GetSuccessors(); }

  // Generate the basic block with random statements and symbols
  void Generate();

  // Generate the code of this basic block for a given execution
  [[nodiscard]] std::string GenerateCode(const UBFreeExec &exec) const;

private:
  [[nodiscard]] std::string generateCondCode(const UBFreeExec &exec, int successor) const;

  [[nodiscard]] std::string generateUncondCode(int successor) const;

private:
  FunGen &fun;      // The residing function of this basic block
  int bblNo;        // The identifier of this basic block
  BblSketch bblSkt; // The sketch of this basic block

  std::vector<int> assignments{}; // The order of the definition of each variable
  std::map<int, std::vector<int>> defUsedVars{
  }; // The list of used variables for each defined variables
  std::vector<int> condUsedVars{}; // The list of used variables for the conditional
};

#endif // GRAPHFUZZ_BLOCK_HPP
