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

// Contains Transformation Rules that attempt to cause InstCombine passes in the compiler


#ifndef REIFY_INSTCOMBINE_HPP
#define REIFY_INSTCOMBINE_HPP

#include <lib/rule.hpp>

  // Notation:
  // C1, C2, ... := constants/Literals
  // E1, E2, ... := (Sub)Expression
  // S1, S2, ... := statement (e.g. Assign, For, While or if)
  // B1, B2, ... := Conditional Stmt
  // {A, ..., Z, a, ..., z} Variables

namespace transformations::instcombine {

  // targeting the LLVM transformation: (A + C) + (B & ~C) => A + (B | C)
  // Note A + (B | C) => (A + C) + (B & ~C) may be unsafe with counter example:
  // A = -999955844
  // B = 1073766401
  // C = -1608775916
  // (A + C) overflows, but A + (B | C) does not
  // Hence alternative inverse transformation is needed:
  // C1 + (C2 & B) => A = C1 - ~C2; (C + A) + (~C & B) + E2
  // where C = ~C2
  struct FoldAddLikeCommutative : Rule {
    bool match(const symir::Stmt *stmt) const override;
    void rewrite(
      symir::FunctBuilder *funBd,
      std::vector<symir::BlockBuilder *> &blockBds,
      VariableState &varState,
      size_t targetBlockIdx,
      size_t targetStmtIdx
    ) const override;
    std::string varPrefix = "fold_add_like_commutative";
  };

}

#endif //REIFY_INSTCOMBINE_HPP
