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

