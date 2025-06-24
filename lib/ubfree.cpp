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

#include "lib/ubfree.hpp"

#include "lib/samputils.hpp"
#include "lib/z3utils.hpp"

void SignedOverflow::Optimize() {
  // Accumulate all our assertions obtained so far
  z3::goal goal(ctx);
  for (auto c: constraints) {
    goal.add(c);
  }

  // Create a chain of optimization passes (or tactics in Z3) for constraint optimization
  // Note, we only consider tactics that can preserve constant (i.e., symbols) here as
  // we'll extract them later. So we should exclude for example solve-eqs.
  z3::tactic opt =
      z3::repeat(z3::tactic(ctx, "simplify") & z3::tactic(ctx, "propagate-values"), /*max=*/3);

  // Apply all the assertions on our optimization chain to obtain a new set of constraints
  auto optimized = opt(goal);
  // All tactics we used generate one subgoal
  Assert(optimized.size() == 1u, "More than one (=%u) subgoals are generated!", optimized.size());

  // Re-add all assertions after optimization into our solver
  constraints = z3::expr_vector(ctx);
  for (int i = 0; i < static_cast<int>(optimized[0].size()); i++) {
    constraints.push_back(optimized[0][i]);
  }
}

z3::expr SignedOverflow::CreateVarExpr(const symir::VarDef *var, int version) {
  if (version == -1) {
    version = versions[var->GetName()];
  }
  return ctx.int_const((var->GetName() + "_" + std::to_string(version)).c_str());
}

z3::expr SignedOverflow::CreateCoefExpr(const symir::Coef &coef) {
  // If we've already extracted the value from the model, we use that value,
  // otherwise it's an uninterpreted constant which the solver needs to figure
  // out the value for
  if (coef.IsSolved()) {
    return ctx.int_val(std::stoi(coef.GetValue()));
  } else {
    return ctx.int_const(coef.GetName().c_str());
  }
}

void SignedOverflow::MakeInitInteresting() {
  std::vector<z3::expr> params;
  for (auto param: fun.GetParams()) {
    params.push_back(CreateVarExpr(param, 0));
  }
  constraints.push_back(AtMostKZeroes(ctx, params, fun.NumParams() / 2));
}

void SignedOverflow::MakeInitWithRandomValue() {
  auto rand = Random::Get().Uniform(
      GlobalOptions::Get().LowerInitBound, GlobalOptions::Get().UpperInitBound
  );
  for (auto param: fun.GetParams()) {
    constraints.push_back(CreateVarExpr(param, 0) == rand());
  }
}

void SignedOverflow::MakeInitDifferentFrom(const std::vector<int> &init) {
  // There should be at lease NUM_VAR/2 variables which are not equal in both initialisations
  std::vector<z3::expr> params;
  for (int i = 0; i < fun.NumParams(); i++) {
    int oldValue = init[i];
    z3::expr newValue = CreateVarExpr(fun.GetParams()[i], 0);
    params.push_back(z3::ite(oldValue != newValue, ctx.int_val(1), ctx.int_val(0)));
  }
  constraints.push_back(AtMostKZeroes(ctx, params, std::min(3, fun.NumParams() / 2)));
}

void SignedOverflow::Visit(const symir::VarUse &v) {
  auto varExpr = CreateVarExpr(v.GetDef());
  constraints.push_back(varExpr >= GlobalOptions::Get().LowerBound);
  constraints.push_back(varExpr <= GlobalOptions::Get().UpperBound);
  pushExpression(varExpr);
}

void SignedOverflow::Visit(const symir::Coef &c) {
  auto coefExpr = CreateCoefExpr(c);
  constraints.push_back(coefExpr >= GlobalOptions::Get().LowerCoefBound);
  constraints.push_back(coefExpr <= GlobalOptions::Get().UpperCoefBound);
  pushExpression(coefExpr);
}

void SignedOverflow::Visit(const symir::Term &t) {
  t.GetCoef()->Accept(*this);
  z3::expr coefExpr = popExpression();

  z3::expr *varExpr = nullptr;
  if (t.GetOp() != symir::Term::Op::OP_CST) {
    t.GetVar()->Accept(*this);
    varExpr = new z3::expr(popExpression());
  }

  z3::expr *termExpr = nullptr;

  switch (t.GetOp()) {
    case symir::Term::Op::OP_ADD:
      termExpr = new z3::expr(coefExpr + *varExpr);
      break;
    case symir::Term::Op::OP_SUB:
      termExpr = new z3::expr(coefExpr - *varExpr);
      break;
    case symir::Term::Op::OP_MUL:
      termExpr = new z3::expr(coefExpr * *varExpr);
      break;
    case symir::Term::Op::OP_DIV:
      constraints.push_back(*varExpr != 0);
      termExpr = new z3::expr(z3::cxx_idiv(coefExpr, *varExpr));
      break;
    case symir::Term::Op::OP_REM:
      constraints.push_back(*varExpr != 0);
      termExpr = new z3::expr(z3::cxx_irem(coefExpr, *varExpr));
      break;
    case symir::Term::Op::OP_CST:
      termExpr = new z3::expr(coefExpr);
      break;
    default:
      Panic("Cannot reacher here");
  }

  constraints.push_back(*termExpr >= GlobalOptions::Get().LowerBound);
  constraints.push_back(*termExpr <= GlobalOptions::Get().UpperBound);

  pushExpression(*termExpr);

  if (varExpr != nullptr) {
    delete varExpr;
  }
  delete termExpr;
}

void SignedOverflow::Visit(const symir::Expr &e) {
  auto result = ctx.int_val(0);
  std::vector<symir::Coef *> coefs;
  for (int i = 0; i < e.NumTerms(); i++) {
    coefs.push_back(e.GetTerm(i)->GetCoef());
    e.GetTerm(i)->Accept(*this);
    auto termExpr = popExpression();
    if (i == 0) {
      result = termExpr;
      continue;
    }
    switch (e.GetOp()) {
      case symir::Expr::Op::OP_ADD:
        result = result + termExpr;
        break;
      case symir::Expr::Op::OP_SUB:
        result = result - termExpr;
        break;
      default:
        Panic("Cannot reacher here");
    }
    constraints.push_back(result >= GlobalOptions::Get().LowerBound);
    constraints.push_back(result <= GlobalOptions::Get().UpperBound);
  }
  if (enableInterestCoefs) {
    makeCoefsInteresting(e);
  }
  pushExpression(result);
}

void SignedOverflow::Visit(const symir::Cond &c) {
  c.GetExpr()->Accept(*this);
  auto expr = popExpression();
  switch (c.GetOp()) {
    {
      case symir::Cond::Op::OP_GTZ:
        pushExpression(expr > 0);
        break;
      case symir::Cond::Op::OP_LTZ:
        pushExpression(expr < 0);
        break;
      case symir::Cond::Op::OP_EQZ:
        pushExpression(expr == 0);
        break;
      default:
        Panic("Cannot reacher here");
    }
  }
}

void SignedOverflow::Visit(const symir::AssStmt &a) {
  a.GetExpr()->Accept(*this);
  auto exprExpr = popExpression();
  auto varNewVerExpr = CreateVarExpr(a.GetVar()->GetDef(), ++versions[a.GetVar()->GetName()]);
  constraints.push_back(varNewVerExpr == exprExpr);
}

void SignedOverflow::Visit(const symir::RetStmt &r) { /* DO NOTHING */ }

void SignedOverflow::Visit(const symir::Branch &b) {
  b.GetCond()->Accept(*this);
  auto condExpr = popExpression();

  // This means we are the last executed basic block
  if (nextBbl == "") {
    return;
  }

  if (b.GetTrueTarget() == nextBbl) {
    constraints.push_back(condExpr);
  } else if (b.GetFalseTarget() == nextBbl) {
    constraints.push_back(!condExpr);
  } else if (nextBbl == execution.back()) {
    // This means we have to stop earlier
    return;
  } else {
    Panic(
        "The branch (true: \"%s\", false: \"%s\") does not have targets called \"%s\"",
        b.GetTrueTarget().c_str(), b.GetFalseTarget().c_str(), nextBbl.c_str()
    );
  }
}

void SignedOverflow::Visit(const symir::Goto &g) { /* DO NOTHING */ }

void SignedOverflow::Visit(const symir::Param &p) { Panic("Cannot reach here"); }

void SignedOverflow::Visit(const symir::Local &l) { Panic("We currently does not support Locals"); }

void SignedOverflow::Visit(const symir::Block &b) {
  for (const auto &stmt: b.GetStmts()) {
    stmt->Accept(*this);
  }
}

void SignedOverflow::Visit(const symir::Func &f) {
  Assert(
      exprStack.empty(), "Expression stack is not empty before visiting function %s",
      f.GetName().c_str()
  );

  for (int i = 0; i < execution.size() - 1; i++) {
    currBbl = execution[i];
    nextBbl = execution[i + 1];
    f.GetBlock(currBbl)->Accept(*this);
  }
  currBbl = execution.back();
  nextBbl = "";
  f.GetBlock(currBbl)->Accept(*this);

  Assert(
      exprStack.empty(), "Expression stack is not empty after visiting function %s",
      f.GetName().c_str()
  );
}

void SignedOverflow::makeCoefsInteresting(const symir::Expr &expr) {
  std::vector<z3::expr> coefs;
  for (const auto term: expr.GetTerms()) {
    coefs.push_back(CreateCoefExpr(*term->GetCoef()));
  }
  constraints.push_back(AtMostKZeroes(ctx, coefs, static_cast<int>(coefs.size()) / 2));
}
