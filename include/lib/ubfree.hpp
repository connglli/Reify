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

#include "global.hpp"
#include "lib/argument.hpp"
#include "lib/lang.hpp"
#include "lib/logger.hpp"
#include "lib/naming.hpp"
#include "z3++.h"

/// UBSan is a visitor that collects constraints to ensure that the
/// execution of a function is free of undefined behavior like overflow .
class UBSan : protected symir::SymIRVisitor {
public:
  UBSan(const symir::Funct &fun, std::vector<std::string> execution) :
      fun(fun), execution(std::move(execution)) {}

  // Get the collected constraints that ensures the execution to be UB-free over integer arithmetic
  [[nodiscard]] const z3::expr_vector &GetConstraints() const { return constraints; }

  // Get the Z3 context of us for collecting constraints
  [[nodiscard]] z3::context &GetContext() { return ctx; }

  // Set the enable or disable making coefficients interesting
  void EnableInterestCoefs(bool e) { enableInterestCoefs = e; }

  // Reset the visitor to its initial state
  void Reset() {
    constraints = z3::expr_vector(ctx);
    versions.clear();
    exprStack = std::stack<z3::expr>();
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
  z3::expr CreateScaExpr(const symir::VarDef *var, int version = -1);

  /// Return the name of the loc-th element of the vector variable
  std::string GetVecElName(const symir::VarDef *var, int loc) const;

  // Create a versioned variable expression for the loc-th element of the vector variable
  // When version==-1, the current version in the version table is used
  // When version==-2, the version in the version table is incremented and the new version is used
  z3::expr CreateVecElExpr(const symir::VarDef *var, int loc, int version = -1);

  // Create a coefficient expression for the given name
  z3::expr CreateCoefExpr(const symir::Coef &coef);

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
  // Push an expression to the expression stack
  void pushExpression(z3::expr expr) { exprStack.push(std::move(expr)); }

  // Pop an expression from the expression stack
  z3::expr popExpression() {
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

  // Generate constraints to make the coefficients interesting
  void makeCoefsInteresting(const std::vector<symir::Coef *> &coefs);

private:
  const symir::Funct &fun;            // The function that we're analyzing
  std::vector<std::string> execution; // The execution path of the function

  z3::context ctx;                  // The context of our constraint collecting
  z3::expr_vector constraints{ctx}; // The constraints generated by this visitor

  // Context used in collecting UB-free constraints
  std::stack<z3::expr> exprStack{}; // The expression stack for evaluating the SymIR program
  std::stack<int> vecElLocStack{};  // The element location stack for evaluating the SymIR program
  std::string prevBbl = "", currBbl = "",
              nextBbl = ""; // The previous/current/next blocks been/being/to-be evaluated
  std::map<std::string, int> versions{};        // The SSA version table for each variable
  std::map<std::string, std::string> verbbls{}; // The defined basic block for the current version

  // Flags controlling the behavior of the visitor
  bool enableInterestCoefs = true; // Whether to make coefficients interesting
};


#endif // REIFY_UBFREE_HPP
