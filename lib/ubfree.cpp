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

namespace {
  void IterateStructElements(
      const symir::Funct &fun, const symir::StructDef *sDef,
      const std::function<void(std::string)> &callback, std::string prefix = ""
  ) {
    for (const auto &field: sDef->GetFields()) {
      int numEls = 1;
      if (!field.shape.empty())
        numEls = symir::VarDef::GetVecNumEls(field.shape);

      for (int k = 0; k < numEls; ++k) {
        std::string elName =
            prefix + field.name + (field.shape.empty() ? "" : "_el" + std::to_string(k));

        if (field.baseType == symir::SymIR::Type::STRUCT ||
            field.type == symir::SymIR::Type::STRUCT) {
          const auto *innerDef = fun.GetStruct(field.structName);
          Assert(
              innerDef != nullptr, "Inner struct definition %s not found for field %s",
              field.structName.c_str(), field.name.c_str()
          );
          IterateStructElements(fun, innerDef, callback, elName + "_");
        } else {
          callback(elName);
        }
      }
    }
  }

  int FlattenRowMajorIndex(const std::vector<int> &shape, const std::vector<int> &indices) {
    Assert(
        shape.size() == indices.size(),
        "Cannot flatten index: shape dims (%zu) != indices dims (%zu)", shape.size(), indices.size()
    );
    int flat = 0;
    for (size_t i = 0; i < shape.size(); ++i) {
      Assert(shape[i] > 0, "Invalid shape dim (%d) while flattening", shape[i]);

      // Indices should already be constrained to be in-bounds at this point.
      Assert(
          indices[i] >= 0 && indices[i] < shape[i],
          "Index out of bounds while flattening: idx=%d dim=%d", indices[i], shape[i]
      );
      flat = flat * shape[i] + indices[i];
    }
    return flat;
  }
} // namespace

void UBSan::Optimize() {
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

z3::expr UBSan::CreateScaExpr(const symir::VarDef *var, int version) {
  Assert(var != nullptr, "Cannot create a variable expression for a nullptr variable");
  const auto &varName = var->GetName();
  Assert(
      var->IsScalar(),
      "Cannot create a scalar expression for a vector (%s). Use the other overload instead.",
      varName.c_str()
  );
  if (version == -1) {
    version = versions[varName];
  } else if (version == -2) {
    version = ++versions[varName];
    verbbls[varName] = currBbl;
  }
  return ctx.int_const((varName + "_v" + std::to_string(version)).c_str());
}

std::string UBSan::GetVecElName(const symir::VarDef *var, int loc) const {
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

z3::expr UBSan::CreateVecElExpr(const symir::VarDef *var, int loc, int version) {
  Assert(var != nullptr, "Cannot create a variable expression for a nullptr array variable");
  Assert(
      var->IsVector(),
      "Cannot create a variable expression for a scalar variable (%s). Use the "
      "other overload instead.",
      var->GetName().c_str()
  );
  const auto elName = GetVecElName(var, loc);
  if (version == -1) {
    version = versions[elName];
  } else if (version == -2) {
    version = ++versions[elName];
  }
  return ctx.int_const((elName + "_v" + std::to_string(version)).c_str());
}

std::string UBSan::GetStructFieldName(const symir::VarDef *var, const std::string &field) const {
  return var->GetName() + "_" + field;
}

z3::expr
UBSan::CreateStructFieldExpr(const symir::VarDef *var, const std::string &field, int version) {
  std::string name = GetStructFieldName(var, field);
  if (version == -1) {
    version = versions[name];
  } else if (version == -2) {
    version = ++versions[name];
    verbbls[name] = currBbl;
  }
  return ctx.int_const((name + "_v" + std::to_string(version)).c_str());
}

z3::expr
UBSan::CreateVersionedExpr(const symir::VarDef *var, const std::string &suffix, int version) {
  std::string name = var->GetName() + suffix;
  if (version == -1) {
    version = versions[name];
  } else if (version == -2) {
    version = ++versions[name];
    verbbls[name] = currBbl;
  }
  return ctx.int_const((name + "_v" + std::to_string(version)).c_str());
}

z3::expr UBSan::CreateCoefExpr(const symir::Coef &coef) {
  // If we've already extracted the value from the model, we use that value,
  // otherwise it's an uninterpreted constant which the solver needs to figure
  // out the value for
  if (coef.IsSolved()) {
    return ctx.int_val(std::stoi(coef.GetValue()));
  } else {
    return ctx.int_const(coef.GetName().c_str());
  }
}

void UBSan::MakeInitInteresting() {
  std::vector<z3::expr> params;
  for (auto param: fun.GetParams()) {
    if (param->IsScalar()) {
      if (param->GetType() == symir::SymIR::Type::STRUCT) {
        const auto *sDef = fun.GetStruct(param->GetStructName());
        IterateStructElements(fun, sDef, [&](std::string elName) {
          params.push_back(CreateStructFieldExpr(param, elName, 0));
        });
      } else {
        params.push_back(CreateScaExpr(param, 0));
      }
    } else {
      for (int i = 0; i < param->GetVecNumEls(); i++) {
        params.push_back(CreateVecElExpr(param, i, 0));
      }
    }
  }
  constraints.push_back(AtMostKZeroes(ctx, params, static_cast<int>(params.size()) / 2));
}

void UBSan::MakeInitWithRandomValue() {
  auto rand = Random::Get().Uniform(
      GlobalOptions::Get().LowerInitBound, GlobalOptions::Get().UpperInitBound
  );
  for (auto param: fun.GetParams()) {
    if (param->IsScalar()) {
      if (param->GetType() == symir::SymIR::Type::STRUCT) {
        const auto *sDef = fun.GetStruct(param->GetStructName());
        IterateStructElements(fun, sDef, [&](std::string elName) {
          constraints.push_back(CreateStructFieldExpr(param, elName, 0) == rand());
        });
      } else {
        constraints.push_back(CreateScaExpr(param, 0) == rand());
      }
    } else {
      for (int i = 0; i < param->GetVecNumEls(); i++) {
        constraints.push_back(CreateVecElExpr(param, i, 0) == rand());
      }
    }
  }
}

void UBSan::MakeInitDifferentFrom(const std::vector<ArgPlus<int>> &init) {
  // There should be at lease NUM_VAR/2 variables which are not equal in both initialisations
  std::vector<z3::expr> params;
  for (int i = 0; i < fun.NumParams(); i++) {
    const auto p = fun.GetParams()[i];
    const auto &oldValue = init[i];
    if (p->IsScalar()) {
      if (p->GetType() == symir::SymIR::Type::STRUCT) {
        const auto *sDef = fun.GetStruct(p->GetStructName());
        int k = 0;
        IterateStructElements(fun, sDef, [&](std::string elName) {
          z3::expr newValue = CreateStructFieldExpr(p, elName, 0);
          params.push_back(z3::ite(newValue != oldValue.GetValue(k), ctx.int_val(1), ctx.int_val(0))
          );
          k++;
        });
      } else {
        z3::expr newValue = CreateScaExpr(p, 0);
        params.push_back(z3::ite(newValue != oldValue.GetValue(), ctx.int_val(1), ctx.int_val(0)));
      }
    } else {
      for (int j = 0; j < p->GetVecNumEls(); j++) {
        z3::expr newValue = CreateVecElExpr(p, j, 0);
        params.push_back(z3::ite(newValue != oldValue.GetValue(j), ctx.int_val(1), ctx.int_val(0)));
      }
    }
  }
  constraints.push_back(AtMostKZeroes(ctx, params, std::min(3, static_cast<int>(params.size()) / 2))
  );
}

void UBSan::Visit(const symir::VarUse &v) {
  Assert(
      v.GetType() == symir::SymIR::Type::I32, "Only 32-bit integer variables are supported for now!"
  );

  const auto *varDef = v.GetDef();
  const auto &access = v.GetAccess();

  symir::SymIR::Type currType = varDef->GetType();
  symir::SymIR::Type currBaseType = varDef->GetBaseType();
  std::string currStruct =
      (currType == symir::SymIR::Type::STRUCT)
          ? varDef->GetStructName()
          : (currBaseType == symir::SymIR::Type::STRUCT ? varDef->GetStructName() : "");
  std::vector<int> currShape = varDef->GetVecShape();
  size_t currentShapeIdx = 0; // Index into currShape

  std::string suffix = "";

  // Row-major flattening for the current contiguous array access chain.
  std::vector<int> pendingArrayIndices;
  std::vector<int> pendingArrayShape;

  for (size_t i = 0; i < access.size(); ++i) {
    access[i]->Accept(*this);
    auto idxExpr = popExpression();

    if (currentShapeIdx < currShape.size()) {
      // Array Access
      int dimLen = currShape[currentShapeIdx];

      constraints.push_back(idxExpr >= 0);
      constraints.push_back(idxExpr < dimLen);

      // Concretize
      int elLoc = 0;
      int constIdx = 0;
      if (idxExpr.is_numeral_i(constIdx)) {
        elLoc = constIdx;
      } else {
        elLoc = Random::Get().Uniform(0, dimLen - 1)();
        constraints.push_back(idxExpr == elLoc);
      }

      pendingArrayShape.push_back(dimLen);
      pendingArrayIndices.push_back(elLoc);
      currentShapeIdx++;

      if (currentShapeIdx == currShape.size()) {
        const int flatLoc = FlattenRowMajorIndex(pendingArrayShape, pendingArrayIndices);
        suffix += "_el" + std::to_string(flatLoc);
        pendingArrayShape.clear();
        pendingArrayIndices.clear();

        // Array exhausted. Switch to base type.
        currType = currBaseType;
        // Struct name is already set if base is struct
      }
    } else {
      // Struct Access
      Assert(currType == symir::SymIR::Type::STRUCT, "Excess access coefficients on non-struct");
      const auto *sDef = fun.GetStruct(currStruct);
      int numFields = sDef->GetFields().size();

      constraints.push_back(idxExpr >= 0);
      constraints.push_back(idxExpr < numFields);

      int fIdx = 0;
      int constIdx = 0;
      if (idxExpr.is_numeral_i(constIdx)) {
        fIdx = constIdx;
      } else {
        fIdx = Random::Get().Uniform(0, numFields - 1)();
        constraints.push_back(idxExpr == fIdx);
      }

      const auto &field = sDef->GetField(fIdx);
      suffix += "_" + field.name;

      // Update state for next level
      currType = field.type;
      currBaseType = field.baseType;
      if (currType == symir::SymIR::Type::STRUCT) {
        currStruct = field.structName;
      } else if (currBaseType == symir::SymIR::Type::STRUCT) {
        currStruct = field.structName;
      }
      currShape = field.shape;
      currentShapeIdx = 0;

      pendingArrayShape.clear();
      pendingArrayIndices.clear();
    }
  }

  auto finalExpr = CreateVersionedExpr(varDef, suffix);
  constraints.push_back(finalExpr >= GlobalOptions::Get().LowerBound);
  constraints.push_back(finalExpr <= GlobalOptions::Get().UpperBound);

  pushExpression(finalExpr);
  pushSuffix(suffix);
}

void UBSan::Visit(const symir::Coef &c) {
  Assert(c.GetType() == symir::SymIR::I32, "Only 32-bit integer variables are supported for now!");
  auto coefExpr = CreateCoefExpr(c);
  constraints.push_back(coefExpr >= GlobalOptions::Get().LowerCoefBound);
  constraints.push_back(coefExpr <= GlobalOptions::Get().UpperCoefBound);
  pushExpression(coefExpr);
}

void UBSan::Visit(const symir::Term &t) {
  t.GetCoef()->Accept(*this);
  z3::expr coefExpr = popExpression();

  z3::expr varExpr(ctx);
  if (t.GetOp() != symir::Term::Op::OP_CST) {
    t.GetVar()->Accept(*this);
    varExpr = popExpression();
    popSuffix(); // Discard suffix as we used the value
  }

  z3::expr termExpr(ctx);

  switch (t.GetOp()) {
    case symir::Term::Op::OP_ADD:
      termExpr = coefExpr + varExpr;
      break;
    case symir::Term::Op::OP_SUB:
      termExpr = coefExpr - varExpr;
      break;
    case symir::Term::Op::OP_MUL:
      termExpr = coefExpr * varExpr;
      break;
    case symir::Term::Op::OP_DIV:
      constraints.push_back(varExpr != 0);
      termExpr = z3::cxx_idiv(coefExpr, varExpr);
      break;
    case symir::Term::Op::OP_REM:
      constraints.push_back(varExpr != 0);
      termExpr = z3::cxx_irem(coefExpr, varExpr);
      break;
    case symir::Term::Op::OP_CST:
      termExpr = coefExpr;
      break;
    default:
      Panic("Cannot reacher here");
  }

  constraints.push_back(termExpr >= GlobalOptions::Get().LowerBound);
  constraints.push_back(termExpr <= GlobalOptions::Get().UpperBound);

  pushExpression(termExpr);
}

void UBSan::Visit(const symir::Expr &e) {
  auto result = ctx.int_val(0);
  std::vector<symir::Coef *> coefs;
  for (size_t i = 0; i < e.NumTerms(); i++) {
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
    makeCoefsInteresting(coefs);
  }
  pushExpression(result);
}

void UBSan::Visit(const symir::Cond &c) {
  c.GetExpr()->Accept(*this);
  auto expr = popExpression();
  switch (c.GetOp()) {
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

void UBSan::Visit(const symir::AssStmt &a) {
  a.GetExpr()->Accept(*this);
  auto exprExpr = popExpression();
  a.GetVar()->Accept(*this);
  auto newVerExpr = popExpression(); // It's indeed the old expr; we don't need it
  std::string suffix = popSuffix();

  newVerExpr = CreateVersionedExpr(a.GetVar()->GetDef(), suffix, -2);

  constraints.push_back(newVerExpr == exprExpr);
  // TODO: This is an ugly support for enabling value numbering. Try make this a separate visitor.
  // TODO: Also considering numbering the value of a term or an expression, besides a variable.
  if (GlobalOptions::Get().EnableValueNumbering && versions.size() > 1 &&
      Random::Get().UniformReal()() < GlobalOptions::Get().ValueNumberingProba) {
    // If value numbering is enabled, we also add a constraint that the new version
    // of the variable should be equal to the newest version of one other variable.
    // We only consider variables in the previous and the current block to let the compiler
    // found them more easily.
    // TODO: However, we are irreducible and hard to be analyzed. Better consider dup the
    // statement.
    std::vector<std::string> aliveVars{};
    for (const auto &[v, b]: verbbls) {
      if (b == currBbl || b == prevBbl) {
        aliveVars.push_back(v);
      }
    }
    if (aliveVars.empty()) {
      return; // No available variables to enforce value numbering
    }
    const auto ourName = a.GetVar()->GetName();
    const auto rand = Random::Get().Uniform(0, static_cast<int>(aliveVars.size()) - 1);
    std::string otherName;
    for (int tries = 0; tries < 10; tries++) {
      otherName = aliveVars[rand()];
      if (otherName != ourName) {
        break;
      }
    }
    if (otherName == ourName) {
      return; // Give up
    }
    const auto otherVar = fun.FindVar(otherName);
    if (!otherVar) {
      return;
    } // Might be struct field which is not easily found via FindVar
    // If otherVar is null (e.g. it was a struct field name in verbbls), we skip value numbering for
    // now.

    // Also if otherVar is struct, we skip.
    if (otherVar->GetType() == symir::SymIR::Type::STRUCT)
      return;

    auto otherVarExpr = CreateScaExpr(otherVar);
    constraints.push_back(newVerExpr == otherVarExpr);
    Log::Get().Out() << "Value numbering enforced: " << ourName << "(version=" << versions[ourName]
                     << ") == " << otherName << "(version=" << versions[otherName] << ")"
                     << std::endl;
  }
}

void UBSan::Visit(const symir::RetStmt &r) { /* DO NOTHING */ }

void UBSan::Visit(const symir::Branch &b) {
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
  } else {
    Panic(
        "The branch (true: \"%s\", false: \"%s\") does not have targets called \"%s\"",
        b.GetTrueTarget().c_str(), b.GetFalseTarget().c_str(), nextBbl.c_str()
    );
  }
}

void UBSan::Visit(const symir::Goto &g) { /* DO NOTHING */ }

void UBSan::Visit(const symir::ScaParam &p) {
  auto varExpr = CreateScaExpr(&p, 0);
  versions[p.GetName()] = 0;
  verbbls[p.GetName()] = "___entry_bbl";
  constraints.push_back(varExpr <= GlobalOptions::Get().UpperInitBound);
  constraints.push_back(varExpr >= GlobalOptions::Get().LowerInitBound);
}

void UBSan::Visit(const symir::VecParam &p) {
  for (int e = 0; e < p.GetVecNumEls(); e++) {
    auto elExpr = CreateVecElExpr(&p, e, 0);
    auto elName = GetVecElName(&p, e);
    versions[elName] = 0;
    constraints.push_back(elExpr <= GlobalOptions::Get().UpperInitBound);
    constraints.push_back(elExpr >= GlobalOptions::Get().LowerInitBound);
  }
}

void UBSan::Visit(const symir::StructParam &p) {
  const auto *sDef = fun.GetStruct(p.GetStructName());
  Assert(
      sDef != nullptr, "Struct definition %s not found for param %s", p.GetStructName().c_str(),
      p.GetName().c_str()
  );
  IterateStructElements(fun, sDef, [&](std::string elName) {
    auto fieldExpr = CreateStructFieldExpr(&p, elName, 0);
    std::string fullFieldName = GetStructFieldName(&p, elName);
    versions[fullFieldName] = 0;
    verbbls[fullFieldName] = "___entry_bbl";
    constraints.push_back(fieldExpr <= GlobalOptions::Get().UpperInitBound);
    constraints.push_back(fieldExpr >= GlobalOptions::Get().LowerInitBound);
  });
}

void UBSan::Visit(const symir::ScaLocal &l) {
  l.GetCoef()->Accept(*this);
  auto coefExpr = popExpression();
  auto varExpr = CreateScaExpr(l.GetDefinition(), 0);
  versions[l.GetName()] = 0;
  verbbls[l.GetName()] = "___entry_bbl";
  constraints.push_back(varExpr <= GlobalOptions::Get().UpperInitBound);
  constraints.push_back(varExpr >= GlobalOptions::Get().LowerInitBound);
  constraints.push_back(varExpr == coefExpr);
}

void UBSan::Visit(const symir::VecLocal &l) {
  int numEls = l.GetVecNumEls();
  const auto &inits = l.GetCoefs();
  for (int i = 0; i < numEls; i++) {
    inits[i]->Accept(*this);
    auto coefExpr = popExpression();
    auto elName = GetVecElName(l.GetDefinition(), i);
    auto elExpr = CreateVecElExpr(l.GetDefinition(), i, 0);
    versions[elName] = 0;
    constraints.push_back(elExpr <= GlobalOptions::Get().UpperInitBound);
    constraints.push_back(elExpr >= GlobalOptions::Get().LowerInitBound);
    constraints.push_back(elExpr == coefExpr);
  }
  if (enableInterestCoefs) {
    makeCoefsInteresting(inits);
  }
}

void UBSan::Visit(const symir::StructLocal &l) {
  const auto *sDef = fun.GetStruct(l.GetStructName());
  const auto &inits = l.GetCoefs();

  size_t initIdx = 0;
  IterateStructElements(fun, sDef, [&](std::string elName) {
    Assert(initIdx < inits.size(), "Mismatch init size for struct local %s", l.GetName().c_str());
    inits[initIdx]->Accept(*this);
    auto coefExpr = popExpression();

    auto fieldExpr = CreateStructFieldExpr(l.GetDefinition(), elName, 0);
    std::string fullFieldName = GetStructFieldName(l.GetDefinition(), elName);
    versions[fullFieldName] = 0;
    verbbls[fullFieldName] = "___entry_bbl";

    constraints.push_back(fieldExpr <= GlobalOptions::Get().UpperInitBound);
    constraints.push_back(fieldExpr >= GlobalOptions::Get().LowerInitBound);
    constraints.push_back(fieldExpr == coefExpr);
    initIdx++;
  });

  Assert(initIdx == inits.size(), "Mismatch init size for struct local %s", l.GetName().c_str());

  if (enableInterestCoefs) {
    makeCoefsInteresting(inits);
  }
}

void UBSan::Visit(const symir::StructDef &s) {
  // Struct definition doesn't introduce variables, so nothing to do for UBSan.
}

void UBSan::Visit(const symir::Block &b) {
  for (const auto &stmt: b.GetStmts()) {
    stmt->Accept(*this);
  }
}

void UBSan::Visit(const symir::Funct &f) {
  Assert(
      exprStack.empty(), "Expression stack is not empty before visiting function %s",
      f.GetName().c_str()
  );

  for (const auto &param: f.GetParams()) {
    param->Accept(*this);
  }
  for (const auto &local: f.GetLocals()) {
    local->Accept(*this);
  }
  for (size_t i = 0; i < execution.size() - 1; i++) {
    prevBbl = currBbl;
    currBbl = execution[i];
    nextBbl = execution[i + 1];
    f.GetBlock(currBbl)->Accept(*this);
  }
  prevBbl = currBbl;
  currBbl = execution.back();
  nextBbl = "";
  f.GetBlock(currBbl)->Accept(*this);

  Assert(
      exprStack.empty(), "Expression stack is not empty after visiting function %s",
      f.GetName().c_str()
  );
}

void UBSan::makeCoefsInteresting(const std::vector<symir::Coef *> &coefs) {
  std::vector<z3::expr> exprs;
  for (auto c: coefs) {
    exprs.push_back(CreateCoefExpr(*c));
  }
  constraints.push_back(AtMostKZeroes(ctx, exprs, static_cast<int>(coefs.size()) / 2));
}
