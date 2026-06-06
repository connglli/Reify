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
namespace transformations::instcombine {
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
      [](const symir::Expr *P) {
        int C1, C2;
        bool match = patternmatch::match(
          P,
          m_AddExpr(
            m_AnyTwoSeq(m_CstTerm(m_Value(m_Int(&C1)), m_NoVar()), m_AndTerm(m_Value(m_Int(&C2)), m_Var()))
          )
        );
        return match && utils::noSubOverflow(C1, ~C2);
      },
      varInsertFun,
      1
    );

    blockBd->CommitStmtAt(blockBd->SymAssStmt(A, blockBd->SymExpr(
      utils::randomExprOp(),
      { blockBd->SymCstTerm(funBd->SymI32Const(AVal), nullptr) }
    )), targetStmtIdx);
  }

  bool ShlToAddTwice::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(
      stmt, 
      m_AddExpr(m_AnyAfter(0, m_ShlTerm(m_Eq<symir::Coef *, int32_t>(1), m_Var())))
    );
  }

  void ShlToAddTwice::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running AddTwiceToShl" << std::endl;
    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);

    auto rep = utils::StmtReplacer<symir::Expr>(funBd, blockBd);

    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder * ,symir::BlockBuilder *, const symir::Expr &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Expr &e, void **data) {
          std::vector<symir::BlockBuilder::TermID> termIds;
          auto terms = e.GetTerms();
          termIds.reserve(terms.size() + 1);
          bool hasReplaced = false;
          auto c = symir::StmtCopier(thisFunBd, thisBlockBd);
          for (size_t i = 0; i < terms.size(); i++) {
            const symir::Term *term = terms[i];
            const symir::VarUse *RHS;
            if (
              !hasReplaced &&
              i > 0 &&
              patternmatch::match(term, m_ShlTerm(m_Eq<symir::Coef *, int32_t>(1), m_Var(&RHS)))
            ) {
              hasReplaced = true;
              auto accessRHS = utils::copyAccess(thisFunBd, RHS);
              termIds.push_back(utils::variableTerm(thisFunBd, thisBlockBd, RHS->GetDef(), accessRHS));
              termIds.push_back(utils::variableTerm(thisFunBd, thisBlockBd, RHS->GetDef(), accessRHS));
            } else {
              termIds.push_back(symir::StmtCopier(thisFunBd, thisBlockBd).CopyTerm(term));
            }
          }
          return thisBlockBd->SymExpr(e.GetOp(), termIds);
        };
  
    rep.ReplaceStmt(
      stmt,
      make_matcher(
        const symir::Expr *,
        m_AddExpr(
          m_AddExpr(m_AnyAfter(0, m_ShlTerm(m_Eq<symir::Coef *, int32_t>(1), m_Var())))
        )
      ),
      varInsertFun,
      1
    );
  }

  bool OrToAddAndXor::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(
      stmt,
      m_Expr(
        m_Any(m_OrTerm(m_Solved(), m_Var()))
      )
    );
  }

  void OrToAddAndXor::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running OrToAddAndXor" << std::endl;
    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);

    auto rep = utils::StmtReplacer<symir::Term>(funBd, blockBd);
    symir::BlockBuilder::ExprID AExprId;
    rep.data = &AExprId;

    const symir::VarDef *A = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix);

    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder * ,symir::BlockBuilder *, const symir::Term &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Term &t, void **data) {
          const symir::VarUse *B;
          symir::Coef *C;
          Assert(patternmatch::match(&t, m_OrTerm(m_Solved(&C), m_Var(&B))), "This should be guarenteed");
          auto accessB = utils::copyAccess(thisFunBd, B);
          symir::BlockBuilder::ExprID *AExprIdPtr = static_cast<symir::BlockBuilder::ExprID *>(*data);
          switch (Random::Get().Uniform(0, 1)()) {
          case 0: {
            // xor then and version
            *AExprIdPtr = thisBlockBd->SymAddExpr({
              thisBlockBd->SymXorTerm(C, B->GetDef(), accessB),
              thisBlockBd->SymAndTerm(C, B->GetDef(), accessB)
            });
          } break;
          case 1: {
            // and then xor version
            *AExprIdPtr = thisBlockBd->SymAddExpr({
              thisBlockBd->SymAndTerm(C, B->GetDef(), accessB),
              thisBlockBd->SymXorTerm(C, B->GetDef(), accessB)
            });
          } break;
          default: Panic("should not reach here");
          }
          return utils::variableTerm(thisFunBd, thisBlockBd, A);
        };
  
    rep.ReplaceStmt(
      stmt,
      make_matcher(
        const symir::Term *,
        m_OrTerm(m_Solved(), m_Var())
      ),
      varInsertFun,
      1
    );

    blockBd->CommitStmtAt(blockBd->SymAssStmt(A, AExprId), targetStmtIdx);
  }

  bool AddToAddOrAnd::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(
      stmt,
      m_Expr(
        m_Any(m_AddTerm(m_Solved(), m_Var()))
      )
    );
  }

  void AddToAddOrAnd::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running AddToAddOrAnd" << std::endl;
    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);

    auto rep = utils::StmtReplacer<symir::Term>(funBd, blockBd);
    symir::BlockBuilder::ExprID AExprId;
    rep.data = &AExprId;

    const symir::VarDef *A = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix);

    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder * ,symir::BlockBuilder *, const symir::Term &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Term &t, void **data) {
          const symir::VarUse *B;
          symir::Coef *C;
          Assert(patternmatch::match(&t, m_AddTerm(m_Solved(&C), m_Var(&B))), "This should be guarenteed");
          auto accessB = utils::copyAccess(thisFunBd, B);
          symir::BlockBuilder::ExprID *AExprIdPtr = static_cast<symir::BlockBuilder::ExprID *>(*data);
          switch (Random::Get().Uniform(0, 1)()) {
          case 0: {
            // or then and version
            *AExprIdPtr = thisBlockBd->SymAddExpr({
              thisBlockBd->SymOrTerm(C, B->GetDef(), accessB),
              thisBlockBd->SymAndTerm(C, B->GetDef(), accessB)
            });
          } break;
          case 1: {
            // and then or version
            *AExprIdPtr = thisBlockBd->SymAddExpr({
              thisBlockBd->SymAndTerm(C, B->GetDef(), accessB),
              thisBlockBd->SymOrTerm(C, B->GetDef(), accessB)
            });
          } break;
          default: Panic("should not reach here");
          }
          return utils::variableTerm(thisFunBd, thisBlockBd, A);
        };
  
    rep.ReplaceStmt(
      stmt,
      make_matcher(
        const symir::Term *,
        m_AddTerm(m_Solved(), m_Var())
      ),
      varInsertFun,
      1
    );

    blockBd->CommitStmtAt(blockBd->SymAssStmt(A, AExprId), targetStmtIdx);
  }

  bool AndToSubOrXor::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(
      stmt,
      m_Expr(
        m_Any(m_AndTerm(m_Solved(), m_Var()))
      )
    );
  }

  void AndToSubOrXor::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running AndToSubOrXor" << std::endl;
    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);

    auto rep = utils::StmtReplacer<symir::Term>(funBd, blockBd);
    symir::BlockBuilder::ExprID AExprId;
    rep.data = &AExprId;

    const symir::VarDef *A = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix);

    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder * ,symir::BlockBuilder *, const symir::Term &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Term &t, void **data) {
            const symir::VarUse *B;
            symir::Coef *C;
              Assert(patternmatch::match(&t, m_AndTerm(m_Solved(&C), m_Var(&B))), "This should be guarenteed");
              auto accessB = utils::copyAccess(thisFunBd, B);
              symir::BlockBuilder::ExprID *AExprIdPtr = static_cast<symir::BlockBuilder::ExprID *>(*data);
              *AExprIdPtr = thisBlockBd->SymSubExpr({
                thisBlockBd->SymOrTerm(C, B->GetDef(), accessB),
                thisBlockBd->SymXorTerm(C, B->GetDef(), accessB)
              });
              return utils::variableTerm(thisFunBd, thisBlockBd, A);
        };
  
    rep.ReplaceStmt(
      stmt,
      make_matcher(
        const symir::Term *,
        m_AndTerm(m_Solved(), m_Var())
      ),
      varInsertFun,
      1
    );

    blockBd->CommitStmtAt(blockBd->SymAssStmt(A, AExprId), targetStmtIdx);
  }

  bool XorToSubOrAnd::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(
      stmt,
      m_Expr(
        m_Any(m_XorTerm(m_Solved(), m_Var()))
      )
    );
  }

  void XorToSubOrAnd::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running XorToSubOrAnd" << std::endl;
    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);

    auto rep = utils::StmtReplacer<symir::Term>(funBd, blockBd);
    symir::BlockBuilder::ExprID AExprId;
    rep.data = &AExprId;

    const symir::VarDef *A = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix);

    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder * ,symir::BlockBuilder *, const symir::Term &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Term &t, void **data) {
          const symir::VarUse *B;
          symir::Coef *C;
          Assert(patternmatch::match(&t, m_XorTerm(m_Solved(&C), m_Var(&B))), "This should be guarenteed");
          auto accessB = utils::copyAccess(thisFunBd, B);
          symir::BlockBuilder::ExprID *AExprIdPtr = static_cast<symir::BlockBuilder::ExprID *>(*data);
          *AExprIdPtr = thisBlockBd->SymSubExpr({
            thisBlockBd->SymOrTerm(C, B->GetDef(), accessB),
            thisBlockBd->SymAndTerm(C, B->GetDef(), accessB)
          });
          return utils::variableTerm(thisFunBd, thisBlockBd, A);
        };
  
    rep.ReplaceStmt(
      stmt,
      make_matcher(
        const symir::Term *,
        m_XorTerm(m_Solved(), m_Var())
      ),
      varInsertFun,
      0.25
    );

    blockBd->CommitStmtAt(blockBd->SymAssStmt(A, AExprId), targetStmtIdx);
  }

  bool AndToSubAndAnd::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(
      stmt,
      m_Expr(
        m_Any(m_XorTerm(m_Solved(), m_Var()))
      )
    );
  }

  void AndToSubAndAnd::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running AndToSubAndAnd" << std::endl;
    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);

    auto rep = utils::StmtReplacer<symir::Term>(funBd, blockBd);
    symir::BlockBuilder::ExprID AExprId;
    rep.data = &AExprId;

    const symir::VarDef *A = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix);

    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder * ,symir::BlockBuilder *, const symir::Term &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Term &t, void **data) {
          const symir::VarUse *B;
          symir::Coef *C;
          Assert(patternmatch::match(&t, m_XorTerm(m_Solved(&C), m_Var(&B))), "This should be guarenteed");
          auto accessB = utils::copyAccess(thisFunBd, B);
          symir::BlockBuilder::ExprID *AExprIdPtr = static_cast<symir::BlockBuilder::ExprID *>(*data);
          *AExprIdPtr = thisBlockBd->SymSubExpr({
            thisBlockBd->SymOrTerm(C, B->GetDef(), accessB),
            thisBlockBd->SymAndTerm(C, B->GetDef(), accessB)
          });
          return utils::variableTerm(thisFunBd, thisBlockBd, A);
        };
  
    rep.ReplaceStmt(
      stmt,
      make_matcher(
        const symir::Term *,
        m_XorTerm(m_Solved(), m_Var())
      ),
      varInsertFun,
      0.25
    );

    blockBd->CommitStmtAt(blockBd->SymAssStmt(A, AExprId), targetStmtIdx);
  }


}
