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

#ifndef REIFY_TRANSFORMATIONS_HPP
#define REIFY_TRANSFORMATIONS_HPP

#include "lib/lang.hpp"
#include <memory>

// ==================== StmtReplacers Definition ====================

template<typename Node>
class StmtReplacer : public symir::StmtCopier {
public:
  StmtReplacer(
   symir::FunctBuilder *funBd,
   symir::BlockBuilder *blockBd
  ) : symir::StmtCopier(funBd, blockBd) {}
  /// Copies Stmt with while also replacing any Subexpression that matches 'matchFunction' with the return value of 'replaceFunction'
  StmtID CopyStmtWithReplacement(
    const symir::Stmt *s, 
    std::function<bool(const Node *)> matchFunction,
    std::function<size_t(symir::FunctBuilder *, symir::BlockBuilder *, const Node &, void **)> replaceFunction,
    double randThreshold = 1,
    size_t replaceMax = 1
  );

  void *getExtractedDataRef() { return this->data; }

protected:
  void Visit(const Node &e) override;
private:
  bool match(const Node &e) {
    return !this->hasReplaced && this->rand() <= this->randThreshold && this->matchFunction(&e);
  }
  ExprID replace(const Node &e) { 
    this->hasReplaced = true;
    return this->replaceFunction(funBd, blockBd, e, &this->data); 
  }
  double rand() { return this->randUniform(); }

private:
  std::function<bool(const Node *)> matchFunction;
  std::function<size_t(symir::FunctBuilder *, symir::BlockBuilder *, const Node &, void **)> replaceFunction;
  std::function<double()> randUniform;
  void *data = nullptr;
  double randThreshold = 1;
  bool hasReplaced = false;
};

/// ==================== Virtual Rule Definition ====================
struct Rule {
  Rule(std::string locPrefix = "local_created_by_rule_without_proper_locPrefix") : locPrefix(locPrefix) {}
  virtual ~Rule() = default;
  virtual bool match(const symir::Stmt *stmt) const = 0;
  virtual std::vector<symir::BlockBuilder::StmtID> rewrite(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::Stmt *stmt
  ) = 0;

  /// get a new local that is not yet used in the block with label `blockLabel`
  const symir::VarDef *getNewLocal(symir::FunctBuilder *funBd, std::string blockLabel) {
    std::string locName = this->locPrefix + "_" + std::to_string(varCounterMap[blockLabel]++);
    const symir::VarDef *loc = funBd->FindVar(locName);
    if (loc == nullptr) loc = funBd->SymUnInitLocal(locName);
    return loc;
  }

protected:
  std::string locPrefix;
  std::map<std::string, size_t> varCounterMap;
};

/// ==================== Rewrite Engine Definition ====================
class RewriteEngine {
public:
  void addRule(std::unique_ptr<Rule> rule, int weight);
  /// normal run method used for random selection of rules based on the passed weight, attempts to runs `times` rules in total
  void run(symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd, size_t times) const;

private:
  std::optional<symir::BlockBuilder::StmtID> runInSubStmt(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::Stmt *stmt
  ) const;
  std::optional<symir::BlockBuilder::StmtID> runInForStmt(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::ForStmt *forStmt
  ) const;
  std::optional<symir::BlockBuilder::StmtID> runInWhileStmt(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::WhileStmt *whileStmt
  ) const;
  std::optional<symir::BlockBuilder::StmtID> runInIfStmt(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::IfStmt *ifStmt
  ) const;
  std::optional<Rule *> getRandomMatchingRule(const symir::Stmt *stmt) const;
protected:

  std::vector<std::unique_ptr<Rule>> rules{};
  std::vector<int> weights{};
};

// ==================== Various Rule Definition ====================

// Notation:
// C1, C2, ... := constants/Literals
// E1, E2, ... := (Sub)Expression
// S1, S2, ... := statement (e.g. Assign, For, While or if)
// B1, B2, ... := Conditional Stmt
// {A, ..., Z, a, ..., z} Variables

// ======== Primitive creating rules ========

/// E1 + C1 + E2 => cpk = C1; E1 + cpk + E2
struct ConstProba : Rule {
  ConstProba() : Rule("const_proba") {}
  bool match(const symir::Stmt *stmt) const override;
  std::vector<symir::BlockBuilder::StmtID> rewrite(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::Stmt *stmt
  ) override;
};

/// E1 + C1 + E2 => E1 + C2 + C3 +E2
/// where C2 + C3 = C1
struct AdditionFromConst : Rule {
  bool match(const symir::Stmt *stmt) const override;
  std::vector<symir::BlockBuilder::StmtID> rewrite(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::Stmt *stmt
  ) override;
};

/// x = C1 => x = C2; for (i = 0; i < C3; i += 1) { x = C4 + x; }, 
/// where C3 * C4 + C2 = C1
struct ForSumFromConst : Rule {
  ForSumFromConst() : Rule("i") {}
  bool match(const symir::Stmt *stmt) const override;
  std::vector<symir::BlockBuilder::StmtID> rewrite(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::Stmt *stmt
  ) override;
};

/// x = E1 => if (B1) { x = E2 } else if (B2) { x = E3 } ... else { x = E`n` }
/// where exactly one or no B1 evaluates to true, if one does evaluate true the corresponding branch contains x = E1, if non are true then the else branch contains x = E1
struct DeadCodeFromAssign : Rule {
  DeadCodeFromAssign(int minBranches = 2, int maxBranches = 4, bool allowUB = false) :
    minBranches(minBranches), maxBranches(maxBranches), allowUB(allowUB) {
    Assert(minBranches >= 2, "AssToDeadCode must have atleast 2 branches");
  }
  bool match(const symir::Stmt *stmt) const override;
  std::vector<symir::BlockBuilder::StmtID> rewrite(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::Stmt *stmt
  ) override;

private:
  int minBranches;
  int maxBranches;
  bool allowUB;
};

struct VectorizerDeadAssignFromCopy : Rule {
  VectorizerDeadAssignFromCopy() : Rule("dead_assign") {}
  bool match(const symir::Stmt *stmt) const override;
  std::vector<symir::BlockBuilder::StmtID> rewrite(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::Stmt *stmt
  ) override;
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

#endif //REIFY_TRANSFORMATIONS_HPP
