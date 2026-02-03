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

#ifndef REIFY_UBBASE_HPP
#define REIFY_UBBASE_HPP

#include <bitwuzla/cpp/bitwuzla.h>
#include <map>
#include <memory>
#include <stack>
#include <string>

#include "lib/lang.hpp"

/// Base class for UB-related visitors that provides common functionality
/// for creating versioned expressions and managing term caches.
class UBVisitorBase : protected symir::SymIRVisitor {
protected:
  UBVisitorBase(std::unique_ptr<bitwuzla::TermManager> tm, bitwuzla::Sort bvSort) :
      tm(std::move(tm)), bvSort(bvSort) {}

  virtual ~UBVisitorBase() = default;

  // Create a versioned variable expression for the given scalar variable
  // When version==-1, the current version in the version table is used
  // When version==-2, the version in the version table is incremented and the new version is used
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

  // Push an expression to the expression stack
  void pushExpression(bitwuzla::Term expr) { exprStack.push(std::move(expr)); }

  // Pop an expression from the expression stack
  bitwuzla::Term popExpression() {
    auto e = exprStack.top();
    exprStack.pop();
    return e;
  }

protected:
  std::unique_ptr<bitwuzla::TermManager> tm; // The term manager for constraint collecting
  bitwuzla::Sort bvSort;                     // The bitvector sort for operations

  // Context used in collecting constraints
  std::stack<bitwuzla::Term> exprStack{}; // The expression stack for evaluating the SymIR program
  std::map<std::string, int> versions{};  // The SSA version table for each variable

  // Term caches to ensure Bitwuzla terms are reused (Bitwuzla doesn't unify by name!)
  std::map<std::string, bitwuzla::Term> coefTerms{};  // Cache of coefficient terms
  std::map<std::string, bitwuzla::Term> paramTerms{}; // Cache of parameter/variable terms
};

#endif // REIFY_UBBASE_HPP
