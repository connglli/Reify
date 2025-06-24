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

#ifndef GRAPHFUZZ_UBFEXEC_HPP
#define GRAPHFUZZ_UBFEXEC_HPP

#include "lib/funcplus.hpp"
#include "lib/ubfree.hpp"
#include "z3++.h"

class FunPlus;

class UBFreeExec {
public:
  explicit UBFreeExec(const FunPlus &fun, std::vector<int> execution);

  // Get the function generator
  [[nodiscard]] const FunPlus &GetFun() const { return fun; }

  // Get the execution path of the function
  [[nodiscard]] const std::vector<int> &GetExecution() const { return execution; }

  // Get all solved initializations
  [[nodiscard]] const std::vector<std::vector<int>> &GetInitializations() const {
    return initializations;
  }

  // Get all solved finalizations
  [[nodiscard]] const std::vector<std::vector<int>> &GetFinalizations() const {
    return finalizations;
  }

  // Check if we need a pass counter for the execution
  [[nodiscard]] bool NeedPassCounter() const { return passCounterBbl != -1; }

  // Get which basick block needs a pass counter
  [[nodiscard]] int GetPassCounterBbl() const { return passCounterBbl; }

  // Get the value of the pass counter
  [[nodiscard]] int GetPassCounter() const { return passCounterVal; }

  // Solve all symbols in the function to obtain an UB-free function with a number
  // of initialization-fianlization mapping. Return the number of obtained mapping.
  int Solve(
      // clang-format off
      int numMap,
      bool withInterestInit = true,
      bool withRandomInit = false,
      bool withInterestCoefs = true,
      bool debug = false
      // clang-format on
  );

private:
  // Detect unterminated execution and fix it to terminate
  void fixUnterminatedExecution();

  // Generate constraints, solve them, and instantiate resolved symbols.
  // For the very first call to this method, the constraints generated are purely
  // from scratch. However, this is a stateful call. That means, all subsequent calls
  // although always generate the same as query, the solver would use the symbols
  // we already resolved in the previous calls to this method.
  bool solve(
      // clang-format off
      bool withInterestInit,
      bool withRandomInit,
      bool withInterestCoefs,
      bool debug
      // clang-format on
  );

  // Function to extract all symbols from the model, so that we can instantiate
  // all the symbols that the solver successfully found an interpretation for
  void extractSymbolsFromModel(z3::model &model);

  // Extract the initialization values from the model and save it
  void extractInitFromModel(z3::model &model);

  // Extract the finalization values from the model and save it
  void extractFinalFromModel(z3::model &model);

  // Insert undefined behaviors into unexecuted basic blocks
  void insertUBsIntoUnexecutedBbls();

private:
  const FunPlus &fun;

  // Execution path of the function
  std::vector<int> execution;

  // Constraints collector for signed overflow
  std::unique_ptr<SignedOverflow> ubSov; // TODO: Add more UB types in the future

  // Whether we need a pass counter for the execution.
  // This help ensure we reach the end of a function for unterminated executions.
  int passCounterBbl = -1; // -1 means we do not need any pass counter
  int passCounterVal = -1; // The value of the pass counter

  // The value of each input parameter at the entry of the function.
  std::vector<std::vector<int>> initializations{};

  // The value of each input parameter at the exit of the function.
  std::vector<std::vector<int>> finalizations{};
};

#endif // GRAPHFUZZ_UBFEXEC_HPP
