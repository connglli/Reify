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

#include "lib/patternmatch.hpp"
#include "lib/Transformations/instcombine.hpp"
#include "lib/Transformations/utils.hpp"
#include "lib/logger.hpp"
#include <climits>
#include <iostream>


using namespace patternmatch;
namespace transformations::instcombine{
  bool FoldAddLikeCommutative::match(const symir::Stmt *stmt) const {
    int C1, C2;
    bool match = utils::matchSubExprInAnyStmt(
      stmt,
      m_AddExpr(
        m_AnyTwoSeq(m_CstTerm(m_Value(m_Int(&C1)), m_NoVar()), m_AndTerm(m_Value(m_Int(&C2)), m_Var()))
      )
    );
    return match && utils::noSubOverflow(C1, ~C2);
  }

  void FoldAddLikeCommutative::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running FoldAddLikeCommutative" << std::endl;
    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);

    auto rep = utils::StmtReplacer<symir::Expr>(funBd, blockBd);
    int AVal;
    rep.data = &AVal;

    const symir::VarDef *A = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix);

    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder * ,symir::BlockBuilder *, const symir::Expr &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Expr &e, void **data) {
          std::vector<symir::BlockBuilder::TermID> termIds;
          auto terms = e.GetTerms();
          bool hasReplaced = false;
          auto c = symir::StmtCopier(thisFunBd, thisBlockBd);
          for (size_t i = 0; i < terms.size() - 1; i++) {
            const symir::Term *term1 = terms[i];
            const symir::Term *term2 = terms[i + 1];
            const symir::VarUse *B;
            int C1, C2;
            if (
              !hasReplaced &&
              patternmatch::match(term1, m_CstTerm(m_Value(m_Int(&C1)), m_NoVar())) &&
              patternmatch::match(term2, m_AndTerm(m_Value(m_Int(&C2)), m_Var(&B))) &&
              utils::noSubOverflow(C1, ~C2)
            ) {
              hasReplaced = true;
              int *AValPtr = static_cast<int *>(*data);
              *AValPtr = C1 - ~C2;
              auto accessB = utils::copyAccess(thisFunBd, B);
              termIds.push_back(thisBlockBd->SymAddTerm(thisFunBd->SymI32Const(~C2), A));
              termIds.push_back(thisBlockBd->SymAndTerm(thisFunBd->SymI32Const(C2), B->GetDef(), accessB));
              i += 1;
              if (i == terms.size() - 2)
                termIds.push_back(symir::StmtCopier(thisFunBd, thisBlockBd).CopyTerm(terms[i + 1]));
            } else if (i == terms.size() - 2){
              termIds.push_back(symir::StmtCopier(thisFunBd, thisBlockBd).CopyTerm(term1));
              termIds.push_back(symir::StmtCopier(thisFunBd, thisBlockBd).CopyTerm(term2));
            } else {
              termIds.push_back(symir::StmtCopier(thisFunBd, thisBlockBd).CopyTerm(term1));
            }
          }
          return thisBlockBd->SymExpr(e.GetOp(), termIds);
        };
  
    rep.ReplaceStmt(
      stmt,
      make_matcher(
        const symir::Expr *,
        m_AddExpr(
          m_AnyAfter(1, m_Not(m_CstTerm(m_WildCard<const symir::Coef *>(), m_WildCard<const symir::VarUse *>())))
        )
      ),
      varInsertFun,
      1
    );

    blockBd->CommitStmtAt(blockBd->SymAssStmt(A, blockBd->SymExpr(
      utils::randomExprOp(),
      { blockBd->SymCstTerm(funBd->SymI32Const(AVal), nullptr) }
    )), targetStmtIdx);
  }
}
