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
  // Note: Bitwuzla performs simplification automatically during solving.
  // We could use the simplify() method on the solver if needed, but
  // for now we'll rely on Bitwuzla's built-in optimization.
  // The constraints are already collected and will be simplified when solving.
}

bitwuzla::Term UBSan::CreateScaExpr(const symir::VarDef *var, int version) {
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

  // Cache all terms (Bitwuzla does not intern/unify by name).
  std::string fullName = varName + "_v" + std::to_string(version);

  auto it = paramTerms.find(fullName);
  if (it != paramTerms.end()) {
    return it->second;
  }
  auto term = tm->mk_const(bvSort, fullName.c_str());
  paramTerms[fullName] = term;
  return term;
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

bitwuzla::Term UBSan::CreateVecElExpr(const symir::VarDef *var, int loc, int version) {
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

  // Cache all terms (Bitwuzla does not intern/unify by name).
  std::string fullName = elName + "_v" + std::to_string(version);

  auto it = paramTerms.find(fullName);
  if (it != paramTerms.end()) {
    return it->second;
  }
  auto term = tm->mk_const(bvSort, fullName.c_str());
  paramTerms[fullName] = term;
  return term;
}

std::string UBSan::GetStructFieldName(const symir::VarDef *var, const std::string &field) const {
  return var->GetName() + "_" + field;
}

bitwuzla::Term
UBSan::CreateStructFieldExpr(const symir::VarDef *var, const std::string &field, int version) {
  std::string name = GetStructFieldName(var, field);
  if (version == -1) {
    version = versions[name];
  } else if (version == -2) {
    version = ++versions[name];
    verbbls[name] = currBbl;
  }

  // Cache all terms (Bitwuzla does not intern/unify by name).
  std::string fullName = name + "_v" + std::to_string(version);

  auto it = paramTerms.find(fullName);
  if (it != paramTerms.end()) {
    return it->second;
  }
  auto term = tm->mk_const(bvSort, fullName.c_str());
  paramTerms[fullName] = term;
  return term;
}

bitwuzla::Term
UBSan::CreateVersionedExpr(const symir::VarDef *var, const std::string &suffix, int version) {
  std::string name = var->GetName() + suffix;
  if (version == -1) {
    version = versions[name];
  } else if (version == -2) {
    version = ++versions[name];
    verbbls[name] = currBbl;
  }

  // Cache all terms (Bitwuzla does not intern/unify by name).
  // This is critical for parameter extraction from the model.
  std::string fullName = name + "_v" + std::to_string(version);

  auto it = paramTerms.find(fullName);
  if (it != paramTerms.end()) {
    return it->second;
  }
  auto term = tm->mk_const(bvSort, fullName.c_str());
  paramTerms[fullName] = term;
  return term;
}

bitwuzla::Term UBSan::CreateCoefExpr(const symir::Coef &coef) {
  // If we've already extracted the value from the model, we use that value,
  // otherwise it's an uninterpreted constant which the solver needs to figure
  // out the value for
  if (coef.IsSolved()) {
    return tm->mk_bv_value_int64(bvSort, std::stoll(coef.GetValue()));
  } else {
    // Use cached term if it exists, otherwise create and cache it
    const std::string &coefName = coef.GetName();
    auto it = coefTerms.find(coefName);
    if (it != coefTerms.end()) {
      return it->second;
    } else {
      auto term = tm->mk_const(bvSort, coefName.c_str());
      coefTerms[coefName] = term;
      return term;
    }
  }
}

void UBSan::MakeInitInteresting() {
  std::vector<bitwuzla::Term> params;
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
  constraints.push_back(AtMostKZeroes(*tm, params, static_cast<int>(params.size()) / 2));
}

void UBSan::MakeInitWithRandomValue() {
  auto rand =
      Random::Get().Uniform(GlobalOptions::Get().LowerBound, GlobalOptions::Get().UpperBound);
  for (auto param: fun.GetParams()) {
    if (param->IsScalar()) {
      if (param->GetType() == symir::SymIR::Type::STRUCT) {
        const auto *sDef = fun.GetStruct(param->GetStructName());
        IterateStructElements(fun, sDef, [&](std::string elName) {
          auto val = tm->mk_bv_value_int64(bvSort, rand());
          constraints.push_back(
              tm->mk_term(bitwuzla::Kind::EQUAL, {CreateStructFieldExpr(param, elName, 0), val})
          );
        });
      } else {
        auto val = tm->mk_bv_value_int64(bvSort, rand());
        constraints.push_back(tm->mk_term(bitwuzla::Kind::EQUAL, {CreateScaExpr(param, 0), val}));
      }
    } else {
      for (int i = 0; i < param->GetVecNumEls(); i++) {
        auto val = tm->mk_bv_value_int64(bvSort, rand());
        constraints.push_back(
            tm->mk_term(bitwuzla::Kind::EQUAL, {CreateVecElExpr(param, i, 0), val})
        );
      }
    }
  }
}

void UBSan::MakeInitDifferentFrom(const std::vector<ArgPlus<int>> &init) {
  // There should be at lease NUM_VAR/2 variables which are not equal in both initialisations
  std::vector<bitwuzla::Term> params;
  auto zero = tm->mk_bv_zero(bvSort);
  auto one = tm->mk_bv_one(bvSort);

  for (int i = 0; i < fun.NumParams(); i++) {
    const auto p = fun.GetParams()[i];
    const auto &oldValue = init[i];
    if (p->IsScalar()) {
      if (p->GetType() == symir::SymIR::Type::STRUCT) {
        const auto *sDef = fun.GetStruct(p->GetStructName());
        int k = 0;
        IterateStructElements(fun, sDef, [&](std::string elName) {
          bitwuzla::Term newValue = CreateStructFieldExpr(p, elName, 0);
          auto oldVal = tm->mk_bv_value_int64(bvSort, oldValue.GetValue(k));
          auto isNotEqual = tm->mk_term(bitwuzla::Kind::DISTINCT, {newValue, oldVal});
          params.push_back(tm->mk_term(bitwuzla::Kind::ITE, {isNotEqual, one, zero}));
          k++;
        });
      } else {
        bitwuzla::Term newValue = CreateScaExpr(p, 0);
        auto oldVal = tm->mk_bv_value_int64(bvSort, oldValue.GetValue());
        auto isNotEqual = tm->mk_term(bitwuzla::Kind::DISTINCT, {newValue, oldVal});
        params.push_back(tm->mk_term(bitwuzla::Kind::ITE, {isNotEqual, one, zero}));
      }
    } else {
      for (int j = 0; j < p->GetVecNumEls(); j++) {
        bitwuzla::Term newValue = CreateVecElExpr(p, j, 0);
        auto oldVal = tm->mk_bv_value_int64(bvSort, oldValue.GetValue(j));
        auto isNotEqual = tm->mk_term(bitwuzla::Kind::DISTINCT, {newValue, oldVal});
        params.push_back(tm->mk_term(bitwuzla::Kind::ITE, {isNotEqual, one, zero}));
      }
    }
  }
  constraints.push_back(AtMostKZeroes(*tm, params, std::min(3, static_cast<int>(params.size()) / 2))
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
      auto dimLenTerm = tm->mk_bv_value_int64(bvSort, dimLen);
      auto zero = tm->mk_bv_zero(bvSort);

      constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SGE, {idxExpr, zero}));
      constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SLT, {idxExpr, dimLenTerm}));

      // Concretize
      int elLoc = 0;
      if (idxExpr.is_value()) {
        elLoc = std::stoll(idxExpr.value<std::string>(2), nullptr, 2);
        if (elLoc < 0 || elLoc >= dimLen) {
          elLoc = Random::Get().Uniform(0, dimLen - 1)();
        }
      } else {
        elLoc = Random::Get().Uniform(0, dimLen - 1)();
        auto elLocTerm = tm->mk_bv_value_int64(bvSort, elLoc);
        constraints.push_back(tm->mk_term(bitwuzla::Kind::EQUAL, {idxExpr, elLocTerm}));
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
      auto numFieldsTerm = tm->mk_bv_value_int64(bvSort, numFields);
      auto zero = tm->mk_bv_zero(bvSort);

      constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SGE, {idxExpr, zero}));
      constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SLT, {idxExpr, numFieldsTerm}));

      int fIdx = 0;
      if (idxExpr.is_value()) {
        fIdx = std::stoll(idxExpr.value<std::string>(2), nullptr, 2);
        if (fIdx < 0 || fIdx >= numFields) {
          fIdx = Random::Get().Uniform(0, numFields - 1)();
        }
      } else {
        fIdx = Random::Get().Uniform(0, numFields - 1)();
        auto fIdxTerm = tm->mk_bv_value_int64(bvSort, fIdx);
        constraints.push_back(tm->mk_term(bitwuzla::Kind::EQUAL, {idxExpr, fIdxTerm}));
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
  auto lowerBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().LowerBound);
  auto upperBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().UpperBound);
  constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SGE, {finalExpr, lowerBound}));
  constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SLE, {finalExpr, upperBound}));

  pushExpression(finalExpr);
  pushSuffix(suffix);
}

void UBSan::Visit(const symir::Coef &c) {
  Assert(c.GetType() == symir::SymIR::I32, "Only 32-bit integer variables are supported for now!");
  auto coefExpr = CreateCoefExpr(c);
  auto lowerBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().LowerBound);
  auto upperBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().UpperBound);
  constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SGE, {coefExpr, lowerBound}));
  constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SLE, {coefExpr, upperBound}));
  pushExpression(coefExpr);
}

void UBSan::Visit(const symir::Term &t) {
  t.GetCoef()->Accept(*this);
  bitwuzla::Term coefExpr = popExpression();

  bitwuzla::Term varExpr = tm->mk_bv_zero(bvSort);
  if (t.GetOp() != symir::Term::Op::OP_CST) {
    t.GetVar()->Accept(*this);
    varExpr = popExpression();
    popSuffix(); // Discard suffix as we used the value
  }

  bitwuzla::Term termExpr = tm->mk_bv_zero(bvSort);
  auto zero = tm->mk_bv_zero(bvSort);

  switch (t.GetOp()) {
    case symir::Term::Op::OP_ADD:
      // Prevent signed addition overflow
      constraints.push_back(tm->mk_term(
          bitwuzla::Kind::NOT, {tm->mk_term(bitwuzla::Kind::BV_SADD_OVERFLOW, {coefExpr, varExpr})}
      ));
      termExpr = tm->mk_term(bitwuzla::Kind::BV_ADD, {coefExpr, varExpr});
      break;
    case symir::Term::Op::OP_SUB:
      // Prevent signed subtraction overflow
      constraints.push_back(tm->mk_term(
          bitwuzla::Kind::NOT, {tm->mk_term(bitwuzla::Kind::BV_SSUB_OVERFLOW, {coefExpr, varExpr})}
      ));
      termExpr = tm->mk_term(bitwuzla::Kind::BV_SUB, {coefExpr, varExpr});
      break;
    case symir::Term::Op::OP_MUL:
      // Prevent signed multiplication overflow
      constraints.push_back(tm->mk_term(
          bitwuzla::Kind::NOT, {tm->mk_term(bitwuzla::Kind::BV_SMUL_OVERFLOW, {coefExpr, varExpr})}
      ));
      termExpr = tm->mk_term(bitwuzla::Kind::BV_MUL, {coefExpr, varExpr});
      break;
    case symir::Term::Op::OP_DIV:
      constraints.push_back(tm->mk_term(bitwuzla::Kind::DISTINCT, {varExpr, zero}));
      // Prevent signed division overflow (INT_MIN / -1)
      constraints.push_back(tm->mk_term(
          bitwuzla::Kind::NOT, {tm->mk_term(bitwuzla::Kind::BV_SDIV_OVERFLOW, {coefExpr, varExpr})}
      ));
      termExpr = tm->mk_term(bitwuzla::Kind::BV_SDIV, {coefExpr, varExpr});
      break;
    case symir::Term::Op::OP_REM:
      constraints.push_back(tm->mk_term(bitwuzla::Kind::DISTINCT, {varExpr, zero}));
      // Note: Remainder doesn't have overflow (same as division overflow would be INT_MIN % -1)
      // But we'll add the constraint anyway for consistency
      constraints.push_back(tm->mk_term(
          bitwuzla::Kind::NOT, {tm->mk_term(bitwuzla::Kind::BV_SDIV_OVERFLOW, {coefExpr, varExpr})}
      ));
      termExpr = tm->mk_term(bitwuzla::Kind::BV_SREM, {coefExpr, varExpr});
      break;
    case symir::Term::Op::OP_CST:
      termExpr = coefExpr;
      break;
    default:
      Panic("Cannot reacher here");
  }

  auto lowerBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().LowerBound);
  auto upperBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().UpperBound);
  constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SGE, {termExpr, lowerBound}));
  constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SLE, {termExpr, upperBound}));

  pushExpression(termExpr);
}

void UBSan::Visit(const symir::Expr &e) {
  auto result = tm->mk_bv_zero(bvSort);
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
        // Prevent signed addition overflow
        constraints.push_back(tm->mk_term(
            bitwuzla::Kind::NOT, {tm->mk_term(bitwuzla::Kind::BV_SADD_OVERFLOW, {result, termExpr})}
        ));
        result = tm->mk_term(bitwuzla::Kind::BV_ADD, {result, termExpr});
        break;
      case symir::Expr::Op::OP_SUB:
        // Prevent signed subtraction overflow
        constraints.push_back(tm->mk_term(
            bitwuzla::Kind::NOT, {tm->mk_term(bitwuzla::Kind::BV_SSUB_OVERFLOW, {result, termExpr})}
        ));
        result = tm->mk_term(bitwuzla::Kind::BV_SUB, {result, termExpr});
        break;
      default:
        Panic("Cannot reacher here");
    }
    auto lowerBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().LowerBound);
    auto upperBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().UpperBound);
    constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SGE, {result, lowerBound}));
    constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SLE, {result, upperBound}));
  }
  if (enableInterestCoefs) {
    makeCoefsInteresting(coefs);
  }
  pushExpression(result);
}

void UBSan::Visit(const symir::Cond &c) {
  c.GetExpr()->Accept(*this);
  auto expr = popExpression();
  auto zero = tm->mk_bv_zero(bvSort);

  switch (c.GetOp()) {
    case symir::Cond::Op::OP_GTZ:
      pushExpression(tm->mk_term(bitwuzla::Kind::BV_SGT, {expr, zero}));
      break;
    case symir::Cond::Op::OP_LTZ:
      pushExpression(tm->mk_term(bitwuzla::Kind::BV_SLT, {expr, zero}));
      break;
    case symir::Cond::Op::OP_EQZ:
      pushExpression(tm->mk_term(bitwuzla::Kind::EQUAL, {expr, zero}));
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

  constraints.push_back(tm->mk_term(bitwuzla::Kind::EQUAL, {newVerExpr, exprExpr}));

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
    constraints.push_back(tm->mk_term(bitwuzla::Kind::EQUAL, {newVerExpr, otherVarExpr}));
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
    constraints.push_back(tm->mk_term(bitwuzla::Kind::NOT, {condExpr}));
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
  auto lowerBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().LowerBound);
  auto upperBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().UpperBound);
  constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SLE, {varExpr, upperBound}));
  constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SGE, {varExpr, lowerBound}));
}

void UBSan::Visit(const symir::VecParam &p) {
  auto lowerBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().LowerBound);
  auto upperBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().UpperBound);
  for (int e = 0; e < p.GetVecNumEls(); e++) {
    auto elExpr = CreateVecElExpr(&p, e, 0);
    auto elName = GetVecElName(&p, e);
    versions[elName] = 0;
    constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SLE, {elExpr, upperBound}));
    constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SGE, {elExpr, lowerBound}));
  }
}

void UBSan::Visit(const symir::StructParam &p) {
  const auto *sDef = fun.GetStruct(p.GetStructName());
  Assert(
      sDef != nullptr, "Struct definition %s not found for param %s", p.GetStructName().c_str(),
      p.GetName().c_str()
  );
  auto lowerBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().LowerBound);
  auto upperBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().UpperBound);
  IterateStructElements(fun, sDef, [&](std::string elName) {
    auto fieldExpr = CreateStructFieldExpr(&p, elName, 0);
    std::string fullFieldName = GetStructFieldName(&p, elName);
    versions[fullFieldName] = 0;
    verbbls[fullFieldName] = "___entry_bbl";
    constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SLE, {fieldExpr, upperBound}));
    constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SGE, {fieldExpr, lowerBound}));
  });
}

void UBSan::Visit(const symir::ScaLocal &l) {
  l.GetCoef()->Accept(*this);
  auto coefExpr = popExpression();
  auto varExpr = CreateScaExpr(l.GetDefinition(), 0);
  versions[l.GetName()] = 0;
  verbbls[l.GetName()] = "___entry_bbl";
  auto lowerBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().LowerBound);
  auto upperBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().UpperBound);
  constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SLE, {varExpr, upperBound}));
  constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SGE, {varExpr, lowerBound}));
  constraints.push_back(tm->mk_term(bitwuzla::Kind::EQUAL, {varExpr, coefExpr}));
}

void UBSan::Visit(const symir::VecLocal &l) {
  int numEls = l.GetVecNumEls();
  const auto &inits = l.GetCoefs();
  auto lowerBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().LowerBound);
  auto upperBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().UpperBound);
  for (int i = 0; i < numEls; i++) {
    inits[i]->Accept(*this);
    auto coefExpr = popExpression();
    auto elName = GetVecElName(l.GetDefinition(), i);
    auto elExpr = CreateVecElExpr(l.GetDefinition(), i, 0);
    versions[elName] = 0;
    constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SLE, {elExpr, upperBound}));
    constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SGE, {elExpr, lowerBound}));
    constraints.push_back(tm->mk_term(bitwuzla::Kind::EQUAL, {elExpr, coefExpr}));
  }
  if (enableInterestCoefs) {
    makeCoefsInteresting(inits);
  }
}

void UBSan::Visit(const symir::StructLocal &l) {
  const auto *sDef = fun.GetStruct(l.GetStructName());
  const auto &inits = l.GetCoefs();
  auto lowerBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().LowerBound);
  auto upperBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().UpperBound);

  size_t initIdx = 0;
  IterateStructElements(fun, sDef, [&](std::string elName) {
    Assert(initIdx < inits.size(), "Mismatch init size for struct local %s", l.GetName().c_str());
    inits[initIdx]->Accept(*this);
    auto coefExpr = popExpression();

    auto fieldExpr = CreateStructFieldExpr(l.GetDefinition(), elName, 0);
    std::string fullFieldName = GetStructFieldName(l.GetDefinition(), elName);
    versions[fullFieldName] = 0;
    verbbls[fullFieldName] = "___entry_bbl";

    constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SLE, {fieldExpr, upperBound}));
    constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SGE, {fieldExpr, lowerBound}));
    constraints.push_back(tm->mk_term(bitwuzla::Kind::EQUAL, {fieldExpr, coefExpr}));
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

  // Initialize the bitvector sort for 32-bit integers
  bvSort = tm->mk_bv_sort(32);

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
  std::vector<bitwuzla::Term> exprs;
  for (auto c: coefs) {
    exprs.push_back(CreateCoefExpr(*c));
  }
  // Allow at most 1 zero coefficient to prevent all-zero expressions
  // (which can cause division by zero or infinite loops)
  constraints.push_back(AtMostKZeroes(*tm, exprs, std::min(1, static_cast<int>(coefs.size()) / 3)));
}
