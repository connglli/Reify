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

#include "lib/patternmatch.hpp"
#include "lib/Transformations/vectorize.hpp"
#include "lib/Transformations/utils.hpp"
#include "lib/lang.hpp"
#include "lib/logger.hpp"
#include <climits>

using namespace patternmatch;
namespace transformations::vectorize {

  bool DeadAssignFromCopy::match(const symir::Stmt *stmt) const {
    return patternmatch::match(stmt, m_AssStmt(m_WildCard<const symir::VarUse*>(), m_WildCard<const symir::Expr *>()));
  }
  
  void DeadAssignFromCopy::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
    ) const {

    Log::Get().Out() << "Running DeadAssignFromCopy" << std::endl;

    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmt(targetStmtIdx);

    auto copier = symir::StmtCopier(funBd, blockBd);
    symir::BlockBuilder::StmtID origStmt = copier.CopyStmt(stmt);
    symir::BlockBuilder::StmtID deadStmt = blockBd->SymAssStmt(
      utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix),
      copier.CopyExpr(static_cast<const symir::AssStmt *>(stmt)->GetExpr())
    );
    blockBd->ReplaceCommitStmt({ deadStmt, origStmt }, targetStmtIdx);
  }

  bool Reduction::match(const symir::Stmt *stmt) const {
    return patternmatch::match(
      stmt,
      m_AssStmt(m_WildCard<const symir::VarUse*>(), m_Expr(m_Length<const symir::Term *>(m_Range<size_t, size_t>(2, INT_MAX))))
    );
  }

  void Reduction::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {

    Log::Get().Out() << "Running Reduction" << std::endl;

    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmt(targetStmtIdx);

    const symir::AssStmt *assStmt = static_cast<const symir::AssStmt *>(stmt);
    const symir::VarUse *var = assStmt->GetVar();
    std::vector<symir::Coef *> access = utils::copyAccess(funBd, var);
    const symir::VarDef *varDef = var->GetDef();
    const symir::Expr *expr = assStmt->GetExpr();
    std::vector<const symir::Term *> terms = expr->GetTerms();
    symir::Expr::Op exprOp = expr->GetOp();
    size_t nrTerms = expr->NumTerms();

    symir::StmtCopier c = symir::StmtCopier(funBd, blockBd);

    // create an array that can hold all terms;
    const symir::VarDef *array = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix, nrTerms);
    Log::Get().Out() << "Creating array " << array->GetName() << " with " << nrTerms << " elements" << std::endl;


    std::string loopCondLabel = utils::nameLabel(funBd->GetName(), "for_cond");
    std::string loopBodyLabel = utils::nameLabel(funBd->GetName(), "for_body");
    std::string finalLabel = utils::nameLabel(funBd->GetName(), "for_exit");

    symir::BlockBuilder *secondBlockBd = utils::splitBlockAt(funBd, blockBd, finalLabel, targetStmtIdx);

    auto zero = funBd->SymI32Const(0);
    auto one = funBd->SymI32Const(1);
    static size_t uid = 0;
    // loop through all terms and create an array that holds all terms
    std::vector<symir::BlockBuilder::StmtID> firstBlockAppend;
    firstBlockAppend.reserve(2 + nrTerms);
    for (size_t i = 0; i < nrTerms; i++) {
      Log::Get().Out() << "Initalizing element " << i << " of " << array->GetName() << std::endl;
      firstBlockAppend.push_back(blockBd->SymAssStmt(
        array,
        blockBd->SymAddExpr({ c.CopyTerm(terms[i]) } ),
        { funBd->SymI32Const(i) }
      ));
    }
    
    // Init original value to 0
    Log::Get().Out() << "Initalizing " << var->GetName() << " to 0" << std::endl;
    firstBlockAppend.push_back(blockBd->SymAssStmt(
      var->GetDef(),
      blockBd->SymExpr(exprOp, {
        blockBd->SymCstTerm(zero, nullptr),
      }),
      access
    ));

    auto indVar = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->indVarPrefix);
    // indVar = 0
    firstBlockAppend.push_back(blockBd->SymAssStmt(
      indVar,
      blockBd->SymAddExpr({
        blockBd->SymCstTerm(
          zero,
          nullptr
        )
      })
    ));


    blockBd->ReplaceCommitStmt(firstBlockAppend, targetStmtIdx);
    blockBd->SymGoto(loopCondLabel);
    
    // set now freed ptrs to zero to avoid Use after free;
    stmt = nullptr;
    assStmt = nullptr;
    var = nullptr;
    expr = nullptr;
    terms = {};

    // Build for loop Cond block:

    symir::BlockBuilder *loopCondBd = funBd->OpenBlock(loopCondLabel);

    // indVar - loopCount < 0
    symir::BlockBuilder::CondID loopCond = loopCondBd->SymCond(symir::Cond::OP_LTZ, 
      loopCondBd->SymAddExpr({
        loopCondBd->SymMulTerm(
          one,
          indVar
        ),
        loopCondBd->SymCstTerm(
          funBd->SymI32Const(-nrTerms),
          nullptr
        )
      })
    );

    loopCondBd->SymBranch(loopBodyLabel, finalLabel, loopCond);

    // Build for loop Body Block:

    symir::BlockBuilder *loopBodyBd = funBd->OpenBlock(loopBodyLabel);

    // For Body
    symir::BlockBuilder::StmtID arrSum = loopBodyBd->SymAssStmt(
      varDef,
      loopBodyBd->SymExpr(exprOp, {
        loopBodyBd->SymMulTerm(one, varDef, access),
        loopBodyBd->SymMulTerm(one, array, { 
          funBd->SymCoef("__reduction_loopVarCoef" + std::to_string(uid++) + indVar->GetName(), indVar->GetName()) 
        }) 
      }),
      access
    );

    // indVar = 1 + indVar
    symir::BlockBuilder::StmtID incAss = loopBodyBd->SymAssStmt(
      indVar,
      loopBodyBd->SymAddExpr({
        loopBodyBd->SymAddTerm(
          one,
          indVar
        )
      })
    );

    loopBodyBd->CommitStmt(arrSum);
    loopBodyBd->CommitStmt(incAss);
    loopBodyBd->SymGoto(loopCondLabel);

    utils::insertBlockBd(blockBds, { loopCondBd, loopBodyBd, secondBlockBd }, targetBlockIdx + 1);
  }

  bool Induction::match(const symir::Stmt *stmt) const {
    return patternmatch::match(
      stmt,
      m_AssStmt(
        m_WildCard<const symir::VarUse*>(), 
        m_Expr(m_AtleastN(m_CstTerm(
          m_And(
            m_Range<const symir::Coef *, int32_t>(-1024, 1024),
            m_Not<const symir::Coef *>(m_Eq<const symir::Coef*, int32_t>(0))
          ),
          m_NoVar()
        ), 2))
      )
    );
  }

  void Induction::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {

    Log::Get().Out() << "Running Induction" << std::endl;

    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmt(targetStmtIdx);

    const symir::AssStmt *assStmt = static_cast<const symir::AssStmt *>(stmt);
    const symir::VarUse *var = assStmt->GetVar();
    std::vector<symir::Coef *> access = utils::copyAccess(funBd, var);
    const symir::VarDef *varDef = var->GetDef();
    const symir::Expr *expr = assStmt->GetExpr();
    std::vector<const symir::Term *> terms = expr->GetTerms();
    symir::Expr::Op exprOp = expr->GetOp();
    size_t nrTerms = expr->NumTerms();

    int maxCstTerm = 0;
    for (size_t i = 0; i < nrTerms; i++) {
      if (terms[i]->GetOp() != symir::Term::OP_CST) continue;
      int val = abs(terms[i]->GetCoef()->GetI32Value());
      if (!(val <= 1024)) continue;
      if (val > maxCstTerm) maxCstTerm = val;
    }

    symir::StmtCopier c = symir::StmtCopier(funBd, blockBd);

    // create an array that can hold all terms;
    const symir::VarDef *array = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix, maxCstTerm + 1);
    Log::Get().Out() << "Creating array " << array->GetName() << " with " << nrTerms << " elements" << std::endl;


    std::string loopCondLabel = utils::nameLabel(funBd->GetName(), "for_cond");
    std::string loopBodyLabel = utils::nameLabel(funBd->GetName(), "for_body");
    std::string finalLabel = utils::nameLabel(funBd->GetName(), "for_exit");

    symir::BlockBuilder *secondBlockBd = utils::splitBlockAt(funBd, blockBd, finalLabel, targetStmtIdx);

    auto zero = funBd->SymI32Const(0);
    auto one = funBd->SymI32Const(1);
    auto n_one = funBd->SymI32Const(-1);

    // create the new assign stmt at the start of the second half

    symir::StmtCopier sc = symir::StmtCopier(funBd, secondBlockBd);
    std::vector<symir::BlockBuilder::TermID> newTerms;
    newTerms.resize(nrTerms);
    for (size_t i = 0; i < nrTerms; i++) {
      int val;
      if (
        terms[i]->GetOp() == symir::Term::OP_CST
        && (val = terms[i]->GetCoef()->GetI32Value())
        && (-1024 <= val && val <= 1024)
      ) {
        if (val >= 0) newTerms[i] = secondBlockBd->SymMulTerm(one, array, { funBd->SymI32Const(abs(val)) });
        else newTerms[i] = secondBlockBd->SymMulTerm(n_one, array, { funBd->SymI32Const(abs(val)) });
      } else {
        newTerms[i] = sc.CopyTerm(terms[i]);
      }
    }

    secondBlockBd->CommitStmtAt(
      secondBlockBd->SymAssStmt(
        varDef,
        secondBlockBd->SymExpr(
          exprOp,
          newTerms
        ),
        access
      ),
      0
    );

    static size_t uid = 0;

    auto indVar = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->indVarPrefix);
    // indVar = 0
    blockBd->ReplaceCommitStmt({
      blockBd->SymAssStmt(
        indVar,
        blockBd->SymAddExpr({
          blockBd->SymCstTerm(
            zero,
            nullptr
          )
        }))
      },
      targetStmtIdx
    );

    blockBd->SymGoto(loopCondLabel);
    
    // set now freed ptrs to zero to avoid Use after free;
    stmt = nullptr;
    assStmt = nullptr;
    var = nullptr;
    expr = nullptr;
    terms = {};

    // Build for loop Cond block:

    symir::BlockBuilder *loopCondBd = funBd->OpenBlock(loopCondLabel);

    // indVar - maxCstTerm < 0
    symir::BlockBuilder::CondID loopCond = loopCondBd->SymCond(symir::Cond::OP_LTZ, 
      loopCondBd->SymAddExpr({
        loopCondBd->SymMulTerm(
          one,
          indVar
        ),
        loopCondBd->SymCstTerm(
          funBd->SymI32Const(-(maxCstTerm + 1)),
          nullptr
        )
      })
    );

    loopCondBd->SymBranch(loopBodyLabel, finalLabel, loopCond);

    // Build for loop Body Block:

    symir::BlockBuilder *loopBodyBd = funBd->OpenBlock(loopBodyLabel);

    // For Body
    symir::Coef *indVarCoef = funBd->SymCoef("__induction_loopVarCoef" + std::to_string(uid++) + indVar->GetName(), indVar->GetName());
    symir::BlockBuilder::StmtID arrSum = loopBodyBd->SymAssStmt(
      array,
      loopBodyBd->SymExpr(exprOp, {
        loopBodyBd->SymMulTerm(one, indVar)
      }),
      { indVarCoef }
    );

    // indVar = 1 + indVar
    symir::BlockBuilder::StmtID incAss = loopBodyBd->SymAssStmt(
      indVar,
      loopBodyBd->SymAddExpr({
        loopBodyBd->SymAddTerm(
          one,
          indVar
        )
      })
    );

    loopBodyBd->CommitStmt(arrSum);
    loopBodyBd->CommitStmt(incAss);
    loopBodyBd->SymGoto(loopCondLabel);

    utils::insertBlockBd(blockBds, { loopCondBd, loopBodyBd, secondBlockBd }, targetBlockIdx + 1);
  }

} // namespace
