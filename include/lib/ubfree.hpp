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

#ifndef REIFY_UBFREE_HPP
#define REIFY_UBFREE_HPP

#include <algorithm>
#include <bitwuzla/cpp/bitwuzla.h>
#include <unordered_set>

#include <memory>
#include "global.hpp"
#include "lib/argument.hpp"
#include "lib/lang.hpp"
#include "lib/logger.hpp"
#include "lib/naming.hpp"

/// UBSan is a visitor that collects constraints to ensure that the
/// execution of a function is free of undefined behavior like overflow .
class UBSan : protected symir::SymIRVisitor {
public:
  struct Stats {
    size_t assertedConstraints = 0;
    size_t uniqueConstraintIds = 0;
    size_t uniqueBoundedIds = 0;

    uint64_t addCalls = 0;
    uint64_t skippedTrue = 0;
    uint64_t deduped = 0;
    uint64_t ensureInRangeCalls = 0;
    uint64_t boundedAlready = 0;
  };

  UBSan(const symir::Funct &fun, std::vector<std::string> execution) :
      fun(fun), execution(std::move(execution)), tm(std::make_unique<bitwuzla::TermManager>()),
      coefTerms(), paramTerms() {
    bvSort = tm->mk_bv_sort(32); // 32-bit for normal integer operations
  }

  // Get the collected constraints that ensures the execution to be UB-free over integer arithmetic
  [[nodiscard]] const std::vector<bitwuzla::Term> &GetConstraints() const { return constraints; }

  // Get the Bitwuzla term manager for collecting constraints
  [[nodiscard]] bitwuzla::TermManager &GetTermManager() { return *tm; }

  // Set the enable or disable making coefficients interesting
  void EnableInterestCoefs(bool e) { enableInterestCoefs = e; }

  // Reset the visitor to its initial state
  void Reset() {
    constraints.clear();
    constraintIds.clear();
    boundedIds.clear();
    addCalls_ = 0;
    skippedTrue_ = 0;
    deduped_ = 0;
    ensureInRangeCalls_ = 0;
    boundedAlready_ = 0;
    versions.clear();
    // DON'T clear coefTerms - coefficients must persist across resets!
    // coefTerms.clear();
    exprStack = std::stack<bitwuzla::Term>();
    suffixStack = std::stack<std::string>();
    prevBbl = "___entry_bbl";
    currBbl = "";
    nextBbl = "";
  }

  // Collect constraints that can make the execution UB free
  void Collect() { fun.Accept(*this); }

  // Optimize the set of collected constraints to obtain an optimized set
  void Optimize();

  // Create a versioned variable expression for the given scalar variable
  // When version==-1, the current version in the version table is used
  // When version==-2, the version in the version table is incremented and the new version is
  // used
  bitwuzla::Term CreateScaExpr(const symir::VarDef *var, int version = -1);

  /// Return the name of the loc-th element of the vector variable
  std::string GetVecElName(const symir::VarDef *var, int loc) const;

  // Create a versioned variable expression for the loc-th element of the vector variable
  // When version==-1, the current version in the version table is used
  // When version==-2, the version in the version table is incremented and the new version is used
  bitwuzla::Term CreateVecElExpr(const symir::VarDef *var, int loc, int version = -1);

  /// Return the name of the field of the struct variable
  std::string GetStructFieldName(const symir::VarDef *var, const std::string &field) const;

  // Create a versioned variable expression for the field of the struct variable
  // When version==-1, the current version in the version table is used
  // When version==-2, the version in the version table is incremented and the new version is used
  bitwuzla::Term
  CreateStructFieldExpr(const symir::VarDef *var, const std::string &field, int version = -1);

  // Create a versioned variable expression using a suffix
  bitwuzla::Term
  CreateVersionedExpr(const symir::VarDef *var, const std::string &suffix, int version = -1);

  // Create a coefficient expression for the given name
  bitwuzla::Term CreateCoefExpr(const symir::Coef &coef);

  // Generate constraints for making the initialization interesting
  void MakeInitInteresting();

  // Generate constraints so that the initialization has random values
  void MakeInitWithRandomValue();

  // Generate constraints to differentiate the initialization of the parameters from the given one
  void MakeInitDifferentFrom(const std::vector<ArgPlus<int>> &init);

  // Print all constraints to the logger
  void PrintConstraints() const {
    for (const auto &c: constraints) {
      Log::Get().Out() << c << std::endl;
    }
  }

  // Number of collected constraints.
  [[nodiscard]] size_t NumConstraints() const { return constraints.size(); }

  [[nodiscard]] Stats GetStats() const;

protected:
  void Visit(const symir::VarUse &v) override;
  void Visit(const symir::Coef &c) override;
  void Visit(const symir::Term &t) override;
  void Visit(const symir::Expr &e) override;
  void Visit(const symir::Cond &c) override;
  void Visit(const symir::AssStmt &a) override;
  void Visit(const symir::RetStmt &r) override;
  void Visit(const symir::Branch &b) override;
  void Visit(const symir::Goto &g) override;
  void Visit(const symir::ScaParam &p) override;
  void Visit(const symir::VecParam &p) override;
  void Visit(const symir::StructParam &p) override;
  void Visit(const symir::ScaLocal &l) override;
  void Visit(const symir::VecLocal &l) override;
  void Visit(const symir::StructLocal &l) override;
  void Visit(const symir::StructDef &s) override;
  void Visit(const symir::Block &b) override;
  void Visit(const symir::Funct &f) override;

private:
  // Add a constraint unless it's trivially true or already added.
  void addConstraint(const bitwuzla::Term &c);

  // Ensure a term is within global bounds (asserted at most once per term).
  void ensureInRange(const bitwuzla::Term &t);

  // Push an expression to the expression stack
  void pushExpression(bitwuzla::Term expr) { exprStack.push(std::move(expr)); }

  // Pop an expression from the expression stack
  bitwuzla::Term popExpression() {
    auto e = exprStack.top();
    exprStack.pop();
    return e;
  }

  // Push an element location to the element location stack
  void pushVecElLoc(int loc) { vecElLocStack.push(loc); }

  // Pop an element location to the element location stack
  int popVecElLoc() {
    auto loc = vecElLocStack.top();
    vecElLocStack.pop();
    return loc;
  }

  // Push a suffix to the suffix stack
  void pushSuffix(std::string s) { suffixStack.push(std::move(s)); }

  // Pop a suffix from the suffix stack
  std::string popSuffix() {
    auto s = suffixStack.top();
    suffixStack.pop();
    return s;
  }

  // Generate constraints to make the coefficients interesting
  void makeCoefsInteresting(const std::vector<symir::Coef *> &coefs);

private:
  const symir::Funct &fun;            // The function that we're analyzing
  std::vector<std::string> execution; // The execution path of the function

  std::unique_ptr<bitwuzla::TermManager> tm; // The term manager for our constraint collecting
  std::vector<bitwuzla::Term> constraints;   // The constraints generated by this visitor
  bitwuzla::Sort bvSort;                     // The bitvector sort for 32-bit integers

  // Dedup helpers
  std::unordered_set<uint64_t> constraintIds{};
  std::unordered_set<uint64_t> boundedIds{};

  // Lightweight instrumentation counters
  uint64_t addCalls_ = 0;
  uint64_t skippedTrue_ = 0;
  uint64_t deduped_ = 0;
  uint64_t ensureInRangeCalls_ = 0;
  uint64_t boundedAlready_ = 0;

  // Context used in collecting UB-free constraints
  std::stack<bitwuzla::Term> exprStack{}; // The expression stack for evaluating the SymIR program
  std::stack<int> vecElLocStack{}; // The element location stack for evaluating the SymIR program
  std::stack<std::string> suffixStack{}; // The suffix stack for evaluating variable access
  std::string prevBbl = "", currBbl = "",
              nextBbl = ""; // The previous/current/next blocks been/being/to-be evaluated
  std::map<std::string, int> versions{};        // The SSA version table for each variable
  std::map<std::string, std::string> verbbls{}; // The defined basic block for the current version

  // Term caches to ensure Bitwuzla terms are reused (Bitwuzla doesn't unify by name!)
  std::map<std::string, bitwuzla::Term> coefTerms{};  // Cache of coefficient terms
  std::map<std::string, bitwuzla::Term> paramTerms{}; // Cache of parameter terms (version 0 only)

  // Flags controlling the behavior of the visitor
  bool enableInterestCoefs = true; // Whether to make coefficients interesting
};


#endif // REIFY_UBFREE_HPP
