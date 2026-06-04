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
#include <string>

using namespace patternmatch;
namespace transformations::primitive {
  
  bool Guard::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(stmt, m_Expr(m_Any(m_CstTerm(m_Solved(), m_NoVar()))));
  }

  void Guard::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    if (varState.nrVariables == 0) return;
    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);

    const symir::VarDef *var = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix);
    auto rep = utils::StmtReplacer<symir::Term>(funBd, blockBd);
    std::pair<int32_t, int32_t> targetPair;
    rep.data = &targetPair;

    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder * ,symir::BlockBuilder *, const symir::Term &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Term &t, void **data) {
        
        auto randTarget = Random::Get().Uniform(0, prime - 1);
        utils::PrimeInterpolation interpolGen = utils::PrimeInterpolation(prime); 

        int32_t target = t.GetCoef()->GetI32Value();

        int32_t interpolTarget;
        if (target == INT32_MIN) {
          // avoid the div by 0 case of the else stmt
          interpolTarget = 0;
        } else {
          interpolTarget = randTarget() % static_cast<int32_t>((-static_cast<int64_t>(INT32_MIN)) + target);
        }
        std::pair<int32_t, int32_t> *targetPair = static_cast<std::pair<int32_t, int32_t> *>(*data);
        targetPair->first = target;
        targetPair->second = interpolTarget;

        return thisBlockBd->SymAddTerm(
          funBd->SymI32Const(target - interpolTarget),
          var
        );
      };
    rep.ReplaceStmt(
      stmt,
      make_matcher(const symir::Term *, m_CstTerm(m_Solved(), m_NoVar())),
      varInsertFun,
      0.25
    );

    int32_t interpolTarget = targetPair.second;

    // Filter Varstate
    utils::VarFilter filter = utils::VarFilter(funBd, varState);
    filter.randomlyFilter();

    utils::PrimeInterpolation interpolGen = utils::PrimeInterpolation(this->prime); 

    size_t nrVars = filter.filteredVars.size();
    size_t nrIters = filter.filteredVarState.size() / filter.filteredVars.size();
    interpolGen.interpolate(
      nrVars,
      nrIters,
      filter.filteredVarState,
      interpolTarget
    );
    std::vector<int32_t> polynomial = interpolGen.getPolynomial();
    std::vector<int32_t> coeffsVals = interpolGen.getCoeffs();
    interpolGen.assertCorrectness(
      nrVars,
      nrIters,
      filter.filteredVarState,
      interpolTarget
    );

    // we need coeff object to hand to the builder
    std::vector<symir::Coef *> coeffs{};
    coeffs.reserve(coeffsVals.size());
    for (const int32_t c : coeffsVals) {
      coeffs.push_back(funBd->SymI32Const(c));
    }
    blockBd->CommitStmtAt(
      blockBd->SymModAssStmt(
        var,
        blockBd->SymModExpr(
          coeffs,
          filter.filteredVars,
          filter.filteredAccesses,
          polynomial,
          prime
        ),
        {}
      ), 
      targetStmtIdx
    );
  }

  bool AdditionFromConst::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(stmt, m_Expr(m_Any(m_CstTerm(m_Solved(), m_NoVar()))));
  }
  
  void AdditionFromConst::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running AdditionFromConst" << std::endl;
  
    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);

    auto rep = utils::StmtReplacer<symir::Expr>(funBd, blockBd);
    std::function<symir::BlockBuilder::ExprID(symir::FunctBuilder * ,symir::BlockBuilder *, const symir::Expr &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Expr &e, void **data) {
          std::vector<symir::BlockBuilder::TermID> termIds;
          termIds.reserve(e.GetTerms().size() + 1);
          bool hasReplaced = false;
          auto terms = e.GetTerms();
          for (size_t i = 0; i < terms.size(); i++) {
            auto term = terms[i];
            if (!hasReplaced && term->GetOp() == symir::Term::OP_CST) {
              hasReplaced = true;
              *data = term->GetCoef();
              int target = term->GetCoef()->GetI32Value();
              // v1 must be choosen s.t. |v1| < |target| and sign(v1) == sign(target)
              // this ensures a UB free transformation since:
              // if Sk is the prefix sum up to the target Term then
              // |Sk op v1| < |Sk op target| and since Sk op target does not overflow neither does |Sk op v1|
              int v1, v2;
              if (target > 0) v1 = Random::Get().Uniform(e.GetOp() == symir::Expr::OP_SUB ? 1 : 0, target)(); 
              else if (target == 0) v1 = 0;
              else v1 = Random::Get().Uniform(target, -1)();

              switch (e.GetOp()) {
              case symir::Expr::OP_ADD: {
                v2 = target - v1;
                Assert(v1 + v2 == target, "Faulty Transformation");
              }; break;
              case symir::Expr::OP_SUB: {
                v2 = v1 - target;
                if (i != 0) { 
                  v2 = -v2;
                  Assert(-v1 - v2 == -target, "Faulty Transformation");
                } else {
                  Assert(v1 - v2 == target, "Faulty Transformation");

                }
              }; break;
              default: { Panic("should not reach here"); }
              }
  
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
    rep.ReplaceStmt(
      stmt,
      make_matcher(const symir::Expr *, m_Expr(m_Any(m_CstTerm(m_Solved(), m_NoVar())))),
      varInsertFun,
      0.25
    );

    symir::Coef *replacedCoef = static_cast<symir::Coef *>(rep.data);

    Assert(replacedCoef != nullptr, "replacedCoef should never be nullptr");
    Assert(replacedCoef->IsSolved(), "replacedCoef should never be unsolved");
    
  }

  bool AggressiveAdditionFromConst::match(const symir::Stmt *stmt) const {
    return patternmatch::match(
      stmt,
      m_AssStmt(
        m_WildCard<const symir::VarUse *>(),
        m_Expr(m_FirstN(1, m_CstTerm(m_Solved(), m_NoVar())))
      )
    );
  }

  void AggressiveAdditionFromConst::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running AdditionFromConst" << std::endl;
  
    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);

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
              int v1, v2;
              if (target >= 0) {
                v1 = Random::Get().Uniform(INT_MIN + target + 1, target)();
              } else {
                v1 = Random::Get().Uniform(target + 1, INT_MAX + target)();
              }
              v2 = target - v1;
              Assert(v2 != INT_MIN || e.GetOp() == symir::Expr::OP_SUB, "This overflows");
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
    rep.ReplaceStmt(
      stmt,
      make_matcher(const symir::Expr *, m_Expr(m_FirstN(1, m_CstTerm(m_Solved(), m_NoVar())))),
      varInsertFun,
      1
    );
  }

  bool InsertConstZeroAdditions::match(const symir::Stmt *stmt) const {
    return patternmatch::match(
      stmt,
      m_AssStmt(
        m_WildCard<const symir::VarUse *>(),
        m_AddExpr(m_WildCard<std::vector<const symir::Term *>>())
      )
    );
  }

  void InsertConstZeroAdditions::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running AdditionFromConst" << std::endl;
  
    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);

    auto rep = utils::StmtReplacer<symir::Expr>(funBd, blockBd);
    std::function<symir::BlockBuilder::ExprID(symir::FunctBuilder * ,symir::BlockBuilder *, const symir::Expr &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Expr &e, void **data) {
          std::vector<symir::BlockBuilder::TermID> termIds;
          termIds.reserve(e.GetTerms().size() + 1);
          int val = Random::Get().Uniform(INT_MIN + 1, INT_MAX)();
          termIds.push_back(thisBlockBd->SymTerm(
            symir::Term::OP_CST,
            thisFunBd->SymI32Const(val),
            nullptr, {}
          ));
          termIds.push_back(thisBlockBd->SymTerm(
            symir::Term::OP_CST,
            thisFunBd->SymI32Const(-val),
            nullptr, {}
          ));
          for (auto term : e.GetTerms()) {
            termIds.push_back(symir::StmtCopier(thisFunBd, thisBlockBd).CopyTerm(term));
          }
          return thisBlockBd->SymExpr(e.GetOp(), termIds);
        };
    rep.ReplaceStmt(
      stmt,
      make_matcher(const symir::Expr *, m_AddExpr(m_WildCard<std::vector<const symir::Term *>>())),
      varInsertFun,
      1
    );
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
  
  void ForSumFromConst::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running ForSumFromConst" << std::endl;
  
    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmt(targetStmtIdx);

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
  
    std::string loopCondLabel = utils::nameLabel(funBd->GetName(), "for_cond");
    std::string loopBodyLabel = utils::nameLabel(funBd->GetName(), "for_body");
    std::string finalLabel = utils::nameLabel(funBd->GetName(), "for_exit");

    // Split the current block into two at targetStmtIdx while appending to the upper block

    symir::BlockBuilder *secondBlockBd = utils::splitBlockAt(funBd, blockBd, finalLabel, targetStmtIdx);

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
  

    auto indVar = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix);
    // indVar = 0
    symir::BlockBuilder::StmtID initIndVar = blockBd->SymAssStmt(
      indVar,
      blockBd->SymAddExpr({
        blockBd->SymCstTerm(
          funBd->SymI32Const(0),
          nullptr
        )
      })
    );

    blockBd->ReplaceCommitStmt({ initAss, initIndVar}, targetStmtIdx);
    blockBd->SymGoto(loopCondLabel);


    // Build for loop Cond block:

    symir::BlockBuilder *loopCondBd = funBd->OpenBlock(loopCondLabel);

    // indVar - loopCount < 0
    symir::BlockBuilder::CondID loopCond = loopCondBd->SymCond(symir::Cond::OP_LTZ, 
      loopCondBd->SymAddExpr({
        loopCondBd->SymMulTerm(
          funBd->SymI32Const(1),
          indVar
        ),
        loopCondBd->SymCstTerm(
          funBd->SymI32Const(-loopCount),
          nullptr
        )
      })
    );

    loopCondBd->SymBranch(loopBodyLabel, finalLabel, loopCond);

    // Build for loop Body Block:

    symir::BlockBuilder *loopBodyBd = funBd->OpenBlock(loopBodyLabel);

    // For Body
    symir::BlockBuilder::StmtID addAss = loopBodyBd->SymAssStmt(
      def,
      loopBodyBd->SymAddExpr({
        loopBodyBd->SymAddTerm(
          funBd->SymI32Const(randVal),
          def, access
        )
      }),
      access
    );

    // indVar = 1 + indVar
    symir::BlockBuilder::StmtID incAss = loopBodyBd->SymAssStmt(
      indVar,
      loopBodyBd->SymAddExpr({
        loopBodyBd->SymAddTerm(
          funBd->SymI32Const(1),
          indVar
        )
      })
    );

    loopBodyBd->CommitStmt(addAss);
    loopBodyBd->CommitStmt(incAss);
    loopBodyBd->SymGoto(loopCondLabel);

    // insert the new blocks after the current block
    utils::insertBlockBd(blockBds, { loopCondBd, loopBodyBd, secondBlockBd }, targetBlockIdx + 1);
  }
  
  bool DeadCodeFromAssign::match(const symir::Stmt *stmt) const {
    return patternmatch::match(stmt, m_AssStmt(m_WildCard<const symir::VarUse *>(), m_WildCard<const symir::Expr *>()));
  }
  
  void DeadCodeFromAssign::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running AssToDeadCode" << std::endl;

    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmt(targetStmtIdx);
  
    const symir::AssStmt *assStmt = static_cast<const symir::AssStmt *>(stmt);
    const auto use = assStmt->GetVar();
    const auto def = use->GetDef();
    const auto expr = assStmt->GetExpr();
    auto access = utils::copyAccess(funBd, use);
  
    int nrBranches = Random::Get().Uniform(this->minBranches, this->maxBranches)();
    int trueBranch = Random::Get().Uniform(0, nrBranches - 1)();
  
    Log::Get().Out() << "Building " << nrBranches << " branches, True branch: " << trueBranch << std::endl;

    std::string finalLabel = utils::nameLabel(funBd->GetName(), "if_exit");
    std::vector<std::string> condLabels;
    condLabels.resize(nrBranches - 1); // else stmt has not cond and first cond is appended to blockBd, but for convinience we index from the index 1
    std::vector<std::string> bodyLabels;
    bodyLabels.resize(nrBranches);

    for (int i = 0; i < nrBranches; i++) {
      bodyLabels[i] = utils::nameLabel(funBd->GetName(), "if_body_" + std::to_string(i));
      if (i == 0 || i == nrBranches - 1) continue;
      condLabels[i] = utils::nameLabel(funBd->GetName(), "if_cond_" + std::to_string(i));
    }

    symir::BlockBuilder *secondBlockBd = utils::splitBlockAt(funBd, blockBd, finalLabel, targetStmtIdx);

    std::vector<symir::BlockBuilder *> condBlockBds;
    condBlockBds.resize(nrBranches - 1); // else stmt has not cond and first cond is appended to blockBd, but for convinience we index from the index 1
    std::vector<symir::BlockBuilder *> bodyBlockBds;
    bodyBlockBds.resize(nrBranches);

    // Build First Block
    bodyBlockBds[0] = funBd->OpenBlock(bodyLabels[0]);
    if (0 == trueBranch) {
      bodyBlockBds[0]->CommitStmt(
        bodyBlockBds[0]->SymAssStmt(
          def,
          symir::StmtCopier(funBd, bodyBlockBds[0]).CopyExpr(expr),
          access
        )
      );
    } else {
      bodyBlockBds[0]->CommitStmt(utils::trivialAssignment(funBd, bodyBlockBds[0], def, access));
    }
    bodyBlockBds[0]->SymGoto(finalLabel);

    // Then iterativly create all conditions and bodies excluding the last
    for (int i = 1; i < nrBranches; i++) {
      if (i != nrBranches - 1) {
        condBlockBds[i] = funBd->OpenBlock(condLabels[i]);
        condBlockBds[i]->SymBranch(
          bodyLabels[i],
          // last condition if false should point to the else body
          i != nrBranches - 2 ? condLabels[i + 1] : bodyLabels[nrBranches - 1], 
          utils::triviallyCondFor(i == trueBranch , funBd, condBlockBds[i])
        );
      }
      bodyBlockBds[i] = funBd->OpenBlock(bodyLabels[i]);
      if (i == trueBranch) {
        bodyBlockBds[i]->CommitStmt(
          bodyBlockBds[i]->SymAssStmt(
            def,
            symir::StmtCopier(funBd, bodyBlockBds[i]).CopyExpr(expr),
            access
          )
        );
      } else {
        bodyBlockBds[i]->CommitStmt(utils::trivialAssignment(funBd, bodyBlockBds[i], def, access));
      }
      bodyBlockBds[i]->SymGoto(finalLabel);
    }

    blockBd->RemoveCommittedStmts(targetStmtIdx, targetStmtIdx);

    // append first condition to blockBd
    // Fails for nrBranches = 2
    blockBd->SymBranch(
      bodyLabels[0],
      // condition if false should point to the else body if there is no else ifs
      nrBranches > 2 ? condLabels[1] : bodyLabels[1], 
      utils::triviallyCondFor(0 == trueBranch , funBd, blockBd)
    ); 

    // interleave the blocks;
    std::vector<symir::BlockBuilder *> newBlocks;
    newBlocks.reserve(1 + condBlockBds.size() + bodyBlockBds.size());
    newBlocks.push_back(bodyBlockBds[0]);
    for (int i = 1; i < nrBranches; i++) {
      if (i != nrBranches - 1) newBlocks.push_back(condBlockBds[i]);
      newBlocks.push_back(bodyBlockBds[i]);
    }
    newBlocks.push_back(secondBlockBd);
  
    utils::insertBlockBd(blockBds, newBlocks, targetBlockIdx + 1);
  }

  bool ConstPropagationViaAdd::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(stmt, m_Expr(m_Any(m_CstTerm(m_Solved(), m_NoVar()))));
  }
  
  void ConstPropagationViaAdd::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running ConstProbaagationViaAdd" << std::endl;
  
    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);

    auto rep = utils::StmtReplacer<symir::Term>(funBd, blockBd);
    const symir::VarDef *var = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix);

    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder * ,symir::BlockBuilder *, const symir::Term &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Term &t, void **data) {
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
          Log::Get().Out() 
            << "Replacing Const " << target
            << " with " << v1 << " + " << v2 << std::endl;
  
          *data = thisFunBd->SymI32Const(v2);
          return thisBlockBd->SymAddTerm(thisFunBd->SymI32Const(v1), var);
        };
  
    rep.ReplaceStmt(
      stmt,
      make_matcher(const symir::Term *, m_CstTerm(m_Solved(), m_NoVar())),
      varInsertFun,
      0.25
    );
    symir::Coef *replacedCoef = static_cast<symir::Coef *>(rep.data);
    Assert(replacedCoef != nullptr, "replacedCoef should never be nullptr");
    Assert(replacedCoef->IsSolved(), "replacedCoef should never be unsolved");
  
    symir::BlockBuilder::StmtID assignStmts = blockBd->SymAssStmt(
      var,
      blockBd->SymExpr(
        symir::Expr::OP_ADD,
        { blockBd->SymCstTerm(replacedCoef, nullptr) }
      )
    );
  
    blockBd->CommitStmtAt(assignStmts, targetStmtIdx);
  }
  
  bool ConstPropagationViaSub::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(stmt, m_Expr(m_Any(m_CstTerm(m_Solved(), m_NoVar()))));
  }
  
  void ConstPropagationViaSub::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running ConstProba" << std::endl;
  
    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);

    auto rep = utils::StmtReplacer<symir::Term>(funBd, blockBd);
    const symir::VarDef *var = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix);
  
  
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
          Log::Get().Out() 
            << "Replacing Const " << target
            << " with " << v1 << " - " << v2 << std::endl;
  
          *data = thisFunBd->SymI32Const(v2);
          return thisBlockBd->SymSubTerm(thisFunBd->SymI32Const(v1), var);
        };
  
    rep.ReplaceStmt(
      stmt,
      make_matcher(const symir::Term *, m_CstTerm(m_Solved(), m_NoVar())),
      varInsertFun,
      0.25
    );
    symir::Coef *replacedCoef = static_cast<symir::Coef *>(rep.data);
    Assert(replacedCoef != nullptr, "replacedCoef should never be nullptr");
    Assert(replacedCoef->IsSolved(), "replacedCoef should never be unsolved");
  
    symir::BlockBuilder::StmtID assignStmts = blockBd->SymAssStmt(
      var,
      blockBd->SymExpr(
        symir::Expr::OP_ADD,
        { blockBd->SymCstTerm(replacedCoef, nullptr) }
      )
    );
  
    blockBd->CommitStmtAt(assignStmts, targetStmtIdx);
  }
  
  bool ConstPropagationViaMul::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(
      stmt,
      m_Expr(m_Any(m_CstTerm(m_Not(m_Eq<const symir::Coef *, int32_t>(INT_MIN)), m_NoVar())))
    );
  }
  
  void ConstPropagationViaMul::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running ConstPropagationViaMul" << std::endl;
  
    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);

    auto rep = utils::StmtReplacer<symir::Expr>(funBd, blockBd);
    const symir::VarDef *var = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix);
  
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
            Log::Get().Out() 
              << "Replacing Const " << target
              << " with " << randVal << " * " << mulVal
              << " + " << rest << std::endl;

            *data = thisFunBd->SymI32Const(mulVal);
            termIds.push_back(thisBlockBd->SymMulTerm(thisFunBd->SymI32Const(randVal), var));
            termIds.push_back(thisBlockBd->SymCstTerm(thisFunBd->SymI32Const(rest), nullptr));
          }
          return thisBlockBd->SymExpr(e.GetOp(), termIds);
        };
  
    rep.ReplaceStmt(
      stmt,
      make_matcher(const symir::Expr *, m_Expr(m_Any(m_CstTerm(m_Not(m_Eq<const symir::Coef *, int32_t>(INT_MIN)), m_NoVar())))),
      varInsertFun,
      0.25
    );
    symir::Coef *replacedCoef = static_cast<symir::Coef *>(rep.data);
    Assert(replacedCoef != nullptr, "replacedCoef should never be nullptr");
    Assert(replacedCoef->IsSolved(), "replacedCoef should never be unsolved");
  
    symir::BlockBuilder::StmtID assignStmts = blockBd->SymAssStmt(
      var,
      blockBd->SymExpr(
        symir::Expr::OP_ADD,
        { blockBd->SymCstTerm(replacedCoef, nullptr) }
      )
    );
  
    blockBd->CommitStmtAt(assignStmts, targetStmtIdx);
  }
  
  bool ConstPropagationViaDiv::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(stmt, m_Expr(m_Any(m_CstTerm(m_Not(m_Eq<const symir::Coef *, int32_t>(INT_MIN)), m_NoVar()))));
  }
  
  void ConstPropagationViaDiv::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running ConstPropagationViaDiv" << std::endl;

    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);
  
    auto rep = utils::StmtReplacer<symir::Term>(funBd, blockBd);
    const symir::VarDef *var = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix);
  
  
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
          Log::Get().Out() 
            << "Replacing Const " << target
            << " with " << v1 << " / " << v2 << std::endl;
  
          *data = thisFunBd->SymI32Const(v2);
          return thisBlockBd->SymDivTerm(thisFunBd->SymI32Const(v1), var);
        };
  
    rep.ReplaceStmt(
      stmt,
      make_matcher(const symir::Term *, m_CstTerm(m_Not(m_Eq<const symir::Coef *, int32_t>(INT_MIN)), m_NoVar())),
      varInsertFun,
      0.25
    );
    symir::Coef *replacedCoef = static_cast<symir::Coef *>(rep.data);
    Assert(replacedCoef != nullptr, "replacedCoef should never be nullptr");
    Assert(replacedCoef->IsSolved(), "replacedCoef should never be unsolved");
  
    symir::BlockBuilder::StmtID assignStmts = blockBd->SymAssStmt(
      var,
      blockBd->SymExpr(
        symir::Expr::OP_ADD,
        { blockBd->SymCstTerm(replacedCoef, nullptr) }
      )
    );
  
    blockBd->CommitStmtAt(assignStmts, targetStmtIdx);
  }

  bool ConstPropagationViaNot::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(stmt, m_Expr(m_Any(m_CstTerm(m_Solved(), m_NoVar()))));
  }

  void ConstPropagationViaNot::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running ConstPropagationViaNot" << std::endl;

    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);
  
    auto rep = utils::StmtReplacer<symir::Term>(funBd, blockBd);
    const symir::VarDef *var = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix);
  
  
    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder *, symir::BlockBuilder *, const symir::Term &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Term &t, void **data) {
          int target = t.GetCoef()->GetI32Value();
          Log::Get().Out() 
            << "Replacing Const " << target
            << " with " << ~target << std::endl;
          *data = thisFunBd->SymI32Const(~target);
          return thisBlockBd->SymNotTerm(nullptr, var);
        };
  
    rep.ReplaceStmt(
      stmt,
      make_matcher(const symir::Term *, m_CstTerm(m_Solved(), m_NoVar())),
      varInsertFun,
      0.25
    );
    symir::Coef *replacedCoef = static_cast<symir::Coef *>(rep.data);
    Assert(replacedCoef != nullptr, "replacedCoef should never be nullptr");
    Assert(replacedCoef->IsSolved(), "replacedCoef should never be unsolved");
  
    symir::BlockBuilder::StmtID assignStmts = blockBd->SymAssStmt(
      var,
      blockBd->SymExpr(
        symir::Expr::OP_ADD,
        { blockBd->SymCstTerm(replacedCoef, nullptr) }
      )
    );
  
    blockBd->CommitStmtAt(assignStmts, targetStmtIdx);
  }

  bool ConstPropagationViaAnd::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(stmt, m_Expr(m_Any(m_CstTerm(m_Solved(), m_NoVar()))));
  }

  void ConstPropagationViaAnd::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running ConstPropagationViaAnd" << std::endl;

    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);
  
    auto rep = utils::StmtReplacer<symir::Term>(funBd, blockBd);
    const symir::VarDef *var = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix);
  
  
    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder *, symir::BlockBuilder *, const symir::Term &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Term &t, void **data) {
          int target = t.GetCoef()->GetI32Value();
          int v1, v2;
          auto rand = Random::Get().Uniform(INT_MIN, INT_MAX);
          v1 = rand();
          v2 = ~v1 & rand();
          Assert((v1 & v2) == 0, "This must be 0");
          // Now v1 & v2 == 0, e.g. each bit either is opposite or both 0

          v1 |= target;
          v2 |= target;
          Assert((v1 & v2) == target, "Faulty Transformation");
          // Now v1 & v2 == target
          Log::Get().Out() 
            << "Replacing Const " << target
            << " with " << v1 << " & " << v2 << std::endl;
          *data = thisFunBd->SymI32Const(v2);
          return thisBlockBd->SymAndTerm(funBd->SymI32Const(v1), var);
        };
  
    rep.ReplaceStmt(
      stmt,
      make_matcher(const symir::Term *, m_CstTerm(m_Solved(), m_NoVar())),
      varInsertFun,
      0.25
    );
    symir::Coef *replacedCoef = static_cast<symir::Coef *>(rep.data);
    Assert(replacedCoef != nullptr, "replacedCoef should never be nullptr");
    Assert(replacedCoef->IsSolved(), "replacedCoef should never be unsolved");
  
    symir::BlockBuilder::StmtID assignStmts = blockBd->SymAssStmt(
      var,
      blockBd->SymExpr(
        symir::Expr::OP_ADD,
        { blockBd->SymCstTerm(replacedCoef, nullptr) }
      )
    );
  
    blockBd->CommitStmtAt(assignStmts, targetStmtIdx);
  }

  bool ConstPropagationViaXor::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(stmt, m_Expr(m_Any(m_CstTerm(m_Solved(), m_NoVar()))));
  }

  void ConstPropagationViaXor::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running ConstPropagationViaXor" << std::endl;

    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);
  
    auto rep = utils::StmtReplacer<symir::Term>(funBd, blockBd);
    const symir::VarDef *var = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix);
  
  
    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder *, symir::BlockBuilder *, const symir::Term &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Term &t, void **data) {
          int target = t.GetCoef()->GetI32Value();
          int v1, v2;
          v1 = Random::Get().Uniform(INT_MIN, INT_MAX)();
          // using Xor identity x ^ y ^ x = y
          v2 = v1 ^ target;
          Log::Get().Out() 
            << "Replacing Const " << target
            << " with " << v1 << " ^ " << v2 << std::endl;
          Assert((v1 ^ v2) == target, "Faulty Transformation");
          
          *data = thisFunBd->SymI32Const(v2);
          return thisBlockBd->SymXorTerm(funBd->SymI32Const(v1), var);
        };
  
    rep.ReplaceStmt(
      stmt,
      make_matcher(const symir::Term *, m_CstTerm(m_Solved(), m_NoVar())),
      varInsertFun,
      0.25
    );
    symir::Coef *replacedCoef = static_cast<symir::Coef *>(rep.data);
    Assert(replacedCoef != nullptr, "replacedCoef should never be nullptr");
    Assert(replacedCoef->IsSolved(), "replacedCoef should never be unsolved");
  
    symir::BlockBuilder::StmtID assignStmts = blockBd->SymAssStmt(
      var,
      blockBd->SymExpr(
        symir::Expr::OP_ADD,
        { blockBd->SymCstTerm(replacedCoef, nullptr) }
      )
    );
  
    blockBd->CommitStmtAt(assignStmts, targetStmtIdx);
  }

  bool ConstPropagationViaOr::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(stmt, m_Expr(m_Any(m_CstTerm(m_Solved(), m_NoVar()))));
  }

  void ConstPropagationViaOr::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running ConstPropagationViaOr" << std::endl;

    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);
  
    auto rep = utils::StmtReplacer<symir::Term>(funBd, blockBd);
    const symir::VarDef *var = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix);
  
  
    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder *, symir::BlockBuilder *, const symir::Term &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Term &t, void **data) {
          int target = t.GetCoef()->GetI32Value();
          int v1, v2;
          v1 = target & Random::Get().Uniform(INT_MIN, INT_MAX)();
          v2 = target & ~v1;
          Assert((v1 | v2) == target, "Faulty Transformation");
          Log::Get().Out() 
            << "Replacing Const " << target
            << " with " << v1 << " | " << v2 << std::endl;
          *data = thisFunBd->SymI32Const(v2);
          return thisBlockBd->SymOrTerm(funBd->SymI32Const(v1), var);
        };

    rep.ReplaceStmt(
      stmt,
      make_matcher(const symir::Term *, m_CstTerm(m_Solved(), m_NoVar())),
      varInsertFun,
      0.25
    );
    symir::Coef *replacedCoef = static_cast<symir::Coef *>(rep.data);
    Assert(replacedCoef != nullptr, "replacedCoef should never be nullptr");
    Assert(replacedCoef->IsSolved(), "replacedCoef should never be unsolved");
  
  
    symir::BlockBuilder::StmtID assignStmts = blockBd->SymAssStmt(
      var,
      blockBd->SymExpr(
        symir::Expr::OP_ADD,
        { blockBd->SymCstTerm(replacedCoef, nullptr) }
      )
    );
  
    blockBd->CommitStmtAt(assignStmts, targetStmtIdx);
  }

  bool ConstPropagationViaShl::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(
      stmt,
      m_Expr(m_Any(
        m_CstTerm(
          // Ensure atleast one left shift is possible
          // (e.g. target should not be negative and have atleast one bit 0 to be right shifted)
          m_Value(m_UnsetBits(~0x7FFFFFFE)),
          m_NoVar()
        )
      ))
    );
  }

  void ConstPropagationViaShl::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running ConstPropagationViaShl" << std::endl;

    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);
  
    auto rep = utils::StmtReplacer<symir::Term>(funBd, blockBd);
    const symir::VarDef *var = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix);
  
  
    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder *, symir::BlockBuilder *, const symir::Term &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Term &t, void **data) {
          int target = t.GetCoef()->GetI32Value();
          int v1, v2;
          // TODO: __buildin_ctz is compiler specific
          v2 = Random::Get().Uniform(1, target == 0 ? 31: __builtin_ctz(target))();
          // Since target is non negative right shift is fully defined by the standart to be a logical shift e.g. the v2 + 1 most significant bits are now 0
          v1 = target;
          v1 >>= v2;
          *data = thisFunBd->SymI32Const(v1);
          return thisBlockBd->SymShlTerm(funBd->SymI32Const(v2), var);
        };

    rep.ReplaceStmt(
      stmt,
      make_matcher(
        const symir::Term *,
        m_CstTerm(m_Value(m_UnsetBits(~0x7FFFFFFE)), m_NoVar())
      ),
      varInsertFun,
      0.25
    );
    symir::Coef *replacedCoef = static_cast<symir::Coef *>(rep.data);
    Assert(replacedCoef != nullptr, "replacedCoef should never be nullptr");
    Assert(replacedCoef->IsSolved(), "replacedCoef should never be unsolved");
  
    symir::BlockBuilder::StmtID assignStmts = blockBd->SymAssStmt(
      var,
      blockBd->SymExpr(
        symir::Expr::OP_ADD,
        { blockBd->SymCstTerm(replacedCoef, nullptr) }
      )
    );
  
    blockBd->CommitStmtAt(assignStmts, targetStmtIdx);
  }

  bool ConstPropagationViaShr::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(
      stmt,
      m_Expr(m_Any(
        m_CstTerm(
          // Ensure atleast one right shift is possible
          // (e.g. target should have atleast one most sig. bit free to shift left and 1 to shift back right)
          m_Value(m_UnsetBits(~0x3FFFFFFE)),
          m_NoVar()
        )
      ))
    );
  }

  void ConstPropagationViaShr::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running ConstPropagationViaShr" << std::endl;

    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);
  
    auto rep = utils::StmtReplacer<symir::Term>(funBd, blockBd);
    const symir::VarDef *var = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix);
  
  
    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder *, symir::BlockBuilder *, const symir::Term &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Term &t, void **data) {
          int target = t.GetCoef()->GetI32Value();
          int v1, v2;
          // TODO: __buildin_ctz is compiler specific
          v2 = Random::Get().Uniform(1, target == 0 ? 31: __builtin_clz(target))() - 1;
          // Since target is non negative that does not overflow on a left shift of v2
          // it is fully defined by the standart and not UB
          // e.g. the v2 least significant bits are now 0
          v1 = target;
          v1 <<= v2;
          *data = thisFunBd->SymI32Const(v1);
          return thisBlockBd->SymShrTerm(funBd->SymI32Const(v2), var);
        };

    rep.ReplaceStmt(
      stmt,
      make_matcher(
        const symir::Term *,
        m_CstTerm(m_Value(m_UnsetBits(~0x3FFFFFFE)), m_NoVar())
      ),
      varInsertFun,
      0.25
    );
    symir::Coef *replacedCoef = static_cast<symir::Coef *>(rep.data);
    Assert(replacedCoef != nullptr, "replacedCoef should never be nullptr");
    Assert(replacedCoef->IsSolved(), "replacedCoef should never be unsolved");
  
    symir::BlockBuilder::StmtID assignStmts = blockBd->SymAssStmt(
      var,
      blockBd->SymExpr(
        symir::Expr::OP_ADD,
        { blockBd->SymCstTerm(replacedCoef, nullptr) }
      )
    );
  
    blockBd->CommitStmtAt(assignStmts, targetStmtIdx);
  }

  bool Reg2Mem::match(const symir::Stmt *stmt) const {
    return patternmatch::match(
      stmt,
      m_AssStmt(m_And(m_WithType(symir::SymIR::Type::I32), m_ScalarVar()), m_WildCard<const symir::Expr *>())
    );
  }

  void Reg2Mem::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    Log::Get().Out() << "Running Reg2Mem" << std::endl;

    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmt(targetStmtIdx);

    const symir::AssStmt *assStmt = static_cast<const symir::AssStmt *>(stmt);
    const symir::VarDef *memVar = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix, 1);
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
    
    blockBd->ReplaceCommitStmt({ memAssign, assignBack }, targetStmtIdx);
  }

} // namespace 

