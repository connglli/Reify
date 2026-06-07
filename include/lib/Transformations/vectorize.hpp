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

// Contains Transformation Rules that attempt to cause Vectorizations in the compiler

#include "lib/rule.hpp"
#include "lib/lang.hpp"

#ifndef REIFY_VECTORIZE_HPP
#define REIFY_VECTORIZE_HPP

namespace transformations::vectorize {

  // Notation:
  // C1, C2, ... := constants/Literals
  // E1, E2, ... := (Sub)Expression
  // S1, S2, ... := statement (e.g. Assign, For, While or if)
  // B1, B2, ... := Conditional Stmt
  // {A, ..., Z, a, ..., z} Variables

  /// Copies an assignment into a new variable
  /// x = E1 => dx = E1; x = E1
  struct DeadAssignFromCopy : Rule {
    bool match(const symir::Stmt *stmt) const override;
    void rewrite(
      symir::FunctBuilder *funBd,
      std::vector<symir::BlockBuilder *> &blockBds,
      VariableState &varState,
      size_t targetBlockIdx,
      size_t targetStmtIdx
    ) const override;
    std::string RuleName() override { return __CLASS_NAME__ };
    const std::string varPrefix = "dead_assign";
  };



  /// https://llvm.org/docs/Vectorizers.html#reductions
  struct Reduction : Rule {
    bool match(const symir::Stmt *stmt) const override;
    void rewrite(
      symir::FunctBuilder *funBd,
      std::vector<symir::BlockBuilder *> &blockBds,
      VariableState &varState,
      size_t targetBlockIdx,
      size_t targetStmtIdx
    ) const override;
    std::string RuleName() override { return __CLASS_NAME__ };
    const std::string varPrefix = "reduc";
    const std::string indVarPrefix = "i";
  };

  /// https://llvm.org/docs/Vectorizers.html#inductions
  struct Induction: Rule {
    bool match(const symir::Stmt *stmt) const override;
    void rewrite(
      symir::FunctBuilder *funBd,
      std::vector<symir::BlockBuilder *> &blockBds,
      VariableState &varState,
      size_t targetBlockIdx,
      size_t targetStmtIdx
    ) const override;
    std::string RuleName() override { return __CLASS_NAME__ };
    const std::string varPrefix = "induc";
    const std::string indVarPrefix = "i";
  };

  /// Trying to create Runtime Checks of Pointers
  /// https://llvm.org/docs/Vectorizers.html#runtime-checks-of-pointers
  /// x = c1 + ... + c2 + ... + c3 + ...
  /// ==>
  /// int a[X] = { ..., c1, ..., c2, ..., c3 };
  /// for (int i = 0; i < v0; i += 1) {
  ///   a[i] = a[i + v1];
  /// }
  /// x = a[k1] + ... + a[k2] + ... + a[k3] + ...
  struct WithAliasCheck : Rule {
    bool match(const symir::Stmt *stmt) const override;
    void rewrite(
      symir::FunctBuilder *funBd,
      std::vector<symir::BlockBuilder *> &blockBds,
      VariableState &varState,
      size_t targetBlockIdx,
      size_t targetStmtIdx
    ) const override;
    std::string RuleName() override { return __CLASS_NAME__ };
    const std::string varPrefix = "aliasCheck";
    const std::string indVarPrefix = "i";
  };
}

#endif //REIFY_VECTORIZE_HPP
