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
#include "lib/lang.hpp"
#include "lib/random.hpp"
#include "lib/patternmatch.hpp"
#include <climits>
#include <utility>

namespace transformations::utils {

  template class StmtReplacer<symir::Expr>;
  template class StmtReplacer<symir::Term>;
  template class StmtReplacer<symir::Cond>;

  template<typename Node>
  void StmtReplacer<Node>::Visit(const Node &n) { StmtCopier::Visit(n); }
  
  template<typename Node>
  void StmtReplacer<Node>::ReplaceStmt(
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
    bool wasTarget = s->GetIRId() == symir::SymIR::SIR_TGT_GOTO || s->GetIRId() == symir::SymIR::SIR_TGT_BRA;
    size_t idx;
    for (idx = 0; idx < this->blockBd->GetNumberOfCommitedStmt(); idx++) {
      if (this->blockBd->GetCommitedStmt(idx) == s) break;
    }

    s->Accept(*this);
    // update stmt ptr since it has been replaced
    s = this->blockBd->GetCommitedStmtOrTarget(idx);
  
    if (!this->hasReplaced) {
      // if by change (e.g. randTheshold) we have not replaced anything we run it again with a threshold of 1 to guarentee a replacement
      if (!wasTarget) popStmt();
      this->randThreshold = 1;
      s->Accept(*this);
    }
  
    Assert(this->hasReplaced, "StmtReplacer should only be called on stmt that are guaranteed to be able to be replaced");

    if (!wasTarget) {
      // If s is not a target we want to Replace s with our new Stmt manually
      this->blockBd->ReplaceCommitStmt({ popStmt() }, idx);
    }
  
    return;
  }
  
  template<typename Node>
  void StmtReplacer<Node>::Visit(const symir::Branch &b) {
    b.GetCond()->Accept(*this);
    auto condId = popCond();
    std::string tt = b.GetTrueTarget();
    std::string ft = b.GetFalseTarget();
    this->blockBd->RemoveTarget();
    this->blockBd->SymBranch(tt, ft, condId);
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

  symir::BlockBuilder::CondID triviallyCondFor(bool condTarget, symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd) {
    return blockBd->SymCond(
      symir::Cond::OP_EQZ,
      blockBd->SymAddExpr({
        blockBd->SymCstTerm(funBd->SymI32Const(condTarget ? 0 : 1), nullptr)
      })
    );
  }

  symir::BlockBuilder::StmtID trivialAssignment(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::VarDef *var,
    std::vector<symir::Coef *> access
  ) {
    return blockBd->SymAssStmt(
      var,
      blockBd->SymAddExpr({
        blockBd->SymCstTerm(
          funBd->SymI32Const(Random::Get().Uniform(INT_MIN, INT_MAX)()),
          nullptr
        )
      }),
      access
    );
  }

  symir::BlockBuilder * splitBlockAt(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    ::std::string secondLabel,
    size_t splitIdx
  ) {
    Assert(splitIdx < blockBd->GetNumberOfCommitedStmt(), "splitIdx out of bounds for block %s", blockBd->GetLabel().c_str());

    symir::BlockBuilder *secondBlockBd = funBd->OpenBlock(secondLabel);
    symir::StmtCopier secondCopier = symir::StmtCopier(funBd, secondBlockBd);

    for (size_t i = 0; i < blockBd->GetNumberOfCommitedStmt(); i++) {
      if (i > splitIdx) {
        secondBlockBd->CommitStmt(
            secondCopier.CopyStmt(blockBd->GetCommitedStmt(i))
        );
      }
    }

    symir::Target *target = blockBd->GetTarget();
    if (target != nullptr) {
      if (target->GetIRId() == symir::SymIR::SIR_TGT_GOTO) {
        secondBlockBd->SymGoto(static_cast<symir::Goto *>(target)->GetTarget());
      } else if (target->GetIRId() == symir::SymIR::SIR_TGT_BRA) {
        symir::Branch *branch = static_cast<symir::Branch *>(target);
        secondBlockBd->SymBranch(
          branch->GetTrueTarget(),
          branch->GetFalseTarget(),
          secondCopier.CopyCond(branch->GetCond())
        );
      }
      blockBd->RemoveTarget();
    }

    if (splitIdx < blockBd->GetNumberOfCommitedStmt() - 1) {
      blockBd->RemoveCommittedStmts(splitIdx + 1, blockBd->GetNumberOfCommitedStmt());
    }

    return secondBlockBd;
  }

  // TODO: This could be optimized if perf. becomes an issue
  void insertBlockBd(
    std::vector<symir::BlockBuilder *> &currBlockBds,
    std::vector<symir::BlockBuilder *> newBlocks,
    size_t index
  ) {
    int currIndex = index;
    for (size_t i = 0; i < newBlocks.size(); i++) {
      currBlockBds.insert(currBlockBds.begin() + currIndex++, std::move(newBlocks[i]));
    }
  }

  std::string nameLabel(std::string functName, std::string prefix) {
    static std::map<std::pair<std::string, std::string>, size_t> nameCount;
    std::pair namePair = std::make_pair(functName, prefix);
    if (!nameCount.contains(namePair)) nameCount[namePair] = 0;
    return prefix + "_" + std::to_string(nameCount[namePair]++);
  }

  std::string nameVariable(std::string domBlockName, std::string prefix) {
    static std::map<std::pair<std::string, std::string>, size_t> nameCount;
    std::pair namePair = std::make_pair(domBlockName, prefix);
    if (!nameCount.contains(namePair)) nameCount[namePair] = 0;
    return prefix + "_" + std::to_string(nameCount[namePair]++);
  }

  using namespace patternmatch;
  bool matchSubExprInAnyStmt(const symir::Stmt *stmt, const Pattern<const symir::Expr *> &E) {
    return patternmatch::match(
      stmt, 
      m_Or<const symir::Stmt *>(
        m_Branch(
          m_Cond(E),
          m_WildCard<const std::string>(),
          m_WildCard<const std::string>()
        ),
        m_AssStmt(
          m_WildCard<const symir::VarUse *>(),
          E
        )
      )
    );
  }
} // namespace
