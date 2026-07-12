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

// Contains Transformation Rules that obscure the transformation from the compiler using functions locals

#include "lib/patternmatch.hpp"
#include "lib/Transformations/obscure.hpp"
#include "lib/Transformations/utils.hpp"
#include <algorithm>
#include <climits>

using namespace patternmatch;
namespace transformations::obscure {

  bool Conditional::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(stmt, m_Expr(m_Any(m_CstTerm(m_Solved(), m_NoVar()))));
  }

  void Conditional::rewrite(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t targetBlockIdx,
    size_t targetStmtIdx
  ) const {
    if (varState.nrVariables == 0) return;
    symir::BlockBuilder *blockBd = blockBds[targetBlockIdx];
    const symir::Stmt *stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);
    bool isTarget = blockBd->GetNumberOfCommitedStmt() == targetStmtIdx;
    const symir::VarDef *var = utils::getVariable(funBd, blockBds[0]->GetLabel(), this->varPrefix);
    auto rep = utils::StmtReplacer<symir::Term>(funBd, blockBd);
    int32_t target;
    rep.data = &target;

    std::function<symir::BlockBuilder::TermID(symir::FunctBuilder * ,symir::BlockBuilder *, const symir::Term &, void **)>
      varInsertFun =
        [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Term &t, void **data) {
        
        int32_t target = t.GetCoef()->GetI32Value();

        int32_t *targetPair = static_cast<int32_t *>(*data);
        *targetPair = target;

        return utils::variableTerm(funBd, blockBd, var);
      };
    rep.ReplaceStmt(
      stmt,
      make_matcher(const symir::Term *, m_CstTerm(m_Solved(), m_NoVar())),
      varInsertFun,
      0.25
    );
    stmt = blockBd->GetCommitedStmtOrTarget(targetStmtIdx);

    size_t randVariableIdx = Random::Get().Uniform(0, static_cast<int>(varState.nrVariables) - 1)();
    // find last index that is the start of a variable 
    size_t variableStartIdx = randVariableIdx;
    while (!varState.varMap.contains(variableStartIdx)) variableStartIdx -= 1;

    const symir::VarDef *local_var = funBd->FindVar(varState.varMap[variableStartIdx]);
    Assert(
      var != nullptr,
      "var %s not found in fun %s",
      varState.varMap[randVariableIdx].c_str(),
      funBd->GetName().c_str()
    );
    auto access = utils::unflattenAccess(funBd, local_var, randVariableIdx - variableStartIdx);


    size_t nrIters = varState.varState.size() / varState.nrVariables;
    int32_t falseVal;

    { // find a unique unused value in varState
      std::vector<int32_t> sortedTmpVarState;
      sortedTmpVarState.resize(nrIters);

      for (size_t i = 0; i < nrIters; i++) {
        Assert(i * varState.nrVariables + randVariableIdx < varState.varState.size(), "rand variable out of scope");
        sortedTmpVarState[i] = varState.varState[i * varState.nrVariables + randVariableIdx];
      }
      std::sort(sortedTmpVarState.begin(), sortedTmpVarState.end());

      size_t randIterationIdx = Random::Get().Uniform(0, static_cast<int>(nrIters) - 1)();
      falseVal = sortedTmpVarState[randIterationIdx];

      int32_t val;
      if (sortedTmpVarState[0] == INT_MAX) falseVal = Random::Get().Uniform(0, INT_MAX - 1)();
      while ((val = sortedTmpVarState[randIterationIdx]) == falseVal) {
        if (val == INT_MAX) {
          randIterationIdx = 0;
          falseVal = sortedTmpVarState[0] + 1;
        }
        falseVal = val + 1;
        randIterationIdx = (randIterationIdx + 1) % sortedTmpVarState.size();
      }
    }


    std::string bodyLabel = utils::nameLabel(funBd->GetName(), "if_true_" + this->varPrefix);
    std::string condLabel = utils::nameLabel(funBd->GetName(), "if_cond_" + this->varPrefix);
    std::string elseBodyLabel = utils::nameLabel(funBd->GetName(), "if_false_" + this->varPrefix);
    std::string exitLabel = utils::nameLabel(funBd->GetName(), "if_exit_" + this->varPrefix);

    symir::BlockBuilder *secondBlockBd = utils::splitBlockAt(funBd, blockBd, exitLabel, targetStmtIdx);
    symir::StmtCopier secondCopier = symir::StmtCopier(funBd, secondBlockBd);
    if (!isTarget) {
      secondBlockBd->CommitStmtAt(secondCopier.CopyStmt(stmt), 0);
      blockBd->RemoveCommittedStmts(targetStmtIdx, targetStmtIdx);
    }

    // if_true body
    symir::BlockBuilder *bodyBd = funBd->OpenBlock(bodyLabel);
    bodyBd->CommitStmt(
      bodyBd->SymAssStmt(
        var,
        bodyBd->SymExpr(
          utils::randomExprOp(),
          { bodyBd->SymCstTerm(funBd->SymI32Const(Random::Get().Uniform(INT_MIN, INT_MAX)()), nullptr) }
        )
      )
    );
    bodyBd->SymGoto(exitLabel);

    // if_false body
    symir::BlockBuilder *elseBodyBd = funBd->OpenBlock(elseBodyLabel);
    elseBodyBd->CommitStmt(
      elseBodyBd->SymAssStmt(
        var,
        elseBodyBd->SymExpr(utils::randomExprOp(), { elseBodyBd->SymCstTerm(funBd->SymI32Const(target), nullptr) })
      )
    );
    elseBodyBd->SymGoto(exitLabel);

    if (falseVal > 0) {
      blockBd->SymBranch(condLabel, elseBodyLabel,
        blockBd->SymCond(
          symir::Cond::OP_GTZ,
          blockBd->SymExpr(utils::randomExprOp(), {
            utils::variableTerm(funBd, blockBd, local_var, access) 
          })
        )
      );
    } else {
      blockBd->SymBranch(elseBodyLabel, condLabel,
        blockBd->SymCond(
          symir::Cond::OP_GTZ,
          blockBd->SymExpr(utils::randomExprOp(), {
            utils::variableTerm(funBd, blockBd, local_var, access) 
          })
        )
      );
    }

    symir::BlockBuilder *condBd = funBd->OpenBlock(condLabel);
    condBd->SymBranch(bodyLabel, elseBodyLabel, 
      condBd->SymCond(
        symir::Cond::OP_EQZ,
        condBd->SymSubExpr({
          utils::variableTerm(funBd, condBd, local_var, access),
          condBd->SymCstTerm(funBd->SymI32Const(falseVal), nullptr)
        })
      )
    );


    utils::insertBlockBd(blockBds, {condBd, bodyBd, elseBodyBd, secondBlockBd}, targetBlockIdx + 1);
  }

  bool PrimeInterp::match(const symir::Stmt *stmt) const {
    return utils::matchSubExprInAnyStmt(stmt, m_Expr(m_Any(m_CstTerm(m_Solved(), m_NoVar()))));
  }

  void PrimeInterp::rewrite(
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

} // namespace 
