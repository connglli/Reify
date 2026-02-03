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

#include "lib/ubbase.hpp"

#include "global.hpp"

bitwuzla::Term UBVisitorBase::CreateScaExpr(const symir::VarDef *var, int version) {
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

std::string UBVisitorBase::GetVecElName(const symir::VarDef *var, int loc) const {
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

bitwuzla::Term UBVisitorBase::CreateVecElExpr(const symir::VarDef *var, int loc, int version) {
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

std::string
UBVisitorBase::GetStructFieldName(const symir::VarDef *var, const std::string &field) const {
  return var->GetName() + "_" + field;
}

bitwuzla::Term UBVisitorBase::CreateStructFieldExpr(
    const symir::VarDef *var, const std::string &field, int version
) {
  std::string name = GetStructFieldName(var, field);
  if (version == -1) {
    version = versions[name];
  } else if (version == -2) {
    version = ++versions[name];
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

bitwuzla::Term UBVisitorBase::CreateVersionedExpr(
    const symir::VarDef *var, const std::string &suffix, int version
) {
  std::string name = var->GetName() + suffix;
  if (version == -1) {
    version = versions[name];
  } else if (version == -2) {
    version = ++versions[name];
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

bitwuzla::Term UBVisitorBase::CreateCoefExpr(const symir::Coef &coef) {
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
