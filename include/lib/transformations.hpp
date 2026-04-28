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

#ifndef REIFY_UBBASE_HPP
#define REIFY_UBBASE_HPP

#include "lib/lang.hpp"
#include "lib/random.hpp"
#include <memory>
#include <stack>

/// ==================== StmtExprReplacer Definition ====================
/// Copies Stmt with the options of modifying a random expression inside the Stmt
/// Also has helpers to copy Exprs and Terms
class StmtExprReplacer : protected symir::SymIRVisitor, symir::SymIRCopier<symir::BlockBuilder::StmtID, void> {
public:
  explicit StmtExprReplacer(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::Stmt *stmt,
    double assStmtReplProba = 0.2,
    double condReplProba = 0.5,
    double forInitReplProba = 0.5,
    double forIncrReplProba = 0.5
  ) : funBd(funBd), blockBd(blockBd), targetStmt(stmt),
      assStmtReplProba(assStmtReplProba),
      condReplProba(condReplProba),
      forInitReplProba(forInitReplProba),
      forIncrReplProba(forIncrReplProba),
      randFun(Random::Get().UniformReal())
	{}

  StmtID Copy() override;
  TermID CopyTerm(const symir::Term *t);
  TermID CopyExpr(const symir::Expr* e);
  StmtID CopyWithReplacement(std::function<symir::BlockBuilder::ExprID(const symir::Expr *, symir::Coef **)> repFun);
  void CopyAsBuilder() override { Panic("Stmt has no builder class"); }
  symir::Coef *getReplacedCoef() { return replacedCoef; }
  bool hasConst(const symir::Expr *e) {
    bool res = false;
    for (auto &t : e->GetTerms()) res |= t->GetOp() == symir::Term::OP_CST;
    return res;
  }
  bool canReplExpr(const symir::Expr *e, double proba) {
    return !this->hasReplaced && hasConst(e) && this->randFun() <= proba;
  }
  ExprID applyReplFun(const symir::Expr *e) {
    return this->repFun(e, &this->replacedCoef);
  }

protected:
  void Visit(const symir::VarUse &v) override;
  void Visit(const symir::Coef &c) override;
  void Visit(const symir::Term &t) override;
  void Visit(const symir::Expr &e) override;
  void Visit(const symir::ModExpr &e) override;
  void Visit(const symir::Cond &c) override;
  void Visit(const symir::AssStmt &a) override;
  void Visit(const symir::ModAssStmt &a) override;
  void Visit(const symir::IfStmt &i) override;
  void Visit(const symir::ForStmt &f) override;
  void Visit(const symir::WhileStmt &w) override;
  void Visit(const symir::RetStmt &r) override { Panic("Not a valid rewrite target"); }
  void Visit(const symir::Branch &b) override { Panic("Not a valid rewrite target"); }
  void Visit(const symir::Goto &g) override { Panic("Not a valid rewrite target"); }
  void Visit(const symir::ScaParam &p) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::VecParam &p) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::StructParam &p) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::UnInitLocal &l) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::ScaLocal &l) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::VecLocal &l) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::StructLocal &l) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::StructDef &s) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::Block &b) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::Funct &f) override { Panic("Not a subnode of any STMT"); }

private:
  symir::FunctBuilder *funBd;
  symir::BlockBuilder *blockBd;
  const symir::Stmt *targetStmt;
  std::function<symir::BlockBuilder::ExprID(const symir::Expr *, symir::Coef **)> repFun = nullptr;

  double assStmtReplProba;
  double condReplProba;
  double forInitReplProba;
  double forIncrReplProba;
  std::function<double()> randFun;

  bool hasReplaced = false;

  symir::Coef *replacedCoef;

  std::stack<symir::Coef *> coefStack{};
  std::stack<symir::BlockBuilder::TermID> termStack{};
  std::stack<symir::BlockBuilder::ExprID> exprStack{};
  std::stack<symir::BlockBuilder::ExprID> modExprStack{};
  std::stack<symir::BlockBuilder::CondID> condStack{};


};

/// ==================== Virtual Rule Definition ====================
struct Rule {
  Rule(std::string locPrefix = "tmp") : locPrefix(locPrefix) {}
  virtual bool match(const symir::Stmt *stmt) const = 0;
  virtual size_t rewrite(symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd, size_t stmtIdx) = 0;

  const symir::VarDef *getNewLocal(symir::FunctBuilder *funBd) {
    std::string locName = this->locPrefix + std::to_string(this->unique_counter++);
    const symir::VarDef *loc = funBd->FindVar(locName);
    if (loc == nullptr) loc = funBd->SymUnInitLocal(locName);
    return loc;
  }

protected:
  std::string locPrefix;
  size_t unique_counter = 0;
};

/// ==================== Rewrite Engine Definition ====================
struct RewriteEngine {
  void addRule(std::unique_ptr<Rule> rule, int weight);
  /// normal run method used for random selection of passes based on the passed weight, runs `times` passes in total
  void run(symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd, size_t times) const;
  /// run method for debuging runs passes based in the index given by the `indices` vector
  void run(symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd, std::vector<int> indices) const;
protected:

  std::vector<std::unique_ptr<Rule>> rules;
  std::vector<int> weights;
};

/// ==================== Various Rule Definition ====================
struct VariableInjection : Rule {
  VariableInjection() : Rule("vj") {}
  bool match(const symir::Stmt *stmt) const override;
  size_t rewrite(symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd, size_t stmtIdx) override;
};

struct ConstToAdd : Rule {
  bool match(const symir::Stmt *stmt) const override;
  size_t rewrite(symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd, size_t stmtIdx) override;
};

struct ConstToForSum: Rule {
  ConstToForSum() : Rule("i") {}
  bool match(const symir::Stmt *stmt) const override;
  size_t rewrite(symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd, size_t stmtIdx) override;
};

struct ConstToDeadCode: Rule {
  ConstToDeadCode(int minBranches = 2, int maxBranches = 4, bool allowUB = false) :
    minBranches(minBranches), maxBranches(maxBranches), allowUB(allowUB) {
    Assert(minBranches >= 2, "ConstToDeadCode must have atleast 2 branches");
  }
  bool match(const symir::Stmt *stmt) const override;
  size_t rewrite(symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd, size_t stmtIdx) override;
private:
  int minBranches;
  int maxBranches;
  bool allowUB;
};

/// ==================== Classes to embed variables ====================
/// Can Query for constants that can be replaced
struct ConstQuery : symir::SymIRVisitor {
  ConstQuery(symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd) : funBd(funBd), blockBd(blockBd) {}
  std::vector<const symir::Term *> query();
protected:
  void Visit(const symir::VarUse &v) override;
  void Visit(const symir::Coef &c) override { return; }
  void Visit(const symir::Term &t) override;
  void Visit(const symir::Expr &e) override;
  void Visit(const symir::ModExpr &e) override { return; }
  void Visit(const symir::Cond &c) override;
  void Visit(const symir::AssStmt &a) override;
  void Visit(const symir::ModAssStmt &a) override { return; };
  void Visit(const symir::IfStmt &i) override;
  void Visit(const symir::ForStmt &f) override;
  void Visit(const symir::WhileStmt &w) override;
  void Visit(const symir::RetStmt &r) override { Panic("Not a valid embed target"); }
  void Visit(const symir::Branch &b) override { Panic("Not a valid embed target"); }
  void Visit(const symir::Goto &g) override { Panic("Not a valid embed target"); }
  void Visit(const symir::ScaParam &p) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::VecParam &p) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::StructParam &p) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::UnInitLocal &l) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::ScaLocal &l) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::VecLocal &l) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::StructLocal &l) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::StructDef &s) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::Block &b) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::Funct &f) override { Panic("Not a subnode of any STMT"); }
private:
  symir::FunctBuilder *funBd;
  symir::BlockBuilder *blockBd;
  std::vector<const symir::Term *> terms;
};

struct VariableEmbedder: symir::SymIRVisitor, private symir::SymIRCopier<symir::BlockBuilder::StmtID, void> {
  VariableEmbedder(symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd) : funBd(funBd), blockBd(blockBd) {}
  void embed(std::map<const symir::Term *, symir::BlockBuilder::TermID> varMap);


protected:
  void Visit(const symir::VarUse &v) override;
  void Visit(const symir::Coef &c) override;
  void Visit(const symir::Term &t) override;
  void Visit(const symir::Expr &e) override;
  void Visit(const symir::ModExpr &e) override;
  void Visit(const symir::Cond &c) override;
  void Visit(const symir::AssStmt &a) override;
  void Visit(const symir::ModAssStmt &a) override;
  void Visit(const symir::IfStmt &i) override;
  void Visit(const symir::ForStmt &f) override;
  void Visit(const symir::WhileStmt &w) override;
  void Visit(const symir::RetStmt &r) override { Panic("Not a valid embed target"); }
  void Visit(const symir::Branch &b) override { Panic("Not a valid embed target"); }
  void Visit(const symir::Goto &g) override { Panic("Not a valid embed target"); }
  void Visit(const symir::ScaParam &p) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::VecParam &p) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::StructParam &p) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::UnInitLocal &l) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::ScaLocal &l) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::VecLocal &l) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::StructLocal &l) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::StructDef &s) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::Block &b) override { Panic("Not a subnode of any STMT"); }
  void Visit(const symir::Funct &f) override { Panic("Not a subnode of any STMT"); }
private:
  StmtID Copy() override { Panic("Not intended to be used"); };
  void CopyAsBuilder() override { Panic("Not intended to be used"); }
  symir::FunctBuilder *funBd;
  symir::BlockBuilder *blockBd;
  std::map<const symir::Term *, symir::BlockBuilder::TermID> varMap;

};

#endif //REIFY_UBBASE_HPP
