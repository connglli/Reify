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
#include <ostream>
#include <string>

#include "lib/transformations.hpp"
#include "lib/lang.hpp"
#include "lib/random.hpp"
#include "lib/logger.hpp"

namespace {
  bool stmtHasConstTerm(const symir::Stmt *stmt);

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

  bool exprHasConstTerm(const symir::Expr *expr) {
    for (const auto &term : expr->GetTerms()) {
      if (term->GetOp() != symir::Term::OP_CST) continue;
      if (term->GetCoef()->IsSolved()) return true;
    }
    return false;
  }

  bool assignHasConstTerm(const symir::AssStmt *assStmt) {
    Assert(assStmt != nullptr, "Cast is checked and should not fail");
    return exprHasConstTerm(assStmt->GetExpr());
  }

  bool ifHasConstTerm(const symir::IfStmt *ifStmt) {
    Assert(ifStmt != nullptr, "Cast is checked and should not fail");
    for (const auto &cond : ifStmt->getConds()) {
      if (exprHasConstTerm(cond->GetExpr())) return true;
    }
    for (const auto &body : ifStmt->getBodies()) {
      for (const auto &stmt : body) {
        if (stmtHasConstTerm(stmt)) return true;
      }
    }
    return false;
  }

  bool forHasConstTerm(const symir::ForStmt *forStmt) {
    Assert(forStmt != nullptr, "Cast is checked and should not fail");
    if (exprHasConstTerm(forStmt->GetInit())) return true;
    if (exprHasConstTerm(forStmt->GetIncrement())) return true;
    if (exprHasConstTerm(forStmt->GetCond()->GetExpr())) return true;
    for (const auto &stmt: forStmt->GetBody()) {
      if (stmtHasConstTerm(stmt)) return true;
    }
    return false;
  }

  bool whileHasConstTerm(const symir::WhileStmt *whileStmt) {
    Assert(whileStmt != nullptr, "Cast is checked and should not fail");
    if (exprHasConstTerm(whileStmt->GetCond()->GetExpr())) return true;
    for (const auto &stmt: whileStmt->GetBody()) {
      if (stmtHasConstTerm(stmt)) return true;
    }
    return false;
  }

  bool stmtHasConstTerm(const symir::Stmt *stmt) {
    switch(stmt->GetIRId()) {
    case symir::SymIR::SIR_STMT_ASS: {
      return assignHasConstTerm(static_cast<const symir::AssStmt *>(stmt));
    } break;
    case symir::SymIR::SIR_STMT_IF: {
      return ifHasConstTerm(static_cast<const symir::IfStmt *>(stmt));
    } break;
    case symir::SymIR::SIR_STMT_FOR: {
      return forHasConstTerm(static_cast<const symir::ForStmt *>(stmt));
    } break;
    case symir::SymIR::SIR_STMT_WHILE: {
      return whileHasConstTerm(static_cast<const symir::WhileStmt *>(stmt));
    } break;
    default: Panic("Unsupported stmt");
    }
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

} // namespace

symir::BlockBuilder::StmtID StmtExprReplacer::Copy() {
  this->hasReplaced = true;
  this->targetStmt->Accept(*this);
  this->hasReplaced = false;
  return popStmt();
}

symir::BlockBuilder::StmtID StmtExprReplacer::CopyTerm(const symir::Term *t) {
  bool orig = this->hasReplaced;
  this->hasReplaced = true;
  t->Accept(*this);
  this->hasReplaced = orig;
  return popTerm();
}

symir::BlockBuilder::StmtID StmtExprReplacer::CopyExpr(const symir::Expr *e) {
  bool orig = this->hasReplaced;
  this->hasReplaced = true;
  e->Accept(*this);
  this->hasReplaced = orig;
  return popExpr();
}

symir::BlockBuilder::StmtID StmtExprReplacer::CopyCond(const symir::Cond *c) {
  bool orig = this->hasReplaced;
  this->hasReplaced = true;
  c->Accept(*this);
  this->hasReplaced = orig;
  return popCond();
}

symir::BlockBuilder::StmtID StmtExprReplacer::CopyWithReplacement(
  std::function<symir::BlockBuilder::ExprID(const symir::Expr *, symir::Coef **)> repFun
) {
  this->hasReplaced = false;
  this->repFun = repFun;
  symir::BlockBuilder::StmtID res = 0;
  // this is stupitly inefficent. ToDo find way to make this not stupit
  while (!this->hasReplaced) {
    this->targetStmt->Accept(*this);
    res = popStmt();
  }
  this->repFun = nullptr;
  return res;
}

void StmtExprReplacer::Visit(const symir::VarUse &v) {
  for (auto &c: v.GetAccess()) {
      c->Accept(*this);
  }
}

void StmtExprReplacer::Visit(const symir::Coef &c) {
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

void StmtExprReplacer::Visit(const symir::Term &t) {
  t.GetCoef()->Accept(*this);
  const symir::VarDef *var = nullptr;
  std::vector<symir::Coef *> access{};
  if (t.GetOp() != symir::Term::Op::OP_CST) {
    const auto name = t.GetVar()->GetName();
    var = t.GetVar()->GetDef();
    t.GetVar()->Accept(*this);
    for (size_t i = 0; i < t.GetVar()->GetAccess().size(); i++) {
      access.insert(access.begin(), popCoef());
    }
  }
  pushTerm(this->blockBd->SymTerm(t.GetOp(), popCoef(), var, access));
}

void StmtExprReplacer::Visit(const symir::ModExpr &e) {
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

void StmtExprReplacer::Visit(const symir::Expr &e) {
  const auto &terms = e.GetTerms();
  std::vector<TermID> termIds;
  for (const auto &t: terms) {
    t->Accept(*this);
    termIds.push_back(popTerm());
  }
  pushExpr(this->blockBd->SymExpr(e.GetOp(), termIds));
}

void StmtExprReplacer::Visit(const symir::Cond &c) {
  ExprID exprId;
  if (canReplExpr(c.GetExpr(), this->condReplProba)) {
    exprId = applyReplFun(c.GetExpr());
    this->hasReplaced = true;
  } else {
    c.GetExpr()->Accept(*this);
    exprId = popExpr();
  }
  pushCond(this->blockBd->SymCond(c.GetOp(), exprId));
}

void StmtExprReplacer::Visit(const symir::ModAssStmt &a) {
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

void StmtExprReplacer::Visit(const symir::AssStmt &a) {
  const auto use = a.GetVar();
  const auto name = use->GetName();
  const auto var = use->GetDef();
  Assert(var != nullptr, "Variable \"%s\" does not exist", name.c_str());

  use->Accept(*this);
  std::vector<symir::Coef *> access{};
  for (size_t i = 0; i < use->GetAccess().size(); i++) {
    access.insert(access.begin(), popCoef());
  }

  ExprID exprId;
  if (canReplExpr(a.GetExpr(), this->assStmtReplProba)) {
    exprId = applyReplFun(a.GetExpr());
    this->hasReplaced = true;
  } else {
    a.GetExpr()->Accept(*this);
    exprId = popExpr();
  }

  pushStmt(this->blockBd->SymAssStmt(var, exprId, access));
}

void StmtExprReplacer::Visit(const symir::IfStmt &i) {
  const auto conds = i.getConds();
  const auto bodies = i.getBodies();

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

void StmtExprReplacer::Visit(const symir::ForStmt &f) {
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

  ExprID initID;
  if (canReplExpr(init, this->forInitReplProba)) {
    initID = applyReplFun(init);
    this->hasReplaced = true;
  } else {
    init->Accept(*this);
    initID = popExpr();
  }

  ExprID incrementID;
  if (canReplExpr(increment, this->forIncrReplProba)) {
    incrementID = applyReplFun(increment);
    this->hasReplaced = true;
  } else {
    increment->Accept(*this);
    incrementID = popExpr();
  }

  std::vector<StmtID> sids;
  sids.resize(body.size());
  for (size_t i = 0; i < body.size(); i++) {
    Assert(body[i]->GetIRId() != symir::SymIR::SIR_TGT_BRA || body[i]->GetIRId() != symir::SymIR::SIR_TGT_GOTO, "ForStmt contains Goto or Branch");
    body[i]->Accept(*this);
    sids[i] = popStmt();
  }

  pushStmt(this->blockBd->SymForStmt(use->GetDef(), cid, initID, incrementID, sids, access));
}

void StmtExprReplacer::Visit(const symir::WhileStmt &w) {
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

void RewriteEngine::addRule(std::unique_ptr<Rule> rule, int weight) {
  rules.push_back(std::move(rule));
  weights.push_back(weight);
}

void RewriteEngine::run(symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd, size_t times) const {
  int totalWeight = 0;
  for (int weight : this->weights) totalWeight += weight;
  auto rand = Random::Get().Uniform(0, totalWeight);
  Log::Get().OpenSection("RewriteEngine::run() for " + blockBd->GetLabel());
  Log::Get().Out() << "Running randomized RewriteEngine " << times 
                   << " times with " << this->rules.size() 
                   << " passes and total weight: " << totalWeight << std::endl;

  for (size_t t = 0; t < times; t++) {
    int selWeight = rand();
    int index = 0;
    while (selWeight > this->weights[index]) selWeight -= this->weights[index++];
    auto rule = this->rules[index].get();

    this->applyRuleOnBlock(rule, funBd, blockBd);
  }
  Log::Get().CloseSection();
}

void RewriteEngine::run(symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd, std::vector<int> indices) const {
  Log::Get().OpenSection("RewriteEngine::run() for " + blockBd->GetLabel());
  Log::Get().Out() << "Running indexed RewriteEngine with " << this->rules.size() << " passes on indices: ";
  for (int index : indices) {
    Log::Get().Out() << index << " ";
  }
  Log::Get().Out() << std::endl;
  

  auto randDouble = Random::Get().UniformReal();

  for (int index : indices) {
    auto rule = this->rules[index].get();
    this->applyRuleOnBlock(rule, funBd, blockBd);
  }
  Log::Get().CloseSection();
}

void RewriteEngine::applyRuleOnBlock(Rule *rule, symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd) const {
  auto randDouble = Random::Get().UniformReal();
  size_t nrStmts = blockBd->GetNumberCommitedStmt();
  for (size_t i = 0; i < nrStmts; i++) {
    const auto *stmt = blockBd->GetCommitedStmt(i);
    stmt = blockBd->SymReplaceCommitStmt({ this->applyRuleForSubStmt(rule, funBd, blockBd, stmt) }, i)[0];
    if (rule->match(stmt) && rule->applyProbability(stmt) >= randDouble()) {
      std::vector<symir::BlockBuilder::StmtID> newStmts = rule->rewrite(funBd, blockBd, stmt);
      size_t blockSizeIncrease = newStmts.size() - 1;
      blockBd->SymReplaceCommitStmt(newStmts, i);
      i += blockSizeIncrease;
      nrStmts += blockSizeIncrease;
    }
  }
}

symir::BlockBuilder::StmtID RewriteEngine::applyRuleForSubStmt(
  Rule *rule,
  symir::FunctBuilder *funBd,
  symir::BlockBuilder *blockBd,
  const symir::Stmt *stmt
) const {
  switch (stmt->GetIRId()) {
  case symir::SymIR::SIR_STMT_FOR:
    return this->applyRuleOnFor(rule, funBd, blockBd, static_cast<const symir::ForStmt *>(stmt));
  case symir::SymIR::SIR_STMT_WHILE:
    return this->applyRuleOnWhile(rule, funBd, blockBd, static_cast<const symir::WhileStmt *>(stmt));
  case symir::SymIR::SIR_STMT_IF:
    return this->applyRuleOnIf(rule, funBd, blockBd, static_cast<const symir::IfStmt *>(stmt));
  default: return StmtExprReplacer(funBd, blockBd, stmt).Copy();
  }
}

symir::BlockBuilder::StmtID RewriteEngine::applyRuleOnFor(
  Rule *rule,
  symir::FunctBuilder *funBd,
  symir::BlockBuilder *blockBd,
  const symir::ForStmt *forStmt
) const {
  auto randDouble = Random::Get().UniformReal();
  auto body = forStmt->GetBody();
  std::vector<symir::BlockBuilder::StmtID> newBody;
  newBody.reserve(body.size());
  size_t nrStmts = body.size();
  for (size_t i = 0; i < nrStmts; i++) {
    const auto *stmt = body[i];
    stmt = blockBd->GetUncommitedStmt(this->applyRuleForSubStmt(rule, funBd, blockBd, stmt));
    if (rule->match(stmt) && rule->applyProbability(stmt) >= randDouble()) {
      std::vector<symir::BlockBuilder::StmtID> newStmts = rule->rewrite(funBd, blockBd, stmt);
      for (auto newStmt : newStmts) newBody.push_back(newStmt);
    } else {
      newBody.push_back(StmtExprReplacer(funBd, blockBd, stmt).Copy());
    }
  }

  auto copier = StmtExprReplacer(funBd, blockBd, nullptr);
  return blockBd->SymForStmt(
    forStmt->GetVar()->GetDef(),
    copier.CopyCond(forStmt->GetCond()),
    copier.CopyExpr(forStmt->GetInit()),
    copier.CopyExpr(forStmt->GetIncrement()),
    newBody
  );
}

symir::BlockBuilder::StmtID RewriteEngine::applyRuleOnWhile(
  Rule *rule,
  symir::FunctBuilder *funBd,
  symir::BlockBuilder *blockBd,
  const symir::WhileStmt *whileStmt
) const {
  auto randDouble = Random::Get().UniformReal();
  auto body = whileStmt->GetBody();
  std::vector<symir::BlockBuilder::StmtID> newBody;
  newBody.reserve(body.size());
  size_t nrStmts = body.size();
  for (size_t i = 0; i < nrStmts; i++) {
    const auto *stmt = body[i];
    stmt = blockBd->GetUncommitedStmt(this->applyRuleForSubStmt(rule, funBd, blockBd, stmt));
    if (rule->match(stmt) && rule->applyProbability(stmt) >= randDouble()) {
      std::vector<symir::BlockBuilder::StmtID> newStmts = rule->rewrite(funBd, blockBd, stmt);
      for (auto newStmt : newStmts) newBody.push_back(newStmt);
    } else {
      newBody.push_back(StmtExprReplacer(funBd, blockBd, stmt).Copy());
    }
  }

  auto copier = StmtExprReplacer(funBd, blockBd, nullptr);
  return blockBd->SymWhileStmt(copier.CopyCond(whileStmt->GetCond()), newBody);
}

symir::BlockBuilder::StmtID RewriteEngine::applyRuleOnIf(
  Rule *rule,
  symir::FunctBuilder *funBd,
  symir::BlockBuilder *blockBd,
  const symir::IfStmt *ifStmt
) const {
  auto randDouble = Random::Get().UniformReal();
  auto bodies = ifStmt->getBodies();
  std::vector<std::vector<symir::BlockBuilder::StmtID>> newBodies;
  newBodies.resize(bodies.size());
  for (size_t j = 0; j < bodies.size(); j++) {
    auto body = bodies[j];
    newBodies[j].reserve(bodies[j].size());
    size_t nrStmts = body.size();
    for (size_t i = 0; i < nrStmts; i++) {
      const auto *stmt = body[i];
      stmt = blockBd->GetUncommitedStmt(this->applyRuleForSubStmt(rule, funBd, blockBd, stmt));
      if (rule->match(stmt) && rule->applyProbability(stmt) >= randDouble()) {
        std::vector<symir::BlockBuilder::StmtID> newStmts = rule->rewrite(funBd, blockBd, stmt);
        for (auto newStmt : newStmts) newBodies[j].push_back(newStmt);
      } else {
        newBodies[j].push_back(StmtExprReplacer(funBd, blockBd, stmt).Copy());
      }
    }
  }

  auto copier = StmtExprReplacer(funBd, blockBd, nullptr);
  std::vector<symir::BlockBuilder::CondID> cids;
  auto conds = ifStmt->getConds();
  cids.reserve(conds.size());
  for (auto cond : conds) {
    cids.push_back(copier.CopyCond(cond));
  }
  return blockBd->SymIfStmt(cids, newBodies);
}

bool VariableInjection::match(const symir::Stmt *stmt) const {
  return stmtHasConstTerm(stmt);
}

double VariableInjection::applyProbability(const symir::Stmt *stmt) const {
  switch (stmt->GetIRId()) {
  case symir::SymIR::SIR_STMT_ASS: return 0.05;
  case symir::SymIR::SIR_STMT_FOR: return 0.8;
  case symir::SymIR::SIR_STMT_IF: return 0.8;
  case symir::SymIR::SIR_STMT_WHILE: return 0.8;
  default: Panic("No other stmts should appear here");
  }
}

std::vector<symir::BlockBuilder::StmtID> VariableInjection::rewrite(
  symir::FunctBuilder *funBd,
  symir::BlockBuilder *blockBd,
  const symir::Stmt *stmt
) {
  Log::Get().Out() << "Running VariableInjection" << std::endl;

  StmtExprReplacer rep = StmtExprReplacer(funBd, blockBd, stmt);
  const symir::VarDef *var = this->getNewLocal(funBd);
  std::function<symir::BlockBuilder::ExprID(const symir::Expr *, symir::Coef **)> varInsertFun =
    [&](const symir::Expr *e, symir::Coef **c) {
      const auto &terms = e->GetTerms();
      std::vector<symir::BlockBuilder::TermID> termIds;
      bool repConst = false;
      for (const auto &t: terms) {
        if (!repConst && t->GetOp() == symir::Term::OP_CST) {
          *c = t->GetCoef();
          termIds.push_back(blockBd->SymMulTerm(funBd->SymI32Const(1), var));
          repConst = true;
        } else {
          termIds.push_back(rep.CopyTerm(t));
        }
      }
      return blockBd->SymExpr(e->GetOp(), termIds);
    };

  symir::BlockBuilder::StmtID newStmt = rep.CopyWithReplacement(varInsertFun);

  Log::Get().Out() << "Replacing Const " << rep.getReplacedCoef()->GetI32Value() << " with " << var->GetName() << std::endl;

  symir::BlockBuilder::StmtID assignStmts = blockBd->SymAssStmt(
    var,
    blockBd->SymExpr(
      symir::Expr::OP_ADD,
      { blockBd->SymCstTerm(rep.getReplacedCoef(), nullptr) }
    )
  );

  return {assignStmts, newStmt};
}


bool ConstToAdd::match(const symir::Stmt *stmt) const {
  if (stmt->GetIRId() != symir::SymIR::SIR_STMT_ASS) return false;
  return stmtHasConstTerm(stmt);
}

double ConstToAdd::applyProbability(const symir::Stmt *stmt) const {
  return 0.7;
}

std::vector<symir::BlockBuilder::StmtID> ConstToAdd::rewrite(
  symir::FunctBuilder *funBd,
  symir::BlockBuilder *blockBd,
  const symir::Stmt *stmt
) {
  Log::Get().Out() << "Running ConstToAdd" << std::endl;

  const symir::AssStmt *assStmt = static_cast<const symir::AssStmt *>(stmt);
  const auto use = assStmt->GetVar();
  const auto def = use->GetDef();
  const auto expr = assStmt->GetExpr();

  auto access = copyAccess(funBd, use);

  std::vector<symir::BlockBuilder::TermID> termIds;
  termIds.reserve(expr->GetTerms().size() + 1);
  bool has_transformed = false;
  for (const auto &term : expr->GetTerms()) {
    if (!has_transformed && term->GetOp() == symir::Term::OP_CST && term->GetCoef()->IsSolved()) {
      has_transformed = true;
      int target = term->GetCoef()->GetI32Value();
      int v1, v2;
      if (target > 0) {
        v1 = Random::Get().Uniform(0, target)();
        v2 = target - v1;
      } else if (target < 0){
        v1 = Random::Get().Uniform(INT_MIN, target)();
        v2 = target - v1;
      } else {
        // if target == 0 we can choose any positive int and build (x - x)
        v1 = Random::Get().Uniform(0, INT_MAX)();
        v2 = -v1;
      }
      if (expr->GetOp() == symir::Expr::OP_SUB) v2 = -v2;

      Log::Get().Out() << "Replacing Const " << target << " with "
                       << v1 << " " << expr->GetOpSym(expr->GetOp()) << " " << v2 << std::endl;

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
      termIds.push_back(StmtExprReplacer(funBd, blockBd, nullptr).CopyTerm(term));
    }
  }

  return { blockBd->SymAssStmt(def, blockBd->SymExpr(expr->GetOp(), termIds), access) };
}

bool ConstToForSum::match(const symir::Stmt *stmt) const {
  if (stmt->GetIRId() != symir::SymIR::SIR_STMT_ASS) return false;
  const symir::AssStmt *assStmt = dynamic_cast<const symir::AssStmt *>(stmt);
  Assert(assStmt != nullptr, "Cast is checked and should not fail");
  const auto expr = assStmt->GetExpr();
  auto terms = expr->GetTerms();
  if (terms.size() != 1) return false;
  if (terms[0]->GetOp() != symir::Term::OP_CST) return false;
  return true;
}

double ConstToForSum::applyProbability(const symir::Stmt *stmt) const {
  return 0.35;
}

std::vector<symir::BlockBuilder::StmtID> ConstToForSum::rewrite(
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

bool AssToDeadCode::match(const symir::Stmt *stmt) const {
  if (stmt->GetIRId() != symir::SymIR::SIR_STMT_ASS) return false;
  return true;
}

double AssToDeadCode::applyProbability(const symir::Stmt *stmt) const {
  return 0.25;
}

std::vector<symir::BlockBuilder::StmtID> AssToDeadCode::rewrite(
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

  symir::BlockBuilder::ExprID exprId = StmtExprReplacer(funBd, blockBd, nullptr).CopyExpr(expr);

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
  const auto conds = i.getConds();
  const auto bodies = i.getBodies();

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
  const auto conds = i.getConds();
  const auto bodies = i.getBodies();

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

