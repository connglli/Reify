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

#ifndef REIFY_UBFEXEC_HPP
#define REIFY_UBFEXEC_HPP

#include <bitwuzla/cpp/bitwuzla.h>
#include "lib/function.hpp"
#include "lib/ubfree.hpp"

class FunPlus;

class UBFreeExec {
public:
  static const int PassCounterBblId;
  static const std::string PassCounterBblLabel;

public:
  explicit UBFreeExec(const FunPlus &fun, const std::vector<int> &execution);

  ~UBFreeExec();

  // Get the function generator, i.e., owner of this execution
  [[nodiscard]] const FunPlus *GetOwner() const { return owner; }

  // Get the function that this execution is for (We may modify our owner generated function)
  [[nodiscard]] const symir::Funct *GetFun() const { return fun; }

  // Get the execution path of the function
  [[nodiscard]] const std::vector<int> &GetExecution() const { return execution; }

  // Get all solved initializations
  [[nodiscard]] const std::vector<std::vector<ArgPlus<int>>> &GetInits() const { return inits; }

  // Get all solved finalizations
  [[nodiscard]] const std::vector<std::vector<ArgPlus<int>>> &GetFinas() const { return finas; }

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
  void extractSymbolsFromModel();

  /// Extract the solved value of the parameters from the model for the given version
  std::vector<ArgPlus<int>> extractParamsFromModel(int version);

  // Insert undefined behaviors into unexecuted basic blocks
  void insertUBsIntoUnexecutedBbls();

  // Insert random values into unsolved symbols
  void insertRandomValueIntoUnsolvedSymbols();

private:
  const FunPlus *owner;
  const symir::Funct *fun;

  // Execution path of the function
  std::vector<int> execution;
  std::vector<std::string> executionByLabels;

  // Constraints collector for signed overflow
  std::unique_ptr<UBSan> ubSan; // TODO: Add more UB types in the future

  // Bitwuzla solver for solving constraints
  std::unique_ptr<bitwuzla::Bitwuzla> solver;

  // The value of each input parameter at the entry of the function.
  std::vector<std::vector<ArgPlus<int>>> inits{};

  // The value of each input parameter at the exit of the function.
  std::vector<std::vector<ArgPlus<int>>> finas{};
};

#endif // REIFY_UBFEXEC_HPP
