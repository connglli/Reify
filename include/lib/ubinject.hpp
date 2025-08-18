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

#ifndef REIFY_UBINJECT_HPP
#define REIFY_UBINJECT_HPP

#include "lib/lang.hpp"
#include "lib/logger.hpp"
#include "z3++.h"

/// IntUBInject injects int-related undefined behaviors (UBs) into a given block of a given program.
/// It leverages bitvec theory to collect constraints.
class IntUBInject : protected symir::SymIRVisitor {
public:
  IntUBInject() :
      ctx(std::make_unique<z3::context>()), solver(std::make_unique<z3::solver>(*ctx)),
      constraints(std::make_unique<z3::expr_vector>(*ctx)) {}

  // Get the collected constraints that ensures the execution to be UB-free over integer arithmetic
  [[nodiscard]] const z3::expr_vector &GetConstraints() const { return *constraints; }

  // Get the Z3 context of us for collecting constraints
  [[nodiscard]] z3::context &GetContext() { return *ctx; }

  // Inject undefined behavior into the given block of the given function
  // Return a new function with the injected UBs if successful, otherwise return nullptr.
  // Our goal is to ensure that the function is UB-definitive and also makes the compiler realize
  // the UBs as early as possible. So instead of running a function-level abstract interpretation,
  // we run it in the block level. We ensure one of the statement in a block is UB-definitive and
  // inserts some initializations (i.e., assigning some initial values to the used variables) before
  // running the block.
  std::unique_ptr<symir::Funct> InjectUBs(const symir::Funct *f, const symir::Block *b);

  // Reset the visitor to its initial state
  virtual void Reset() {
    solver = nullptr;      // Delete the old solver
    constraints = nullptr; // Delete the old constraints
    ctx = std::make_unique<z3::context>();
    solver = std::make_unique<z3::solver>(*ctx);
    constraints = std::make_unique<z3::expr_vector>(*ctx);
    ubConstraints.clear();
    Assert(
        exprStack.empty(),
        "The expression stack is not empty when resetting the UB injector: %zu elements left",
        exprStack.size()
    );
    versions.clear();
  }

  // Create a versioned variable expression for the given variable
  // When version==-1, the current version in the version table is used
  z3::expr CreateVarExpr(const symir::VarDef *var, int version = -1);

  // Create a coefficient expression for the given name
  z3::expr CreateCoefExpr(const symir::Coef &coef);

protected:
  // Push an expression to the expression stack
  void pushExpression(z3::expr expr) { exprStack.push(std::move(expr)); }

  // Pop an expression from the expression stack
  z3::expr popExpression() {
    auto e = exprStack.top();
    exprStack.pop();
    return e;
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
  void Visit(const symir::Param &p) override;
  void Visit(const symir::Local &l) override;
  void Visit(const symir::Block &b) override;
  void Visit(const symir::Funct &f) override;

private:
  z3::expr mapBitVecExprToIntExpr(const z3::expr &bv);
  void ensureValidInitsForUses(const symir::Block *b);
  void extractAndInitializeUses(
      const z3::model &model, const symir::Block *b, symir::FunctBuilder *funBd,
      symir::BlockBuilder *blkBd
  );
  void
  extractSymbolsFromModel(const z3::model &model, const symir::Funct *f, const symir::Block *b);

  std::string nextCoefNameForBlock(const std::string &label) {
    static int counter = 0;
    return "intubinj_blk" + label + "_coefval" + std::to_string(counter++);
  }

private:
  std::unique_ptr<z3::context> ctx;   // The context of our constraint collecting
  std::unique_ptr<z3::solver> solver; // The solver for checking the constraints

  std::unique_ptr<z3::expr_vector> constraints; // The constraints generated by this visitor
  std::vector<z3::expr> ubConstraints{};        // The UB-specific constraints that ensure UBs

  // Context used in collecting UB-free constraints
  std::stack<z3::expr> exprStack{};      // The expression stack for evaluating the SymIR program
  std::map<std::string, int> versions{}; // The SSA version table for each variable
};

#endif // REIFY_UBINJECT_HPP
