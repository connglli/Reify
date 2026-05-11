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


#include <climits>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <string>

#include "lib/patternmatch.hpp"
#include "lib/transformations.hpp"
#include "lib/lang.hpp"
#include "lib/random.hpp"
#include "lib/logger.hpp"

using namespace patternmatch;

namespace {
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
  bool matchSubExprInAnyStmt(const symir::Stmt *stmt, const Pattern<const symir::Expr *> &E) {
    return patternmatch::match(
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

template<typename Node>
void StmtReplacer<Node>::Visit(const Node &n) { StmtCopier::Visit(n); }

template<typename Node>
symir::BlockBuilder::StmtID StmtReplacer<Node>::CopyStmtWithReplacement(
  const symir::Stmt *s, 
  std::function<bool(const Node *)> matchFunction,
  std::function<ExprID(symir::FunctBuilder *, symir::BlockBuilder *, const Node &, void **)> replaceFunction,
  double randThreshold,
  size_t replaceMax
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

void RewriteEngine::addRule(std::unique_ptr<Rule> rule, int weight) {
  rules.push_back(std::move(rule));
  weights.push_back(weight);
}

void RewriteEngine::run(symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd, size_t times) const {
  Log::Get().OpenSection("RewriteEngine::run() for " + blockBd->GetLabel());
  Log::Get().Out() << "Running randomized RewriteEngine " << times 
                   << " times with " << this->rules.size() << " rules" << std::endl;

  for (size_t t = 0; t < times; t++) {
    size_t nrStmts = blockBd->GetNumberCommitedStmt();
    Assert(nrStmts > 0, "to rewrite a block it needs atleast one stmt");
    auto randStmt = Random::Get().Uniform(0, static_cast<int>(nrStmts)-1)();
    const auto *stmt = blockBd->GetCommitedStmt(randStmt);
    std::optional rewrittenStmt = this->runInSubStmt(funBd, blockBd, stmt);
    if (rewrittenStmt.has_value())
      stmt = blockBd->SymReplaceCommitStmt({ rewrittenStmt.value() }, randStmt)[0];

    std::optional rule = this->getRandomMatchingRule(stmt);
    if (rule.has_value()) {
      std::vector<symir::BlockBuilder::StmtID> newStmts = rule.value()->rewrite(funBd, blockBd, stmt);
      Assert(newStmts.size() > 0, "rewrite deleted all stmts");
      blockBd->SymReplaceCommitStmt(newStmts, randStmt);
    }
  }
  Log::Get().CloseSection();
}

std::optional<symir::BlockBuilder::StmtID> RewriteEngine::runInSubStmt(
  symir::FunctBuilder *funBd,
  symir::BlockBuilder *blockBd,
  const symir::Stmt *stmt
) const {
  switch (stmt->GetIRId()) {
  case symir::SymIR::SIR_STMT_FOR:
    return this->runInForStmt(funBd, blockBd, static_cast<const symir::ForStmt *>(stmt));
  case symir::SymIR::SIR_STMT_WHILE:
    return this->runInWhileStmt(funBd, blockBd, static_cast<const symir::WhileStmt *>(stmt));
  case symir::SymIR::SIR_STMT_IF:
    return this->runInIfStmt(funBd, blockBd, static_cast<const symir::IfStmt *>(stmt));
  default: return {};
  }
}

std::optional<symir::BlockBuilder::StmtID> RewriteEngine::runInForStmt(
  symir::FunctBuilder *funBd,
  symir::BlockBuilder *blockBd,
  const symir::ForStmt *forStmt
) const {
  auto body = forStmt->GetBody();
  size_t nrStmts = body.size();
  bool hasRewritten = false;
  std::vector<symir::BlockBuilder::StmtID> newBody;
  newBody.reserve(nrStmts);
  for (size_t j = 0; j < nrStmts; j++) {
    auto randStmt = Random::Get().Uniform(0, static_cast<int>(nrStmts)-1)();
    for (size_t i = 0; i < nrStmts; i++) {
      const auto *stmt = body[i];

      std::optional rewrittenStmt = this->runInSubStmt(funBd, blockBd, stmt);
      if (i != static_cast<size_t>(randStmt)) {
        newBody.push_back(symir::StmtCopier(funBd, blockBd).CopyStmt(stmt));
        continue;
      }
      if (rewrittenStmt.has_value())
        stmt = blockBd->GetUncommitedStmt(rewrittenStmt.value());

      std::optional rule = this->getRandomMatchingRule(stmt);
      if (rule.has_value()) {
        std::vector<symir::BlockBuilder::StmtID> newStmts = rule.value()->rewrite(funBd, blockBd, stmt);
        for (auto newStmt : newStmts) newBody.push_back(newStmt);
        hasRewritten = true;
      } else if (rewrittenStmt.has_value()) {
        newBody.push_back(rewrittenStmt.value());
        hasRewritten = true;
      }
    }
    if (hasRewritten) break;
    else newBody.clear();
  }

  auto copier = symir::StmtCopier(funBd, blockBd);
  return blockBd->SymForStmt(
    forStmt->GetVar()->GetDef(),
    copier.CopyCond(forStmt->GetCond()),
    copier.CopyExpr(forStmt->GetInit()),
    copier.CopyExpr(forStmt->GetIncrement()),
    newBody
  );
}

std::optional<symir::BlockBuilder::StmtID> RewriteEngine::runInWhileStmt(
  symir::FunctBuilder *funBd,
  symir::BlockBuilder *blockBd,
  const symir::WhileStmt *whileStmt
) const {
  auto body = whileStmt->GetBody();
  size_t nrStmts = body.size();
  bool hasRewritten = false;
  std::vector<symir::BlockBuilder::StmtID> newBody;
  newBody.reserve(body.size());
  for (size_t j = 0; j < nrStmts; j++) {
    newBody.reserve(nrStmts);
    auto randStmt = Random::Get().Uniform(0, static_cast<int>(nrStmts)-1)();
    for (size_t i = 0; i < nrStmts; i++) {
      const auto *stmt = body[i];

      std::optional rewrittenStmt = this->runInSubStmt(funBd, blockBd, stmt);
      if (i != static_cast<size_t>(randStmt)) {
        newBody.push_back(symir::StmtCopier(funBd, blockBd).CopyStmt(stmt));
        continue;
      }
      if (rewrittenStmt.has_value())
        stmt = blockBd->GetUncommitedStmt(rewrittenStmt.value());

      std::optional rule = this->getRandomMatchingRule(stmt);
      if (rule.has_value()) {
        std::vector<symir::BlockBuilder::StmtID> newStmts = rule.value()->rewrite(funBd, blockBd, stmt);
        for (auto newStmt : newStmts) newBody.push_back(newStmt);
        hasRewritten = true;
      } else if (rewrittenStmt.has_value()) {
        newBody.push_back(rewrittenStmt.value());
        hasRewritten = true;
      }
    }
    if (hasRewritten) break;
    else newBody.clear();
  }

  auto copier = symir::StmtCopier(funBd, blockBd);
  return blockBd->SymWhileStmt(copier.CopyCond(whileStmt->GetCond()), newBody);
}

std::optional<symir::BlockBuilder::StmtID> RewriteEngine::runInIfStmt(
  symir::FunctBuilder *funBd,
  symir::BlockBuilder *blockBd,
  const symir::IfStmt *ifStmt
) const {
  auto randDouble = Random::Get().UniformReal();
  auto bodies = ifStmt->GetBodies();
  std::vector<std::vector<symir::BlockBuilder::StmtID>> newBodies;
  newBodies.resize(bodies.size());
  for (size_t j = 0; j < bodies.size(); j++) {
    auto body = bodies[j];
    size_t nrStmts = body.size();
    bool hasRewritten = false;
    newBodies[j].reserve(nrStmts);
    for (size_t k = 0; k < nrStmts; k++) {
      auto randStmt = Random::Get().Uniform(0, static_cast<int>(nrStmts)-1)();
      for (size_t i = 0; i < nrStmts; i++) {
        const auto *stmt = body[i];

        std::optional rewrittenStmt = this->runInSubStmt(funBd, blockBd, stmt);
        if (i != static_cast<size_t>(randStmt)) {
          newBodies[j].push_back(symir::StmtCopier(funBd, blockBd).CopyStmt(stmt));
          continue;
        }
        if (rewrittenStmt.has_value())
          stmt = blockBd->GetUncommitedStmt(rewrittenStmt.value());

        std::optional rule = this->getRandomMatchingRule(stmt);
        if (rule.has_value()) {
          std::vector<symir::BlockBuilder::StmtID> newStmts = rule.value()->rewrite(funBd, blockBd, stmt);
          for (auto newStmt : newStmts) newBodies[j].push_back(newStmt);
          hasRewritten = true;
        } else if (rewrittenStmt.has_value()) {
          newBodies[j].push_back(rewrittenStmt.value());
          hasRewritten = true;
        }
      }
      if (hasRewritten) break;
      else newBodies[j].clear();
    }
  } 

  auto copier = symir::StmtCopier(funBd, blockBd);
  std::vector<symir::BlockBuilder::CondID> cids;
  auto conds = ifStmt->GetConds();
  cids.reserve(conds.size());
  for (auto cond : conds) {
    cids.push_back(copier.CopyCond(cond));
  }
  return blockBd->SymIfStmt(cids, newBodies);
}

std::optional<Rule *> RewriteEngine::getRandomMatchingRule(const symir::Stmt *stmt) const {
  std::vector<size_t> matchingRules;
  int totalWeight = 0;
  for (size_t i = 0; i < this->rules.size(); i++) {
    if (!this->rules[i]->match(stmt)) continue;
    matchingRules.push_back(i);
    totalWeight += this->weights[i];
  }

  if (totalWeight == 0) return {};
  int selWeight = Random::Get().Uniform(0, totalWeight)();
  int index = 0;
  while (selWeight > this->weights[matchingRules[index]]) selWeight -= this->weights[matchingRules[index++]];
  return this->rules[matchingRules[index]].get();
}

bool ConstProba::match(const symir::Stmt *stmt) const {
  return matchSubExprInAnyStmt(stmt, m_Expr(m_Any(m_CstTerm(m_Solved(), m_NoVar()))));
}

std::vector<symir::BlockBuilder::StmtID> ConstProba::rewrite(
  symir::FunctBuilder *funBd,
  symir::BlockBuilder *blockBd,
  const symir::Stmt *stmt
) {
  Log::Get().Out() << "Running ConstProba" << std::endl;

  StmtReplacer rep = StmtReplacer<symir::Term>(funBd, blockBd);
  const symir::VarDef *var = this->getNewLocal(funBd);
  std::function<symir::BlockBuilder::TermID(symir::FunctBuilder * ,symir::BlockBuilder *, const symir::Term &, void **)>
    varInsertFun =
      [&](symir::FunctBuilder *thisFunBd, symir::BlockBuilder *thisBlockBd, const symir::Term &t, void **data) {
        *data = t.GetCoef();
        return thisBlockBd->SymMulTerm(thisFunBd->SymI32Const(1), var);
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


bool AdditionFromConst::match(const symir::Stmt *stmt) const {
  return matchSubExprInAnyStmt(stmt, m_Expr(m_Any(m_CstTerm(m_Solved(), m_NoVar()))));
}

std::vector<symir::BlockBuilder::StmtID> AdditionFromConst::rewrite(
  symir::FunctBuilder *funBd,
  symir::BlockBuilder *blockBd,
  const symir::Stmt *stmt
) {
  Log::Get().Out() << "Running AdditionFromConst" << std::endl;

  StmtReplacer rep = StmtReplacer<symir::Expr>(funBd, blockBd);
  const symir::VarDef *var = this->getNewLocal(funBd);
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

            termIds.push_back(blockBd->SymTerm(
              symir::Term::OP_CST,
              funBd->SymI32Const(v1),
              nullptr, {})
            );
            termIds.push_back(blockBd->SymTerm(
              symir::Term::OP_CST,
              funBd->SymI32Const(v2),
              nullptr, {})
            );
          } else {
            termIds.push_back(symir::StmtCopier(funBd, blockBd).CopyTerm(term));
          }
        }

      return blockBd->SymExpr(e.GetOp(), termIds);
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

  auto access = copyAccess(funBd, use);

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

  auto loopVar = this->getNewLocal(funBd);
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
  auto access = copyAccess(funBd, use);

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
        conds.push_back(triviallyTrueCond(funBd, blockBd));
      } else {
        conds.push_back(triviallyFalseCond(funBd, blockBd));
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

std::vector<const symir::Term *> ConstQuery::query() {
  for (size_t i = 0; i < this->blockBd->GetNumberCommitedStmt(); i++) {
    this->blockBd->GetCommitedStmt(i)->Accept(*this);
  }
  std::vector<const symir::Term *> res;
  res.reserve(this->terms.size());
  for (auto &t: this->terms) {
    res.push_back(t);
  }
  this->terms.clear();
  return res;
}

void ConstQuery::Visit(const symir::VarUse &v) {
  for (auto &c: v.GetAccess()) {
    c->Accept(*this);
  }
}

void ConstQuery::Visit(const symir::Term &t) {
  if (t.GetOp() == symir::Term::OP_CST) {
    this->terms.push_back(&t);
  }
}

void ConstQuery::Visit(const symir::Expr &e) {
  for (auto &t : e.GetTerms()) t->Accept(*this);
}

void ConstQuery::Visit(const symir::Cond &c) {
  c.GetExpr()->Accept(*this);
}

void ConstQuery::Visit(const symir::AssStmt &a) {
  a.GetVar()->Accept(*this);
  a.GetExpr()->Accept(*this);
}

void ConstQuery::Visit(const symir::IfStmt &i) {
  const auto conds = i.GetConds();
  const auto bodies = i.GetBodies();

  for (size_t i = 0; i < conds.size(); i++) {
    conds[i]->Accept(*this);
  }

  for (size_t i = 0; i < bodies.size(); i++) {
    for (size_t j = 0; j < bodies[i].size(); j++) {
      Assert(
        bodies[i][j]->GetIRId() != symir::SymIR::SIR_TGT_BRA || bodies[i][j]->GetIRId() != symir::SymIR::SIR_TGT_GOTO,
        "IfStmt contains Goto or Branch"
      );
      bodies[i][j]->Accept(*this);
    }
  }
}

void ConstQuery::Visit(const symir::ForStmt &f) {
  const symir::Expr *init = f.GetInit();
  const symir::Expr *increment = f.GetIncrement();
  const auto cond = f.GetCond();
  const auto body = f.GetBody();

  cond->Accept(*this);
  init->Accept(*this);
  increment->Accept(*this);

  for (size_t i = 0; i < body.size(); i++) {
    Assert(
      body[i]->GetIRId() != symir::SymIR::SIR_TGT_BRA || body[i]->GetIRId() != symir::SymIR::SIR_TGT_GOTO,
      "ForStmt contains Goto or Branch"
    );
    body[i]->Accept(*this);
  }
}

void ConstQuery::Visit(const symir::WhileStmt &w) {
  const auto cond = w.GetCond();
  const auto body = w.GetBody();

  cond->Accept(*this);

  for (size_t i = 0; i < body.size(); i++) {
    Assert(
      body[i]->GetIRId() != symir::SymIR::SIR_TGT_BRA || body[i]->GetIRId() != symir::SymIR::SIR_TGT_GOTO,
      "ForStmt contains Goto or Branch"
    );
    body[i]->Accept(*this);
  }
}

void VariableEmbedder::embed(std::map<const symir::Term *, symir::BlockBuilder::TermID> varMap) {
  this->varMap = varMap;
  for (size_t i = 0; i < this->blockBd->GetNumberCommitedStmt(); i++) {
    auto s = this->blockBd->GetCommitedStmt(i);
    s->Accept(*this);
    StmtID sid = popStmt();
    this->blockBd->SymReplaceCommitStmt({ sid }, i);
  }
}

void VariableEmbedder::Visit(const symir::VarUse &v) {
  for (auto &c: v.GetAccess()) {
      c->Accept(*this);
  }
}

void VariableEmbedder::Visit(const symir::Coef &c) {
  if (auto coef = this->funBd->FindSymbol(c.GetName()); coef != nullptr) {
    Assert(
        typeid(*coef) == typeid(symir::Coef),
        "Symbol \"%s\" is already defined and is not a coefficient", c.GetName().c_str()
    );
    pushCoef(dynamic_cast<symir::Coef *>(coef));
  } else if (c.IsSolved()) {
    pushCoef(this->funBd->SymCoef(c.GetName(), c.GetValue(), c.GetType()));
  } else {
    pushCoef(this->funBd->SymCoef(c.GetName(), c.GetType()));
  }
}

void VariableEmbedder::Visit(const symir::Term &t) {
  if (this->varMap.contains(&t)) {
    pushTerm(this->varMap[&t]);
  } else {
    t.GetCoef()->Accept(*this);
    const symir::VarDef *var = nullptr;
    std::vector<symir::Coef *> access{};
    if (t.GetOp() != symir::Term::Op::OP_CST) {
      const auto name = t.GetVar()->GetName();
      var = t.GetVar()->GetDef();
      Assert(var != nullptr, "Variable \"%s\" does not exist", name.c_str());
      t.GetVar()->Accept(*this);
      for (size_t i = 0; i < t.GetVar()->GetAccess().size(); i++) {
        access.insert(access.begin(), popCoef());
      }
    }
    pushTerm(this->blockBd->SymTerm(t.GetOp(), popCoef(), var, access));
  }
}

void VariableEmbedder::Visit(const symir::ModExpr &e) {
    std::vector<symir::Coef *> coeffs;
    for (const auto& c : e.GetCoeffs()) {
      c->Accept(*this);
      auto coeff = popCoef();
      coeffs.push_back(coeff);
    }
    std::vector<const symir::VarDef *> variables;

    const symir::VarDef *var = nullptr;
    std::vector<std::vector<symir::Coef *>> accesses{};
    for (const auto& use: e.GetVars()) {
      std::vector<symir::Coef *> access{};
      const auto name = use->GetName();
      var = use->GetDef();
      Assert(var != nullptr, "Variable \"%s\" does not exist", name.c_str());
      use->Accept(*this);
      for (size_t i = 0; i < use->GetAccess().size(); i++) {
        access.insert(access.begin(), popCoef());
      }
      variables.push_back(var);
      accesses.push_back(std::vector(access));
    }

    const std::vector<int> polynomial = e.GetPolynomial();
    const int mod = e.GetMod();

    pushModExpr(this->blockBd->SymModExpr(
      coeffs,
      variables,
      accesses,
      polynomial,
      mod
    ));
}

void VariableEmbedder::Visit(const symir::Expr &e) {
  const auto &terms = e.GetTerms();
  std::vector<TermID> termIds;
  for (const auto &t: terms) {
    t->Accept(*this);
    termIds.push_back(popTerm());
  }
  pushExpr(this->blockBd->SymExpr(e.GetOp(), termIds));
}

void VariableEmbedder::Visit(const symir::Cond &c) {
  c.GetExpr()->Accept(*this);
  ExprID exprId = popExpr();
  pushCond(this->blockBd->SymCond(c.GetOp(), exprId));
}

void VariableEmbedder::Visit(const symir::ModAssStmt &a) {
  const auto use = a.GetVar();
  const auto expr = a.GetExpr();
  const auto name = use->GetName();
  const auto var = use->GetDef();
  Assert(var != nullptr, "Variable \"%s\" does not exist", name.c_str());

  use->Accept(*this);
  expr->Accept(*this);
  auto modExprId = popModExpr();
  std::vector<symir::Coef *> access{};
  for (size_t i = 0; i < use->GetAccess().size(); i++) {
    access.insert(access.begin(), popCoef());
  }

  pushStmt(this->blockBd->SymModAssStmt(var, modExprId, access));
}

void VariableEmbedder::Visit(const symir::AssStmt &a) {
  const auto use = a.GetVar();
  const auto name = use->GetName();
  const auto var = use->GetDef();
  Assert(var != nullptr, "Variable \"%s\" does not exist", name.c_str());

  use->Accept(*this);
  std::vector<symir::Coef *> access{};
  for (size_t i = 0; i < use->GetAccess().size(); i++) {
    access.insert(access.begin(), popCoef());
  }

  a.GetExpr()->Accept(*this);
  ExprID exprId = popExpr();

  pushStmt(this->blockBd->SymAssStmt(var, exprId, access));
}

void VariableEmbedder::Visit(const symir::IfStmt &i) {
  const auto conds = i.GetConds();
  const auto bodies = i.GetBodies();

  std::vector<CondID> cids;
  cids.resize(conds.size());
  for (size_t i = 0; i < conds.size(); i++) {
    conds[i]->Accept(*this);
    cids[i] = popCond();
  }

  std::vector<std::vector<StmtID>> sids;
  sids.resize(bodies.size());
  for (size_t i = 0; i < bodies.size(); i++) {
    sids[i].resize(bodies[i].size());
    for (size_t j = 0; j < bodies[i].size(); j++) {
      Assert(
        bodies[i][j]->GetIRId() != symir::SymIR::SIR_TGT_BRA || bodies[i][j]->GetIRId() != symir::SymIR::SIR_TGT_GOTO,
        "IfStmt contains Goto or Branch"
      );
      bodies[i][j]->Accept(*this);
      sids[i][j] = popStmt();
    }
  }

  pushStmt(this->blockBd->SymIfStmt(cids, sids));
}

void VariableEmbedder::Visit(const symir::ForStmt &f) {
  const symir::VarUse *use = f.GetVar();
  const symir::Expr *init = f.GetInit();
  const symir::Expr *increment = f.GetIncrement();
  const auto cond = f.GetCond();
  const auto body = f.GetBody();

  use->Accept(*this);
  std::vector<symir::Coef *> access{};
  for (size_t i = 0; i < use->GetAccess().size(); i++) {
    access.insert(access.begin(), popCoef());
  }

  cond->Accept(*this);
  CondID cid = popCond();

  init->Accept(*this);
  ExprID initID = popExpr();

  increment->Accept(*this);
  ExprID incrementID = popExpr();

  std::vector<StmtID> sids;
  sids.resize(body.size());
  for (size_t i = 0; i < body.size(); i++) {
    Assert(body[i]->GetIRId() != symir::SymIR::SIR_TGT_BRA || body[i]->GetIRId() != symir::SymIR::SIR_TGT_GOTO, "ForStmt contains Goto or Branch");
    body[i]->Accept(*this);
    sids[i] = popStmt();
  }

  pushStmt(this->blockBd->SymForStmt(use->GetDef(), cid, initID, incrementID, sids, access));
}

void VariableEmbedder::Visit(const symir::WhileStmt &w) {
  const auto cond = w.GetCond();
  const auto body = w.GetBody();

  cond->Accept(*this);
  CondID cid = popCond();

  std::vector<StmtID> sids;
  sids.resize(body.size());
  for (size_t i = 0; i < body.size(); i++) {
    Assert(
      body[i]->GetIRId() != symir::SymIR::SIR_TGT_BRA || body[i]->GetIRId() != symir::SymIR::SIR_TGT_GOTO,
      "ForStmt contains Goto or Branch"
    );
    body[i]->Accept(*this);
    sids[i] = popStmt();
  }

  pushStmt(this->blockBd->SymWhileStmt(cid, sids));
}

