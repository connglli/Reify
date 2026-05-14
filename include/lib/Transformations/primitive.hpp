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

#ifndef REIFY_PRIMITIVE_HPP
#define REIFY_PRIMITIVE_HPP

namespace transformations::primitive {
  
  // Notation:
  // C1, C2, ... := constants/Literals
  // E1, E2, ... := (Sub)Expression
  // S1, S2, ... := statement (e.g. Assign, For, While or if)
  // B1, B2, ... := Conditional Stmt
  // {A, ..., Z, a, ..., z} Variables

  /// Additivly expands an expression with one more element
  /// E1 + C1 + E2 => E1 + C2 + C3 +E2
  /// where C2 + C3 = C1
  struct AdditionFromConst : Rule {
    bool match(const symir::Stmt *stmt) const override;
    std::vector<symir::BlockBuilder::StmtID> rewrite(
      symir::FunctBuilder *funBd,
      symir::BlockBuilder *blockBd,
      const symir::Stmt *stmt
    ) override;
  };

  /// create a For Loop from an assignment of a Const
  /// x = C1 => x = C2; for (i = 0; i < C3; i += 1) { x = C4 + x; }, 
  /// where C3 * C4 + C2 = C1
  struct ForSumFromConst : Rule {
    ForSumFromConst() : Rule("i") {}
    bool match(const symir::Stmt *stmt) const override;
    std::vector<symir::BlockBuilder::StmtID> rewrite(
      symir::FunctBuilder *funBd,
      symir::BlockBuilder *blockBd,
      const symir::Stmt *stmt
    ) override;
  };

  /// Creates If/else stmts from Assignments
  /// x = E1 => if (B1) { x = E2 } else if (B2) { x = E3 } ... else { x = E`n` }
  /// where exactly one or no B1 evaluates to true, if one does evaluate true the corresponding branch contains x = E1, if non are true then the else branch contains x = E1
  struct DeadCodeFromAssign : Rule {
    DeadCodeFromAssign(int minBranches = 2, int maxBranches = 4, bool allowUB = false) :
      minBranches(minBranches), maxBranches(maxBranches), allowUB(allowUB) {
      Assert(minBranches >= 2, "AssToDeadCode must have atleast 2 branches");
    }
    bool match(const symir::Stmt *stmt) const override;
    std::vector<symir::BlockBuilder::StmtID> rewrite(
      symir::FunctBuilder *funBd,
      symir::BlockBuilder *blockBd,
      const symir::Stmt *stmt
    ) override;
  
  private:
    int minBranches;
    int maxBranches;
    bool allowUB;
  };

  /// Extracts a Constant from an expression replacing it with a Variable that is assigned earlier with the extracted Constant
  /// E1 + C1 + E2 => cpk = C1; E1 + cpk + E2
  struct SimpleConstProbagation : Rule {
    SimpleConstProbagation() : Rule("const_proba") {}
    bool match(const symir::Stmt *stmt) const override;
    std::vector<symir::BlockBuilder::StmtID> rewrite(
      symir::FunctBuilder *funBd,
      symir::BlockBuilder *blockBd,
      const symir::Stmt *stmt
    ) override;
  };

} // namespace 

#endif //REIFY_PRIMITIVE_HPP
