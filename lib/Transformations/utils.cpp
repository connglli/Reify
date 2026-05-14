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

#include "lib/Transformations/utils.hpp"
#include "lib/random.hpp"
#include "lib/patternmatch.hpp"

namespace transformations::utils {

  template class StmtReplacer<symir::Expr>;
  template class StmtReplacer<symir::Term>;
  template class StmtReplacer<symir::Cond>;

  template<typename Node>
  void StmtReplacer<Node>::Visit(const Node &n) { StmtCopier::Visit(n); }
  
  template<typename Node>
  symir::BlockBuilder::StmtID StmtReplacer<Node>::CopyStmtWithReplacement(
    const symir::Stmt *s, 
    std::function<bool(const Node *)> matchFunction,
    std::function<ExprID(symir::FunctBuilder *, symir::BlockBuilder *, const Node &, void **)> replaceFunction,
    double randThreshold
  ) {
    this->matchFunction = matchFunction;
    this->replaceFunction = replaceFunction;
    this->randThreshold = randThreshold;
    this->randUniform = Random::Get().UniformReal();
    this->hasReplaced = false;
  
    s->Accept(*this);
  
    if (!this->hasReplaced) {
      // if by change (e.g. randTheshold) we have not replaced anything we run it again with a threshold of 1 to guarentee a replacement
      popStmt();
      this->randThreshold = 1;
      s->Accept(*this);
    }
  
    Assert(this->hasReplaced, "StmtReplacer should only be called on stmt that are guaranteed to be able to be replaced");
  
    return popStmt();
  }
  
  template<> void StmtReplacer<symir::Expr>::Visit(const symir::Expr &e) {
    if (this->match(e)) {
      pushExpr(this->replace(e));
    } else {
      StmtCopier::Visit(e);
    }
  }
  
  template<> void StmtReplacer<symir::Term>::Visit(const symir::Term &t) {
    if (this->match(t)) {
      pushTerm(this->replace(t));
    } else {
      StmtCopier::Visit(t);
    }
  }
  
  template<> void StmtReplacer<symir::Cond>::Visit(const symir::Cond &c) {
    if (this->match(c)) {
      pushCond(this->replace(c));
    } else {
      StmtCopier::Visit(c);
    }
  }

  std::vector<symir::Coef *> copyAccess(symir::FunctBuilder *funBd, const symir::VarUse *use) {
    std::vector<symir::Coef *> access;
    access.reserve(use->GetAccess().size());
    for (const auto &coef : use->GetAccess()) {
      if (auto c = funBd->FindSymbol(coef->GetName()); c != nullptr) {
        Assert(
            typeid(*coef) == typeid(symir::Coef),
            "Symbol \"%s\" is already defined and is not a coefficient", coef->GetName().c_str()
        );
        access.push_back(dynamic_cast<symir::Coef *>(coef));
      } else {
        Panic("coeff not found in provided function builder");
      }
    }
    return access;
  }

  symir::BlockBuilder::CondID triviallyFalseCond(symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd) {
    return blockBd->SymCond(
      symir::Cond::OP_EQZ,
      blockBd->SymAddExpr({
        blockBd->SymCstTerm(funBd->SymI32Const(1), nullptr)
      })
    );
  }

  symir::BlockBuilder::CondID triviallyTrueCond(symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd) {
    return blockBd->SymCond(
      symir::Cond::OP_EQZ,
      blockBd->SymAddExpr({
        blockBd->SymCstTerm(funBd->SymI32Const(0), nullptr)
      })
    );
  }

  // match helpers:
  using namespace patternmatch;
  bool matchSubExprInAnyStmt(const symir::Stmt *stmt, const Pattern<const symir::Expr *> &E) {
    return match(
      stmt,
      m_AnyStmt(
        m_WildCard<const symir::VarUse *>(),
        m_WildCard<std::vector<const symir::VarUse *>>(),
        E,
        m_Cond(E),
        m_Any(m_Cond(E)),
        m_WildCard<const symir::ModExpr *>(),
        m_Any(m_AssStmt(
          m_WildCard<const symir::VarUse *>(),
          E
        )),
        m_Any<std::vector<const symir::Stmt *>>(m_Any(m_AssStmt(
          m_WildCard<const symir::VarUse *>(),
          E
        )))
      )
    );
  }
} // namespace
