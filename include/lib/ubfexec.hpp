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

#include "lib/function.hpp"
#include "lib/naming.hpp"
#include "z3++.h"

class BlkGen;
class FunGen;

class UBFreeExec {
public:
  explicit UBFreeExec(const FunGen &fun, std::vector<int> execution) :
      fun(fun), execution(std::move(execution)) {
    fixUnterminatedExecution();
  }

  // Get the function generator
  [[nodiscard]] const FunGen &GetFun() const { return fun; }

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

  // Check if a symbol is defined
  [[nodiscard]] bool SymDefined(const std::string &name) const {
    return symbols.contains(name) && symbols.find(name)->second.has_value();
  }

  // Get the value of a symbol
  [[nodiscard]] int GetSymVal(const std::string &name) const {
    Assert(SymDefined(name), "Symbol %s not defined", name.c_str());
    return symbols.find(name)->second.value();
  }

  // Initialize a symbol with an undefined value
  void InitSym(const std::string &name) { symbols[name] = std::nullopt; }

  // Define a symbol with a specific value
  void DefineSym(const std::string &name, int val) { symbols[name] = val; }

  // Check if we need a pass counter for the execution
  [[nodiscard]] bool NeedPassCounter() const { return passCounterBbl != -1; }

  // Get which basick block needs a pass counter
  [[nodiscard]] int GetPassCounterBbl() const { return passCounterBbl; }

  // Get the value of the pass counter
  [[nodiscard]] int GetPassCounter() const { return GetSymVal(NamePassCounter()); }

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

  // Generate constraints for the basic block at u and target the basic block at v
  void generateConstraintsForBasicBlock(int u, int v, bool withInterestCoefs);

  // Generate constraints for the term sum of the coefficients and variables
  z3::expr generateConstraintsForTermSum(
      // clang-format off
      int blkIndex, int stmtIndex, bool isInCond,
      const std::vector<int> &dependencies,
      bool withInterestCoefs
      // clang-format on
  );

  // Function to extract all symbols from the model, so that we can instantiate
  // all the symbols that the solver successfully found an interpretation for
  void extractSymbolsFromModel(z3::model &model);

  // Extract the initialization values from the model and save it
  void extractInitFromModel(z3::model &model);

  // Extract the finalization values from the model and save it
  void extractFinalFromModel(z3::model &model);

  // Generate constraints for making the initialization interesting
  z3::expr makeInitInteresting();

  // Generate constraints so that the initialization has random values
  z3::expr makeInitWithRandomValue();

  // Generate constraints to differentiate the initialization from the given one
  z3::expr differentiateInitFrom(std::vector<int> initialisation);

  // Create a versioned variable expression for the given variable
  // When version==-1, the current version in the version table is used
  z3::expr createVarExpr(int varIndex, int version = -1);

  // Generate a bound constraint for each variable
  z3::expr boundVariables(const std::vector<z3::expr> &vars);

  // Create a coefficient expression for the given name
  z3::expr createCoefExpr(const std::string &name);

  // Generate constraints to make the coefficients interesting
  z3::expr makeCoefsInteresting(const std::vector<z3::expr> &coefs);

  // Generate a bound constraint for each coefficient
  z3::expr boundCoefficients(const std::vector<z3::expr> &coefs);

  // Generate a bound constraint for each term
  z3::expr boundTerms(const std::vector<z3::expr> &terms);

  // Insert undefined behaviors into unexecuted basic blocks
  void insertUBsIntoUnexecutedBbls();

private:
  const FunGen &fun;

  // Execution path of the function
  std::vector<int> execution{};

  // Whether we need a pass counter for the execution.
  // This help ensure we reach the end of a function for unterminated executions.
  int passCounterBbl = -1; // -1 means we do not need any pass counter

  // Name and value of each coefficient (including constants) when executing
  // the function following the above execution. Their values ensure that
  // the execution of the function is UB-free given any of the following
  // initialization as input arguments.
  std::unordered_map<std::string, std::optional<int>> symbols{};

  // The value of each input parameter at the entry of the function.
  std::vector<std::vector<int>> initializations{};

  // The value of each input parameter at the exit of the function.
  std::vector<std::vector<int>> finalizations{};

  // The SSA version table for each parameter (and variable in future).
  std::map<int, int> versions{};

  // Our SMT solver for symbol resolution
  z3::context ctx = z3::context();
  z3::solver solver = z3::solver(ctx);
};

#endif // GRAPHFUZZ_UBFEXEC_HPP
