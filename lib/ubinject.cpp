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

#include "lib/ubinject.hpp"

#include <memory>
#include "global.hpp"
#include "lib/z3utils.hpp"

z3::expr IntUBInject::CreateScaExpr(const symir::VarDef *var, int version) {
  Assert(var != nullptr, "Cannot create a variable expression for a nullptr variable");
  const auto &varName = var->GetName();
  Assert(
      var->IsScalar(),
      "Cannot create a scalar expression for a vector %s. Use the other overload instead.",
      varName.c_str()
  );
  Assert(
      var->GetType() == symir::SymIR::I32,
      "Variable %s is of type %s, however only 32-bit integer variables are supported for now!",
      varName.c_str(), symir::SymIR::GetTypeName(var->GetType()).c_str()
  );
  if (version == -1) {
    version = versions[varName];
  } else if (version == -2) {
    version = ++versions[varName];
  }
  // We used 33-bit bitvectors since we're overflowing 32-bit integers
  return ctx->bv_const((varName + "_v" + std::to_string(version)).c_str(), 33);
}

std::string IntUBInject::GetVecElName(const symir::VarDef *var, int loc) const {
  const auto &varName = var->GetName();
  Assert(
      var->IsVector(), "The variable %s is not a vector, cannot get element name", varName.c_str()
  );
  const auto numEls = var->GetVecNumEls();
  Assert(
      loc < numEls, "The element index (%d) is out of bound (%d) for variable %s", loc, numEls,
      varName.c_str()
  );
  return varName + "_el" + std::to_string(loc);
}

z3::expr IntUBInject::CreateVecElExpr(const symir::VarDef *var, int loc, int version) {
  Assert(var != nullptr, "Cannot create a variable expression for a nullptr array variable");
  Assert(
      var->IsVector(), "Cannot create a vector element expression for a scalar variable. "
                       "Use the other overload instead."
  );
  const auto eleName = GetVecElName(var, loc);
  if (version == -1) {
    version = versions[eleName];
  } else if (version == -2) {
    version = ++versions[eleName];
  }
  return ctx->bv_const((eleName + "_v" + std::to_string(version)).c_str(), 33);
}

z3::expr IntUBInject::CreateCoefExpr(const symir::Coef &coef) {
  Assert(
      coef.GetType() == symir::SymIR::I32,
      "Coefficient %s is of type %s, however only 32-bit integer coefficient are supported for "
      "now!",
      coef.GetName().c_str(), symir::SymIR::GetTypeName(coef.GetType()).c_str()
  );
  // If we've already extracted the value from the model, we use that value,
  // otherwise it's an uninterpreted constant which the solver needs to figure
  // out the value for.
  // We used 33-bit bitvectors since we're overflowing 32-bit integers
  if (coef.IsSolved()) {
    return ctx->bv_val(coef.GetTypedValue<int>(), 33);
  } else {
    return ctx->bv_const(coef.GetName().c_str(), 33);
  }
}

std::unique_ptr<symir::Funct> IntUBInject::InjectUBs(const symir::Funct *f, const symir::Block *b) {
  Assert(b != nullptr, "The basic block to inject UBs is a nullptr");

  Log::Get().OpenSection("IntUBInject: Inject Int UBs to Block " + b->GetLabel());

  Reset();

  // Collect constraints that'll introduce int-related UBs
  ensureValidInitsForUses(b);
  b->Accept(*this);
  Log::Get().Out() << "Collected constraints:" << std::endl;
  for (const auto &c: *constraints) {
    Log::Get().Out() << c << std::endl;
  }

  // Check if we can find such a solution
  solver->add(*constraints);
  if (solver->check() != z3::sat) {
    Log::Get().Out() << "UNSAT" << std::endl;
    return nullptr;
  }
  Log::Get().Out() << "SAT" << std::endl;

  // Insert assignments to the used variables
  z3::model model = solver->get_model();
  // clang-format off
  auto nf = symir::FunctCopier(
    f,
    nullptr,
    [&](symir::FunctBuilder *fbd, symir::BlockBuilder *bbd) {
      if (bbd->GetLabel() != b->GetLabel()) {
        return; // We only inject UBs into the given block
      }
      Log::Get().Out()
          << "Adding initializations to ensure undefined behaviors" << std::endl;
      // We need the old block b since we are at the starting point of the new block
      // which has no statements and therefore no uses/definitions can be found.
      this->extractAndInitializeUses(model, b, fbd, bbd);
    },
    nullptr,
    nullptr
  ).Copy();
  // clang-format on
  // Now we need to extract the symbols (solved coefficients) from our model
  extractSymbolsFromModel(model, nf.get(), nf->FindBlock(b->GetLabel()));

  Log::Get().CloseSection();

  return nf;
}

void IntUBInject::Visit(const symir::VarUse &v) {
  Assert(v.GetType() == symir::SymIR::I32, "Only 32-bit integer variables are supported for now!");
  if (v.IsScalar()) {
    // We're not going to restrict the value of any variable
    pushExpression(CreateScaExpr(v.GetDef()));
  } else {
    const auto access = v.GetAccess();
    // We asked them to access the very last element
    for (int d = 0; d < v.GetVecNumDims(); d++) {
      access[d]->Accept(*this);
      auto coefExpr = popExpression();
      constraints->push_back(coefExpr == v.GetVecDimLen(d) - 1);
    }
    auto elExpr = CreateVecElExpr(v.GetDef(), v.GetVecNumEls() - 1);
    // We're not going to restrict the value of any element
    pushExpression(elExpr);
  }
}

void IntUBInject::Visit(const symir::Coef &c) {
  Assert(c.GetType() == symir::SymIR::I32, "Only 32-bit integer variables are supported for now!");
  const auto coefExpr = CreateCoefExpr(c);
  if (!c.IsSolved()) {
    // For the generated program, we'd like it as seemly "normal" as possible so that
    // both developers and compilers treat it as a "normal" program at a first galance.
    // So, let's make the coefficient to be in a range that is not too weird.
    constraints->push_back(coefExpr >= GlobalOptions::Get().LowerCoefBound);
    constraints->push_back(coefExpr <= GlobalOptions::Get().UpperCoefBound);
    // We create a bv2int map so that it's easy for us to extract the coefficient
    constraints->push_back(mapBitVecExprToIntExpr(coefExpr) == z3::bv2int(coefExpr, true));
  }
  pushExpression(coefExpr);
}

void IntUBInject::Visit(const symir::Term &t) {
  t.GetCoef()->Accept(*this);
  const auto coefExpr = popExpression();
  z3::expr varExpr = ctx->bv_val(0, 33);
  if (t.GetVar() != nullptr) {
    t.GetVar()->Accept(*this);
    varExpr = popExpression();
  }
  switch (t.GetOp()) {
    case symir::Term::OP_CST:
      pushExpression(coefExpr);
      break;

    case symir::Term::OP_ADD:
      pushExpression(coefExpr + varExpr);
      break;

    case symir::Term::OP_SUB:
      pushExpression(coefExpr - varExpr);
      break;

    case symir::Term::OP_MUL:
      pushExpression(coefExpr * varExpr);
      break;

    case symir::Term::OP_DIV:
      constraints->push_back(varExpr != 0); // We disallow division by zero
      pushExpression(z3::cxx_bvdiv(coefExpr, varExpr));
      break;

    case symir::Term::OP_REM:
      constraints->push_back(varExpr != 0); // We disallow division by zero
      pushExpression(z3::cxx_bvrem(coefExpr, varExpr));
      break;

    default:
      Panic("Unknown term operation: %s", symir::Term::GetOpName(t.GetOp()).c_str());
      break;
  }
}

void IntUBInject::Visit(const symir::Expr &e) {
  std::vector<z3::expr> exprs;
  for (const auto t: e.GetTerms()) {
    t->Accept(*this);
    exprs.push_back(popExpression());
  }
  z3::expr result = exprs.back();
  exprs.pop_back();
  for (int i = static_cast<int>(exprs.size()) - 1; i >= 0; --i) {
    switch (e.GetOp()) {
      case symir::Expr::OP_ADD:
        result = result + exprs[i];
        break;

      case symir::Expr::OP_SUB:
        result = result - exprs[i];
        break;

      default:
        Panic("Unknown expression operation: %s", symir::Expr::GetOpName(e.GetOp()).c_str());
        break;
    }
  }
  pushExpression(result);
}

void IntUBInject::Visit(const symir::Cond &c) {
  c.GetExpr()->Accept(*this);
  const auto expr = popExpression();
  ubConstraints.push_back(
      expr > GlobalOptions::Get().UpperBound || expr < GlobalOptions::Get().LowerBound
  ); // Inserts signed overflow
  switch (c.GetOp()) {
    case symir::Cond::OP_EQZ:
      pushExpression(expr == 0);
      break;

    case symir::Cond::OP_GTZ:
      pushExpression(expr != 0);
      break;

    case symir::Cond::OP_LTZ:
      pushExpression(expr < 0);
      break;

    default:
      break;
  }
}

void IntUBInject::Visit(const symir::AssStmt &a) {
  a.GetExpr()->Accept(*this);
  const auto expr = popExpression();
  ubConstraints.push_back(
      expr > GlobalOptions::Get().UpperBound || expr < GlobalOptions::Get().LowerBound
  ); // Inserts signed overflow
  if (a.GetVar()->IsScalar()) {
    auto varNewExpr = CreateScaExpr(a.GetVar()->GetDef(), -2);
    constraints->push_back(varNewExpr == expr);
  } else {
    // We only access the very last element. See Visit(const symir::VarUse &v).
    auto elNewExpr = CreateVecElExpr(a.GetVar()->GetDef(), a.GetVar()->GetVecNumEls() - 1, -2);
    constraints->push_back(elNewExpr == expr);
  }
}

void IntUBInject::Visit(const symir::RetStmt &r) {
  // Do nothing
}

void IntUBInject::Visit(const symir::Branch &b) {
  b.GetCond()->Accept(*this);
  popExpression(); // We don't care about the condition of the branch
}

void IntUBInject::Visit(const symir::Goto &g) {
  // Do nothing
}

void IntUBInject::Visit(const symir::ScaParam &p) {
  Panic("Cannot reach here: We only inject UBs into basic blocks, not parameters");
}

void IntUBInject::Visit(const symir::VecParam &p) {
  Panic("Cannot reach here: We only inject UBs into basic blocks, not parameters");
}

void IntUBInject::Visit(const symir::ScaLocal &l) {
  Panic("Cannot reach here: We only inject UBs into basic blocks, not locals");
}

void IntUBInject::Visit(const symir::VecLocal &l) {
  Panic("Cannot reach here: We only inject UBs into basic blocks, not locals");
}

void IntUBInject::Visit(const symir::Block &b) {
  for (const auto s: b.GetStmts()) {
    s->Accept(*this);
  }
  Assert(
      exprStack.empty(), "The expression stack is not empty after visiting the block %s",
      b.GetLabel().c_str()
  );
  // As long as one of the UB constraints is true, we have an UB.
  z3::expr ubc = ctx->bool_val(false);
  for (auto &c: ubConstraints) {
    ubc = ubc || c;
  }
  constraints->push_back(ubc);
}

void IntUBInject::Visit(const symir::Funct &f) {
  Panic("Cannot reach here: We only inject UBs into basic blocks, not functions");
}

z3::expr IntUBInject::mapBitVecExprToIntExpr(const z3::expr &bv) {
  return ctx->int_const((std::string(bv.decl().name().str()) + "_dec").c_str());
}

void IntUBInject::ensureValidInitsForUses(const symir::Block *b) {
  for (const auto uv: b->GetUses()) {
    z3::expr varExpr{*ctx};
    if (uv->IsScalar()) {
      varExpr = CreateScaExpr(uv->GetDef(), 0);
    } else {
      // We only access the very last element. See Visit(const symir::VarUse &v).
      varExpr = CreateVecElExpr(uv->GetDef(), uv->GetVecNumEls() - 1, 0);
    }
    constraints->push_back(varExpr <= GlobalOptions::Get().UpperBound);
    constraints->push_back(varExpr >= GlobalOptions::Get().LowerBound);
    // We create a bv2int map so that it's easy for us to extract the coefficient
    z3::expr varAsInt = mapBitVecExprToIntExpr(varExpr);
    constraints->push_back(varAsInt == z3::bv2int(varExpr, true));
  }
}

void IntUBInject::extractAndInitializeUses(
    const z3::model &model, const symir::Block *b, symir::FunctBuilder *funBd,
    symir::BlockBuilder *blkBd
) {
  for (const auto *uv: b->GetUses()) {
    const auto *nuvd = funBd->FindVar(uv->GetName());
    std::string varName;
    z3::func_decl varInitExpr{*ctx};
    if (uv->IsScalar()) {
      varName = uv->GetName();
      varInitExpr = mapBitVecExprToIntExpr(CreateScaExpr(nuvd, 0)).decl();
    } else {
      // We only access the very last element. See Visit(const symir::VarUse &v).
      varName = GetVecElName(nuvd, uv->GetVecNumEls() - 1);
      varInitExpr = mapBitVecExprToIntExpr(CreateVecElExpr(nuvd, uv->GetVecNumEls() - 1, 0)).decl();
    }
    // Every use has to be solved
    Assert(
        model.has_interp(varInitExpr),
        "We didn't find the initial value of the used variable %s to introduce int-related "
        "undefined behaviors",
        varName.c_str()
    );
    z3::expr varInitConst = model.get_const_interp(varInitExpr);
    Assert(varInitConst.is_numeral(), "Value %s is not a numeral", varName.c_str());
    int varInitVal;
    Assert(
        varInitConst.is_numeral_i(varInitVal),
        "Failed to obtain the valInitVal of variable %s from the model", varName.c_str()
    );
    Log::Get().Out() << "Let variable: var=" << varName << ", value=" << varInitVal << std::endl;
    // Insert an assignment to the variable with its initial value like: uv <- 0* uv +
    // initial_value
    if (uv->IsScalar()) {
      blkBd->SymAssign(
          nuvd, blkBd->SymAddExpr(
                    {blkBd->SymMulTerm(funBd->SymI32Const(0), nuvd),
                     blkBd->SymCstTerm(funBd->SymI32Const(varInitVal), nullptr)}
                )
      );
    } else {
      std::vector<symir::Coef *> access;
      for (int d = 0; d < uv->GetVecNumDims(); d++) {
        access.push_back(funBd->SymI32Const(uv->GetVecDimLen(d) - 1));
      }
      blkBd->SymAssign(
          nuvd,
          blkBd->SymAddExpr(
              {blkBd->SymMulTerm(funBd->SymI32Const(0), nuvd, access),
               blkBd->SymCstTerm(funBd->SymI32Const(varInitVal), nullptr)}
          ),
          access
      );
    }
  }
}

void IntUBInject::extractSymbolsFromModel(
    const z3::model &model, const symir::Funct *f, const symir::Block *b
) {
  for (auto symbol: f->GetSymbols()) {
    if (symbol->IsSolved()) {
      continue;
    }
    const auto symName = symbol->GetName().c_str();
    // Currently, we only support coefficients
    const auto symKey =
        mapBitVecExprToIntExpr(CreateCoefExpr(*dynamic_cast<const symir::Coef *>(symbol))).decl();
    if (!model.has_interp(symKey)) {
      continue; // The symbols don't belong to this basic block
    }
    z3::expr symConst = model.get_const_interp(symKey);
    Assert(symConst.is_numeral(), "Symbol %s is not a numeral", symName);
    int symVal;
    Assert(
        symConst.is_numeral_i(symVal), "Failed to obtain the symVal of symbol %s from the model",
        symName
    );
    symbol->SetValue(std::to_string(symVal));
    Log::Get().Out() << "Let symbol: sym=" << symName << ", value=" << symVal << std::endl;
  }
}
