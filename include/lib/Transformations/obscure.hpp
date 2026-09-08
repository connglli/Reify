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

// Contains Transformation Rules that do not target any specific Compiler optimization but introduce/create operations from constants

#include "lib/rule.hpp"
#include "lib/lang.hpp"

#ifndef REIFY_OBSCURE_HPP
#define REIFY_OBSCURE_HPP

namespace transformations::obscure {
  
  // Notation:
  // C1, C2, ... := constants/Literals
  // E1, E2, ... := (Sub)Expression
  // S1, S2, ... := statement (e.g. Assign, For, While or if)
  // B1, B2, ... := Conditional Stmt
  // {A, ..., Z, a, ..., z} Variables

  struct PrimeInterp : Rule {
    PrimeInterp(int32_t prime = 46337) : prime(prime) {}
    bool Match(const symir::Stmt *stmt) const override;
    void Rewrite(
      symir::FunctBuilder *funBd,
      std::vector<symir::BlockBuilder *> &blockBds,
      VariableState &varState,
      size_t targetBlockIdx,
      size_t targetStmtIdx
    ) const override;
    std::string RuleName() override { return __CLASS_NAME__ };
    int32_t prime;
    const std::string varPrefix = "prime_interpol_guard";
  };


  struct Conditional : Rule {
    Conditional() {}
    bool Match(const symir::Stmt *stmt) const override;
    void Rewrite(
      symir::FunctBuilder *funBd,
      std::vector<symir::BlockBuilder *> &blockBds,
      VariableState &varState,
      size_t targetBlockIdx,
      size_t targetStmtIdx
    ) const override;
    std::string RuleName() override { return __CLASS_NAME__ };
    const std::string varPrefix = "conditional_guard";
  };

}

#endif
