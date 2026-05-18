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
#include "lib/Transformations/primitive.hpp"
#include "lib/Transformations/utils.hpp"
#include "lib/lang.hpp"
#include "lib/logger.hpp"
#include "lib/random.hpp"
#include <climits>
#include <cstdint>

using namespace patternmatch;
namespace transformations::primitive {
  
  bool AdditionFromConst::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(stmt, m_Expr(m_Any(m_CstTerm(m_Solved(), m_NoVar()))));
  }
  
  std::vector<symir::BlockBuilder::StmtID> AdditionFromConst::rewrite(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::Stmt *stmt
  ) {
    Log::Get().Out() << "Running AdditionFromConst" << std::endl;
  
    auto rep = utils::StmtReplacer<symir::Expr>(funBd, blockBd);
    std::function<symir::BlockBuilder::ExprID(symir::FunctBuilder * ,symir::BlockBuilder *, const symir::Expr &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Expr &e, void **data) {
          std::vector<symir::BlockBuilder::TermID> termIds;
          termIds.reserve(e.GetTerms().size() + 1);
          bool hasReplaced = false;
          for (auto term : e.GetTerms()) {
            if (!hasReplaced && term->GetOp() == symir::Term::OP_CST) {
              hasReplaced = true;
              *data = term->GetCoef();
              int target = term->GetCoef()->GetI32Value();
              // v1 must be choosen s.t. |v1| < |target| and sign(v1) == sign(target)
              // this ensures a UB free transformation since:
              // if Sk is the prefix sum up to the target Term then
              // |Sk op v1| < |Sk op target| and since Sk op target does not overflow neither does |Sk op v1|
              int v1, v2;
              if (target >= 0) {
                v1 = Random::Get().Uniform(0, target)();
              } else {
                v1 = Random::Get().Uniform(target, -1)();
              }
              v2 = target - v1;
              if (e.GetOp() == symir::Expr::OP_SUB) v2 = -v2;
  
              termIds.push_back(thisBlockBd->SymTerm(
                symir::Term::OP_CST,
                thisFunBd->SymI32Const(v1),
                nullptr, {})
              );
              termIds.push_back(thisBlockBd->SymTerm(
                symir::Term::OP_CST,
                thisFunBd->SymI32Const(v2),
                nullptr, {})
              );
            } else {
              termIds.push_back(symir::StmtCopier(thisFunBd, thisBlockBd).CopyTerm(term));
            }
          }
  
        return thisBlockBd->SymExpr(e.GetOp(), termIds);
      };
    symir::BlockBuilder::StmtID newStmt = rep.CopyStmtWithReplacement(
      stmt,
      make_matcher(const symir::Expr *, m_Expr(m_Any(m_CstTerm(m_Solved(), m_NoVar())))),
      varInsertFun,
      0.25
    );
    symir::Coef *replacedCoef = static_cast<symir::Coef *>(rep.getExtractedDataRef());
    Assert(replacedCoef != nullptr, "replacedCoef should never be nullptr");
    Assert(replacedCoef->IsSolved(), "replacedCoef should never be unsolved");
  
    Log::Get().Out() << "Replacing Const " << replacedCoef->GetI32Value() << " with addition" << std::endl;
  
    return {newStmt};
  }
  
  bool ForSumFromConst::match(const symir::Stmt *stmt) const {
    return patternmatch::match(
        stmt,
        m_AssStmt(
          m_WildCard<const symir::VarUse *>(),
          m_Expr(m_One(m_CstTerm(m_Solved(), m_NoVar())))
        )
      );
  }
  
  std::vector<symir::BlockBuilder::StmtID> ForSumFromConst::rewrite(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::Stmt *stmt
  ) {
    Log::Get().Out() << "Running ConstToForSum" << std::endl;
  
    const symir::AssStmt *assStmt = static_cast<const symir::AssStmt *>(stmt);
    const auto use = assStmt->GetVar();
    const auto def = use->GetDef();
    const auto expr = assStmt->GetExpr();
  
    Assert(
      expr->GetTerms().size() == 1 && expr->GetTerm(0)->GetOp() == symir::Term::OP_CST,
      "match function for ConstToForSum failed to uphold its promisses"
    );
  
    int32_t val = expr->GetTerm(0)->GetCoef()->GetI32Value();
    int32_t loopCount, rest, randVal;
    if (val > 0) {
      randVal = Random::Get().Uniform(1, std::max(val / 2, 1))();
      rest = val % randVal;
      loopCount = val / randVal;
    } else if (val == 0) {
      randVal = 0;
      rest = 0;
      loopCount = 0;
    } else {
      randVal = Random::Get().Uniform(std::min(val / 2, -1), -1)();
      rest = val % randVal;
      loopCount = val / randVal;
    }
    Assert(loopCount >= 0, "loopCount must be larger then 0");
    Log::Get().Out() << "Target: " << val << ", LoopCount: " << loopCount 
                     << ", Additive Value: " << randVal << ", Remainder: " << rest << std::endl;
  
    auto access = utils::copyAccess(funBd, use);
  
    symir::BlockBuilder::StmtID initAss = blockBd->SymAssStmt(
      def,
      blockBd->SymAddExpr({
        blockBd->SymCstTerm(
          funBd->SymI32Const(rest),
          nullptr, {} 
        )
      }),
      access
    );
  
    symir::BlockBuilder::StmtID addAss = blockBd->SymAssStmt(
      def,
      blockBd->SymAddExpr({
        blockBd->SymAddTerm(
          funBd->SymI32Const(randVal),
          def, access
        )
      }),
      access
    );
  
    auto loopVar = this->getNewScaLocal(funBd, blockBd->GetLabel());
    symir::BlockBuilder::StmtID forSum = blockBd->SymForStmt(
      // loop variable i
      loopVar,
      // loop condition (i - loopCount < 0)
      blockBd->SymCond(symir::Cond::OP_LTZ, 
        blockBd->SymAddExpr({
          blockBd->SymMulTerm(
            funBd->SymI32Const(1),
            loopVar, {}
          ),
          blockBd->SymCstTerm(
            funBd->SymI32Const(-loopCount),
            nullptr, {}
          )
        })
      ),
      // int i = 0
      blockBd->SymAddExpr({
        blockBd->SymCstTerm(
          funBd->SymI32Const(0),
          nullptr
        )
      }),
      // i += 1
      blockBd->SymAddExpr({
        blockBd->SymCstTerm(
          funBd->SymI32Const(1),
          nullptr
        )
      }),
      {addAss}
    );
  
    return { initAss, forSum };
  }
  
  bool DeadCodeFromAssign::match(const symir::Stmt *stmt) const {
    return patternmatch::match(stmt, m_AssStmt(m_WildCard<const symir::VarUse *>(), m_WildCard<const symir::Expr *>()));
  }
  
  std::vector<symir::BlockBuilder::StmtID> DeadCodeFromAssign::rewrite(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::Stmt *stmt
  ) {
    Log::Get().Out() << "Running AssToDeadCode" << std::endl;
  
    const symir::AssStmt *assStmt = static_cast<const symir::AssStmt *>(stmt);
    const auto use = assStmt->GetVar();
    const auto def = use->GetDef();
    const auto expr = assStmt->GetExpr();
    auto access = utils::copyAccess(funBd, use);
  
    symir::BlockBuilder::ExprID exprId = symir::StmtCopier(funBd, blockBd).CopyExpr(expr);
  
    int nrBranches = Random::Get().Uniform(this->minBranches, this->maxBranches)();
    int trueBranch = Random::Get().Uniform(0, nrBranches - 1)();
  
    Log::Get().Out() << "Building " << nrBranches << " branches, True branch: " << trueBranch << std::endl;
  
    std::vector<symir::BlockBuilder::CondID> conds;
    std::vector<std::vector<symir::BlockBuilder::StmtID>> sids;
    conds.reserve(nrBranches - 1);
    sids.reserve(nrBranches);
    for (int i = 0; i < nrBranches; i++) {
      if (i < nrBranches - 1) {
        // not the else branch hence we create a condition
        if (i == trueBranch) {
          conds.push_back(utils::triviallyTrueCond(funBd, blockBd));
        } else {
          conds.push_back(utils::triviallyFalseCond(funBd, blockBd));
        }
      }
      if (i == trueBranch) {
        sids.push_back({ blockBd->SymAssStmt(def, exprId, access) });
      } else {
        // TODO: genrate more complex expression and take the allowUB member into account
        sids.push_back({
          blockBd->SymAssStmt(
            def,
            blockBd->SymAddExpr({
              blockBd->SymCstTerm(
                funBd->SymI32Const(Random::Get().Uniform(INT_MIN, INT_MAX)()),
                nullptr
              )
            }),
            access
          ) 
        });
      }
      
    }
  
    return { blockBd->SymIfStmt(conds, sids) };
  }

  bool ConstProbpagationViaAdd::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(stmt, m_Expr(m_Any(m_CstTerm(m_Solved(), m_NoVar()))));
  }
  
  std::vector<symir::BlockBuilder::StmtID> ConstProbpagationViaAdd::rewrite(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::Stmt *stmt
  ) {
    Log::Get().Out() << "Running ConstProba" << std::endl;
  
    auto rep = utils::StmtReplacer<symir::Term>(funBd, blockBd);
    const symir::VarDef *var = this->getNewScaLocal(funBd, blockBd->GetLabel());

    std::cout << "fun: " << funBd->GetName() << " block: " << blockBd->GetLabel() << " ConstProbaAddVarName: " << var->GetName() << std::endl;
  
    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder * ,symir::BlockBuilder *, const symir::Term &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Term &t, void **data) {
          std::vector<symir::BlockBuilder::TermID> termIds;
          int target = t.GetCoef()->GetI32Value();
          int v1, v2;
          if (target > 0) {
            v1 = Random::Get().Uniform(0, target)();
          } else if (target < 0) {
            v1 = Random::Get().Uniform(target, -1)();
          } else {
            v1 = Random::Get().Uniform(0, INT_MAX)();
          }
          v2 = target - v1;
          Assert(v1 + v2 == target, "Faulty transformation");
  
          *data = thisFunBd->SymI32Const(v2);
          return thisBlockBd->SymAddTerm(thisFunBd->SymI32Const(v1), var);
        };
  
    symir::BlockBuilder::StmtID newStmt = rep.CopyStmtWithReplacement(
      stmt,
      make_matcher(const symir::Term *, m_CstTerm(m_Solved(), m_NoVar())),
      varInsertFun,
      0.25
    );
    symir::Coef *replacedCoef = static_cast<symir::Coef *>(rep.getExtractedDataRef());
    Assert(replacedCoef != nullptr, "replacedCoef should never be nullptr");
    Assert(replacedCoef->IsSolved(), "replacedCoef should never be unsolved");
  
    Log::Get().Out() << "Replacing Const " << replacedCoef->GetI32Value() << " with " << var->GetName() << std::endl;
  
    symir::BlockBuilder::StmtID assignStmts = blockBd->SymAssStmt(
      var,
      blockBd->SymExpr(
        symir::Expr::OP_ADD,
        { blockBd->SymCstTerm(replacedCoef, nullptr) }
      )
    );
  
    return {assignStmts, newStmt};
  }
  
  bool ConstProbpagationViaSub::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(stmt, m_Expr(m_Any(m_CstTerm(m_Solved(), m_NoVar()))));
  }
  
  std::vector<symir::BlockBuilder::StmtID> ConstProbpagationViaSub::rewrite(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::Stmt *stmt
  ) {
    Log::Get().Out() << "Running ConstProba" << std::endl;
  
    auto rep = utils::StmtReplacer<symir::Term>(funBd, blockBd);
    const symir::VarDef *var = this->getNewScaLocal(funBd, blockBd->GetLabel());
  
  
    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder * ,symir::BlockBuilder *, const symir::Term &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Term &t, void **data) {
          std::vector<symir::BlockBuilder::TermID> termIds;
          int target = t.GetCoef()->GetI32Value();
          int v1, v2;
          if (target > 0) {
            v1 = Random::Get().Uniform(0, target)();
          } else if (target < 0) {
            v1 = Random::Get().Uniform(target, -1)();
          } else {
            v1 = Random::Get().Uniform(0, INT_MAX)();
          }
          v2 = v1 - target;
          Assert(v1 - v2 == target, "Faulty transformation");
  
          *data = thisFunBd->SymI32Const(v2);
          return thisBlockBd->SymSubTerm(thisFunBd->SymI32Const(v1), var);
        };
  
    symir::BlockBuilder::StmtID newStmt = rep.CopyStmtWithReplacement(
      stmt,
      make_matcher(const symir::Term *, m_CstTerm(m_Solved(), m_NoVar())),
      varInsertFun,
      0.25
    );
    symir::Coef *replacedCoef = static_cast<symir::Coef *>(rep.getExtractedDataRef());
    Assert(replacedCoef != nullptr, "replacedCoef should never be nullptr");
    Assert(replacedCoef->IsSolved(), "replacedCoef should never be unsolved");
  
    Log::Get().Out() << "Replacing Const " << replacedCoef->GetI32Value() << " with " << var->GetName() << std::endl;
  
    symir::BlockBuilder::StmtID assignStmts = blockBd->SymAssStmt(
      var,
      blockBd->SymExpr(
        symir::Expr::OP_ADD,
        { blockBd->SymCstTerm(replacedCoef, nullptr) }
      )
    );
  
    return {assignStmts, newStmt};
  }
  
  bool ConstProbpagationViaMul::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(stmt, m_Expr(m_Any(m_CstTerm(m_Not(m_Eq<const symir::Coef *, int32_t>(INT_MIN)), m_NoVar()))));
  }
  
  std::vector<symir::BlockBuilder::StmtID> ConstProbpagationViaMul::rewrite(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::Stmt *stmt
  ) {
    Log::Get().Out() << "Running ConstProba" << std::endl;
  
    auto rep = utils::StmtReplacer<symir::Expr>(funBd, blockBd);
    const symir::VarDef *var = this->getNewScaLocal(funBd, blockBd->GetLabel());
  
  
    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder * ,symir::BlockBuilder *, const symir::Expr &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Expr &e, void **data) {
          std::vector<symir::BlockBuilder::TermID> termIds;
          termIds.reserve(e.GetTerms().size() + 1);
          bool hasReplaced = false;
          for (auto term : e.GetTerms()) {
            if (hasReplaced || term->GetOp() != symir::Term::OP_CST || term->GetCoef()->GetI32Value() == INT_MIN) {
              termIds.push_back(symir::StmtCopier(thisFunBd, thisBlockBd).CopyTerm(term));
              continue;
            }

            hasReplaced = true;
            int target = term->GetCoef()->GetI32Value();
            int32_t mulVal, randVal, rest;
            if (target > 0) {
              randVal = Random::Get().Uniform(1, std::max(target / 2, 1))();
              rest = target % randVal;
              mulVal = target / randVal;
            } else if (target == 0) {
              randVal = 0;
              rest = 0;
              mulVal = 0;
            } else {
              randVal = Random::Get().Uniform(std::min(target / 2, -1), -1)();
              rest = target % randVal;
              mulVal = target / randVal;
            }
            Assert(target == 0 || randVal == 0 || mulVal == 0
              || (abs(randVal * mulVal) <= abs(target) && abs(target) / target == abs(randVal * mulVal) / (randVal * mulVal)),
              "Faulty transformation (target: %d, randVal: %d, mulVal: %d)", target, randVal, mulVal
            );
            Assert(randVal * mulVal + rest == target, "Faulty transformation");
            if (e.GetOp() == symir::Expr::OP_SUB) rest = -rest;
            *data = thisFunBd->SymI32Const(mulVal);
            termIds.push_back(thisBlockBd->SymMulTerm(thisFunBd->SymI32Const(randVal), var));
            termIds.push_back(thisBlockBd->SymCstTerm(thisFunBd->SymI32Const(rest), nullptr));
          }
          return thisBlockBd->SymExpr(e.GetOp(), termIds);
        };
  
    symir::BlockBuilder::StmtID newStmt = rep.CopyStmtWithReplacement(
      stmt,
      make_matcher(const symir::Expr *, m_Expr(m_Any(m_CstTerm(m_Not(m_Eq<const symir::Coef *, int32_t>(INT_MIN)), m_NoVar())))),
      varInsertFun,
      0.25
    );
    symir::Coef *replacedCoef = static_cast<symir::Coef *>(rep.getExtractedDataRef());
    Assert(replacedCoef != nullptr, "replacedCoef should never be nullptr");
    Assert(replacedCoef->IsSolved(), "replacedCoef should never be unsolved");
  
    Log::Get().Out() << "Replacing Const " << replacedCoef->GetI32Value() << " with " << var->GetName() << std::endl;
  
    symir::BlockBuilder::StmtID assignStmts = blockBd->SymAssStmt(
      var,
      blockBd->SymExpr(
        symir::Expr::OP_ADD,
        { blockBd->SymCstTerm(replacedCoef, nullptr) }
      )
    );
  
    return {assignStmts, newStmt};
  }
  
  bool ConstProbpagationViaDiv::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(stmt, m_Expr(m_Any(m_CstTerm(m_Not(m_Eq<const symir::Coef *, int32_t>(INT_MIN)), m_NoVar()))));
  }
  
  std::vector<symir::BlockBuilder::StmtID> ConstProbpagationViaDiv::rewrite(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::Stmt *stmt
  ) {
    Log::Get().Out() << "Running ConstProbpagationViaDiv" << std::endl;
  
    auto rep = utils::StmtReplacer<symir::Term>(funBd, blockBd);
    const symir::VarDef *var = this->getNewScaLocal(funBd, blockBd->GetLabel());
  
  
    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder *, symir::BlockBuilder *, const symir::Term &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Term &t, void **data) {
          int target = t.GetCoef()->GetI32Value();
          int v1, v2; 
          if (target > 0) {
            v2 = Random::Get().Uniform(1, INT_MAX / target)();
          } else if (target < 0) {
            // We do not match INT_MIN as target so we can avoid that special case
            // We also want to avoid target * v2 = -INT_MIN
            v2 = Random::Get().Uniform(static_cast<int32_t>((INT_MIN + 1) / -target), -1)();
          } else {
            v2 = Random::Get().Uniform(1, INT_MAX)();
          }
          Assert(v2 != 0, "Attempted to generate a Division be Zero");
          v1 = target * v2;
          Assert(v1 / v2 == target, "Faulty transformation");
  
          *data = thisFunBd->SymI32Const(v2);
          return thisBlockBd->SymDivTerm(thisFunBd->SymI32Const(v1), var);
        };
  
    symir::BlockBuilder::StmtID newStmt = rep.CopyStmtWithReplacement(
      stmt,
      make_matcher(const symir::Term *, m_CstTerm(m_Not(m_Eq<const symir::Coef *, int32_t>(INT_MIN)), m_NoVar())),
      varInsertFun,
      0.25
    );
    symir::Coef *replacedCoef = static_cast<symir::Coef *>(rep.getExtractedDataRef());
    Assert(replacedCoef != nullptr, "replacedCoef should never be nullptr");
    Assert(replacedCoef->IsSolved(), "replacedCoef should never be unsolved");
  
    Log::Get().Out() << "Replacing Const " << replacedCoef->GetI32Value() << " with " << var->GetName() << std::endl;
  
    symir::BlockBuilder::StmtID assignStmts = blockBd->SymAssStmt(
      var,
      blockBd->SymExpr(
        symir::Expr::OP_ADD,
        { blockBd->SymCstTerm(replacedCoef, nullptr) }
      )
    );
  
    return {assignStmts, newStmt};
  }

  bool Reg2Mem::match(const symir::Stmt *stmt) const {
    return patternmatch::match(
      stmt,
      m_AssStmt(m_And(m_WithType(symir::SymIR::Type::I32), m_ScalarVar()), m_WildCard<const symir::Expr *>())
    );
  }

  std::vector<symir::BlockBuilder::StmtID> Reg2Mem::rewrite(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::Stmt *stmt
  ) {
    const symir::AssStmt *assStmt = static_cast<const symir::AssStmt *>(stmt);
    const symir::VarDef *memVar = this->getNewVecLocal(funBd, blockBd->GetLabel(), {1});
    symir::StmtCopier c = symir::StmtCopier(funBd, blockBd);
    std::vector<symir::Coef *> memAccess = {funBd->SymI32Const(0)};
    const symir::BlockBuilder::StmtID memAssign = blockBd->SymAssStmt(memVar, c.CopyExpr(assStmt->GetExpr()), memAccess);

    const symir::VarUse *v = assStmt->GetVar();
    auto access = utils::copyAccess(funBd, v);
    const symir::BlockBuilder::StmtID assignBack = blockBd->SymAssStmt(
      v->GetDef(),
      blockBd->SymAddExpr({ blockBd->SymMulTerm(funBd->SymI32Const(1), memVar, memAccess) }),
      access
    );
    
    return {memAssign, assignBack};
  }

} // namespace 

