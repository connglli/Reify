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

#include "lib/Transformations/primitive.hpp"
#include "lib/Transformations/instcombine.hpp"
#include "lib/Transformations/vectorize.hpp"
#include "lib/lang.hpp"

using namespace transformations;
/// ==================== Rewrite Engine Definition ====================
class RewriteEngine {
public:
  /// Get empty RewriteEngine without any rules added
  static RewriteEngine Empty() { return RewriteEngine(); }
  /// Get default RewriteEngine with the default set of rules added
  static RewriteEngine Default() { 
    auto engine = RewriteEngine();
    engine.addRule(std::make_unique<primitive::SimpleConstProbagation>(), 3);
    engine.addRule(std::make_unique<primitive::AdditionFromConst>(), 4);
    engine.addRule(std::make_unique<primitive::ForSumFromConst>(), 4);
    engine.addRule(std::make_unique<primitive::DeadCodeFromAssign>(), 1);
    engine.addRule(std::make_unique<vectorize::DeadAssignFromCopy>(), 1);
    return engine;
  }
  void addRule(std::unique_ptr<Rule> rule, int weight);
  /// normal run method used for random selection of rules based on the passed weight, attempts to runs `times` rules in total
  void run(symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd, size_t times) const;

private:
  RewriteEngine() = default;

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

// TODO ConstQuery and ConstEmbedder probably fit better in another file then here

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
