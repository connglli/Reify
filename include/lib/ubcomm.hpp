// MIT License
//
// Copyright (c) 2026
//
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

#ifndef REIFY_UBCOMM_HPP
#define REIFY_UBCOMM_HPP

#include <optional>
#include <string>
#include <vector>

#include "lib/lang.hpp"

/// The kinds of undefined behaviors supported for targeted injection.
enum class UBKind {
  SIGNED_ADD_OVERFLOW,
  SIGNED_SUB_OVERFLOW,
  SIGNED_MUL_OVERFLOW,
  ARRAY_OUT_OF_BOUND,
  DIVISION_BY_ZERO,
  SIGNED_DIV_OVERFLOW,
  REMAINDER_BY_ZERO,
  REMAINDER_OVERFLOW,
};

/// Helper to convert a UBKind enum to its corresponding string representation.
inline std::string ubkind_to_string(UBKind kind) {
  switch (kind) {
    case UBKind::SIGNED_ADD_OVERFLOW:
      return "signed_add_overflow";
    case UBKind::SIGNED_SUB_OVERFLOW:
      return "signed_sub_overflow";
    case UBKind::SIGNED_MUL_OVERFLOW:
      return "signed_mul_overflow";
    case UBKind::ARRAY_OUT_OF_BOUND:
      return "array_out_of_bound";
    case UBKind::DIVISION_BY_ZERO:
      return "division_by_zero";
    case UBKind::SIGNED_DIV_OVERFLOW:
      return "signed_division_overflow";
    case UBKind::REMAINDER_BY_ZERO:
      return "remainder_by_zero";
    case UBKind::REMAINDER_OVERFLOW:
      return "remainder_overflow";
  }
  return "unknown";
}

/// Represents the precise location and kind of undefined behavior to be injected.
struct UBSite {
  std::string blockLabel; ///< The basic block label where the UB should occur.
  int stmtIndex;          ///< The 0-based statement index inside the basic block.
  UBKind kind;            ///< The type of UB to trigger.
};

/// A visitor that scans the statements on the execution path to discover all possible
/// candidates for targeted undefined behavior injection.
class UBCandCollector : public symir::SymIRVisitor {
public:
  UBCandCollector(const symir::Funct &fun, const std::vector<std::string> &execution) :
      fun(fun), execution(execution) {}

  /// Scans the execution path and returns a list of all identified UB candidate locations and
  /// kinds.
  std::vector<UBSite> Collect() {
    candidates.clear();
    for (const auto &label: execution) {
      currBbl = label;
      currentStmtIdx = 0;
      const auto *block = fun.FindBlock(label);
      if (block) {
        block->Accept(*this);
      }
    }
    return candidates;
  }

  void Visit(const symir::VarUse &v) override {
    const auto *varDef = v.GetDef();
    if (!varDef)
      return;

    symir::SymIR::Type currType = varDef->GetType();
    symir::SymIR::Type currBaseType = varDef->GetBaseType();
    std::string currStruct =
        (currType == symir::SymIR::Type::STRUCT)
            ? varDef->GetStructName()
            : (currBaseType == symir::SymIR::Type::STRUCT ? varDef->GetStructName() : "");
    std::vector<int> currShape = varDef->GetVecShape();
    size_t currentShapeIdx = 0;

    const auto &access = v.GetAccess();
    for (size_t i = 0; i < access.size(); ++i) {
      if (currentShapeIdx < currShape.size()) {
        // Array access
        hasArrayUse = true;
        currentShapeIdx++;
      } else if (currType == symir::SymIR::Type::STRUCT && !currStruct.empty()) {
        // Struct access
        const auto *cstTerm = dynamic_cast<const symir::Term *>(access[i]);
        if (cstTerm && cstTerm->GetOp() == symir::Term::Op::OP_CST) {
          std::string fieldName = "f" + cstTerm->GetCoef()->GetName();
          const auto *structDef = fun.GetStruct(currStruct);
          if (structDef) {
            int fieldIdx = structDef->GetFieldIndex(fieldName);
            if (fieldIdx != -1) {
              const auto &field = structDef->GetField(fieldIdx);
              currType = field.type;
              currBaseType = field.baseType;
              currStruct = field.structName;
              currShape = field.shape;
              currentShapeIdx = 0;
            }
          }
        }
      }
    }
  }

  void Visit(const symir::Coef &c) override {}

  void Visit(const symir::Term &t) override {
    if (t.GetOp() == symir::Term::Op::OP_ADD) {
      hasAdd = true;
    } else if (t.GetOp() == symir::Term::Op::OP_SUB) {
      hasSub = true;
    } else if (t.GetOp() == symir::Term::Op::OP_MUL) {
      hasMul = true;
    } else if (t.GetOp() == symir::Term::Op::OP_DIV) {
      hasDiv = true;
    } else if (t.GetOp() == symir::Term::Op::OP_REM) {
      hasRem = true;
    }
    if (t.GetVar()) {
      t.GetVar()->Accept(*this);
    }
  }

  void Visit(const symir::Expr &e) override {
    if (e.GetOp() == symir::Expr::Op::OP_ADD && e.NumTerms() > 1) {
      hasAdd = true;
    } else if (e.GetOp() == symir::Expr::Op::OP_SUB && e.NumTerms() > 1) {
      hasSub = true;
    }
    for (const auto *term: e.GetTerms()) {
      term->Accept(*this);
    }
  }

  void Visit(const symir::Cond &c) override {
    if (c.GetExpr() != nullptr) {
      c.GetExpr()->Accept(*this);
    }
  }

  void Visit(const symir::AssStmt &a) override {
    hasAdd = false;
    hasSub = false;
    hasMul = false;
    hasDiv = false;
    hasRem = false;
    hasArrayUse = false;

    a.GetVar()->Accept(*this);
    a.GetExpr()->Accept(*this);


    if (hasAdd) {
      candidates.push_back({currBbl, currentStmtIdx, UBKind::SIGNED_ADD_OVERFLOW});
    }
    if (hasSub) {
      candidates.push_back({currBbl, currentStmtIdx, UBKind::SIGNED_SUB_OVERFLOW});
    }
    if (hasMul) {
      candidates.push_back({currBbl, currentStmtIdx, UBKind::SIGNED_MUL_OVERFLOW});
    }
    if (hasArrayUse) {
      candidates.push_back({currBbl, currentStmtIdx, UBKind::ARRAY_OUT_OF_BOUND});
    }
    if (hasDiv) {
      candidates.push_back({currBbl, currentStmtIdx, UBKind::DIVISION_BY_ZERO});
      candidates.push_back({currBbl, currentStmtIdx, UBKind::SIGNED_DIV_OVERFLOW});
    }
    if (hasRem) {
      candidates.push_back({currBbl, currentStmtIdx, UBKind::REMAINDER_BY_ZERO});
      candidates.push_back({currBbl, currentStmtIdx, UBKind::REMAINDER_OVERFLOW});
    }
  }

  void Visit(const symir::RetStmt &r) override {}

  void Visit(const symir::Branch &b) override {
    if (b.GetCond() != nullptr) {
      b.GetCond()->Accept(*this);
    }
  }

  void Visit(const symir::Goto &g) override {}

  void Visit(const symir::ScaParam &p) override {}

  void Visit(const symir::VecParam &p) override {}

  void Visit(const symir::StructParam &p) override {}

  void Visit(const symir::ScaLocal &l) override {}

  void Visit(const symir::VecLocal &l) override {}

  void Visit(const symir::StructLocal &l) override {}

  void Visit(const symir::StructDef &s) override {}

  void Visit(const symir::Block &b) override {
    for (const auto &stmt: b.GetStmts()) {
      stmt->Accept(*this);
      currentStmtIdx++;
    }
  }

  void Visit(const symir::Funct &f) override {}

private:
  const symir::Funct &fun;
  const std::vector<std::string> &execution;
  std::string currBbl;
  int currentStmtIdx = 0;
  std::vector<UBSite> candidates;

  bool hasAdd = false;
  bool hasSub = false;
  bool hasMul = false;
  bool hasDiv = false;
  bool hasRem = false;
  bool hasArrayUse = false;
};

#endif // REIFY_UBCOMM_HPP
