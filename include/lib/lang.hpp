// MIT License
//
// Copyright (c) 2025-2025
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

#ifndef GRAPHFUZZ_LANG_HPP
#define GRAPHFUZZ_LANG_HPP

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "lang.hpp"
#include "lib/dbgutils.hpp"

namespace symir {

  ///////////////////////////////////////////////////////////////////////
  // The SymIR Tiny Language
  ///////////////////////////////////////////////////////////////////////

  class Var;
  class Coef;
  class Term;
  class Expr;
  class Cond;
  class AssStmt;
  class RetStmt;
  class Branch;
  class Goto;
  class Block;
  class Func;

  class SymIRVisitor {
  public:
    virtual ~SymIRVisitor() = default;
    virtual void Visit(const Var &v) = 0;
    virtual void Visit(const Coef &c) = 0;
    virtual void Visit(const Term &t) = 0;
    virtual void Visit(const Expr &e) = 0;
    virtual void Visit(const Cond &c) = 0;
    virtual void Visit(const AssStmt &a) = 0;
    virtual void Visit(const RetStmt &r) = 0;
    virtual void Visit(const Branch &b) = 0;
    virtual void Visit(const Goto &g) = 0;
    virtual void Visit(const Block &b) = 0;
    virtual void Visit(const Func &f) = 0;
  };

  /**
   * SymIR is an simplified IR for modeling a C-like function.
   *
   * Context-Free Grammar:
   *
   * -----------------------------------------------
   * Func    -> 'func' Name Type (Type Var)+ Block+
   * Block   -> 'block' Label Stmt* Target?
   * ---
   * Target  -> Branch | Goto
   * Branch  -> 'branch' Cond Label Label
   * Goto    -> 'goto' Label
   * ---
   * Stmt    -> AssStmt | RetStmt
   * AssStmt -> 'assign' Var Expr
   * RetStmt -> 'return' Var+
   * ---
   * Cond    -> CondOp Expr
   * Expr    -> ExprOp Term Term+
   * Term    -> TermOp Coef Var
   * ---
   * Name    -> f1, f2, f3, ...
   * Label   -> BB1, BB2, BB3, ...
   * Type    -> int
   * Var     -> '1', v1, v2, v3, ... // '1' to make Term a bias
   * Coef    -> 0, 1, 2, ...
   * CondOp  -> >, <, ==             // Remove other ops as they are identical
   * ExprOp  -> +, -                 // Avoid all other ops to reduce SMT solver's stress
   * TermOp  -> +, -, *, /, %        // Avoid bit-wise ops as we are not using bv theories
   * -----------------------------------------------
   *
   * Example:
   *
   * -----------------------------------------------
   * (func f (int (int v1) (int v2) (int v3))
   *   (BB3
   * 	   (assign v1 (eadd
   * 	   	(add 12 v1)
   * 	   	(sub 23 v2)
   * 	   	(mul 132 1)
   * 	   ))
   * 	   (assign v1 (eadd
   * 	   	(add 12 v1)
   * 	   	(mul 23 v2)
   * 	   ))
   * 	   (branch (geq (eadd
   * 	   	(add 12 v1)
   * 	   	(sub 23 v2)
   * 	   	(mul 132 1)
   * 	   )) 2 BB4)
   * 	   (goto BB3)
   *   )
   *   ...
   * )
   * -----------------------------------------------
   */
  class SymIR {
  public:
    using ID = int;

    enum {
      SIR_COEF,
      SIR_VAR,
      SIR_TERM,
      SIR_EXPR,
      SIR_COND,
      SIR_STMT_ASS,
      SIR_STMT_RET,
      SIR_TGT_BRA,
      SIR_TGT_GOTO,
      SIR_BLK,
      SIR_FUNC,
    };

#define SYMIR_TYPE_LIST(XX) XX(I32, "int", "i32")

    enum Type {

#define XX(val, cname, sname) val,
      SYMIR_TYPE_LIST(XX)
#undef XX
    };

    static std::string GetTypeName(Type type) {
      static const char *names[] = {
#define XX(val, cname, sname) #val,
          SYMIR_TYPE_LIST(XX)
#undef XX
      };
      return names[type];
    }

    static std::string GetTypeSName(Type type) {
      static const char *names[] = {
#define XX(val, cname, sname) sname,
          SYMIR_TYPE_LIST(XX)
#undef XX
      };
      return names[type];
    }

    static std::string GetTypeCName(Type type) {
      static const char *names[] = {
#define XX(val, cname, sname) cname,
          SYMIR_TYPE_LIST(XX)
#undef XX
      };
      return names[type];
    }

    explicit SymIR(ID irId) : irId(irId) {}

    virtual ~SymIR() = default;

    virtual void Accept(SymIRVisitor &v) const = 0;

    [[nodiscard]] ID GetIRId() const { return irId; }

  private:
    ID irId;
  };

  /// The type annotator for SymIR nodes
  class WithType {
  public:
    explicit WithType(const SymIR::Type type = SymIR::Type::I32) : type(type) {}

    [[nodiscard]] SymIR::Type GetType() const { return type; }

  protected:
    void setType(const SymIR::Type type) { this->type = type; }

  protected:
    SymIR::Type type;
  };

  /// A Var represents a variable
  class Var : public SymIR, public WithType {
  public:
    Var(std::string name, const Type type) :
        SymIR(SIR_VAR), WithType(type), name(std::move(name)) {}

    [[nodiscard]] const std::string &GetName() const { return name; }

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  private:
    std::string name;
  };

  /// A Coef represents an coefficient for a variable
  class Coef : public SymIR, public WithType {
  public:
    Coef(std::string name, const Type type) :
        SymIR(SIR_COEF), WithType(type), name(std::move(name)), value(std::nullopt) {}

    Coef(std::string name, const std::string &value, const Type type) :
        SymIR(SIR_COEF), WithType(type), name(std::move(name)), value(value) {}

    [[nodiscard]] const std::string &GetName() const { return name; }

    [[nodiscard]] const std::optional<std::string> &GetValueOrNull() const { return value; }

    [[nodiscard]] const std::string &GetValue() const {
      Assert(IsValueSet(), "Value is not set yet");
      return value.value();
    }

    [[nodiscard]] bool IsValueSet() const { return value.has_value(); }

    void SetValue(const std::string &v) { value = v; }

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  private:
    std::string name;
    std::optional<std::string> value;
  };

  /// A Term is an operation of a Coef and a Var
  class Term : public SymIR, public WithType {
  public:
#define SYMIR_TERMOP_LIST(XX)                                                                      \
  XX(ADD, Add, add, +)                                                                             \
  XX(SUB, Sub, sub, -)                                                                             \
  XX(MUL, Mul, mul, *)                                                                             \
  XX(DIV, Div, div, /)                                                                             \
  XX(MOD, Mod, mod, %)

    enum Op {

#define XX(val, capt, smal, sym) OP_##val,
      SYMIR_TERMOP_LIST(XX)
#undef XX
    };

    static std::string GetOpName(Op op) {
      static const char *names[] = {
#define XX(val, capt, smal, sym) #val,
          SYMIR_TERMOP_LIST(XX)
#undef XX
      };
      return names[op];
    }

    static std::string GetOpShort(Op op) {
      static const char *names[] = {
#define XX(val, capt, smal, sym) #smal,
          SYMIR_TERMOP_LIST(XX)
#undef XX
      };
      return names[op];
    }

    static std::string GetOpSym(Op op) {
      static const char *symbols[] = {
#define XX(val, capt, smal, sym) #sym,
          SYMIR_TERMOP_LIST(XX)
#undef XX
      };
      return symbols[op];
    }

    Term(const Op op, std::unique_ptr<Coef> coef, const Var *var) :
        SymIR(SIR_TERM), op(op), coef(std::move(coef)), var(var) {
      Assert(this->var != nullptr, "Nullptr is given for the variable");
      Assert(this->coef->GetType() == this->var->GetType(), "Different types are given");
      setType(this->var->GetType());
    }

    [[nodiscard]] Op GetOp() const { return op; }

    [[nodiscard]] const Coef *GetCoef() const { return coef.get(); }

    [[nodiscard]] const Var *GetVar() const { return var; }

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  private:
    Op op;
    std::unique_ptr<Coef> coef;
    const Var *var;
  };

  /// An Expr represents a cumulative computation of a series of terms
  class Expr : public SymIR, public WithType {
  public:
#define SYMIR_EXPROP_LIST(XX)                                                                      \
  XX(ADD, Add, add, +)                                                                             \
  XX(SUB, Sub, sub, -)

    enum Op {

#define XX(val, capt, smal, sym) OP_##val,
      SYMIR_EXPROP_LIST(XX)
#undef XX
    };

    static std::string GetOpName(Op op) {
      static const char *names[] = {
#define XX(val, capt, smal, sym) #val,
          SYMIR_EXPROP_LIST(XX)
#undef XX
      };
      return names[op];
    }

    static std::string GetOpShort(Op op) {
      static const char *names[] = {
#define XX(val, capt, smal, sym) #smal,
          SYMIR_EXPROP_LIST(XX)
#undef XX
      };
      return names[op];
    }

    static std::string GetOpSym(Op op) {
      static const char *symbols[] = {
#define XX(val, capt, smal, sym) #sym,
          SYMIR_EXPROP_LIST(XX)
#undef XX
      };
      return symbols[op];
    }

    Expr(const Op op, std::vector<std::unique_ptr<Term>> terms) :
        SymIR(SIR_EXPR), op(op), terms(std::move(terms)) {
      Assert(this->terms.size() > 0, "Emtpy terms are given");
      const auto type_ = this->terms[0]->GetType();
      for (const auto &t: this->terms) {
        Assert(t->GetType() == type_, "Different types are given");
      }
      setType(type_);
    }

    [[nodiscard]] Op GetOp() const { return op; }

    [[nodiscard]] size_t GetNumTerms() const { return terms.size(); }

    [[nodiscard]] std::vector<const Term *> GetTerms() const {
      std::vector<const Term *> r;
      for (const auto &t: terms) {
        r.push_back(t.get());
      }
      return r;
    }

    [[nodiscard]] const Term *GetTerm(size_t i) const {
      Assert(i < terms.size(), "Index out of bounds");
      return terms[i].get();
    }

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  private:
    Op op;
    std::vector<std::unique_ptr<Term>> terms;
  };

  /// A Cond represents a conditional over of an expression and 0
  class Cond : public SymIR, public WithType {
  public:
#define SYMIR_CONDOP_LIST(XX)                                                                      \
  XX(GTZ, Gtz, gtz, >)                                                                             \
  XX(LTZ, Ltz, ltz, <)                                                                             \
  XX(EQZ, Eqz, eqz, ==)

    enum Op {

#define XX(val, capt, smal, sym) OP_##val,
      SYMIR_CONDOP_LIST(XX)
#undef XX
    };

    static std::string GetOpName(Op op) {
      static const char *names[] = {
#define XX(val, capt, smal, sym) #val,
          SYMIR_CONDOP_LIST(XX)
#undef XX
      };
      return names[op];
    }

    static std::string GetOpShort(Op op) {
      static const char *names[] = {
#define XX(val, capt, smal, sym) #smal,
          SYMIR_CONDOP_LIST(XX)
#undef XX
      };
      return names[op];
    }

    static std::string GetOpSym(Op op) {
      static const char *symbols[] = {
#define XX(val, capt, smal, sym) #sym,
          SYMIR_CONDOP_LIST(XX)
#undef XX
      };
      return symbols[op];
    }

    Cond(const Op op, std::unique_ptr<Expr> expr) :
        SymIR(SIR_COND), WithType(expr->GetType()), op(op), expr(std::move(expr)) {}

    [[nodiscard]] Op GetOp() const { return op; }

    [[nodiscard]] const Expr *GetExpr() const { return expr.get(); }

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  private:
    Op op;
    std::unique_ptr<Expr> expr;
  };

  class Stmt : public SymIR {
  public:
    explicit Stmt(SymIR::ID irId) : SymIR(irId) {}
  };

  /// An AssStmt represents the assignment of an expression to a variable
  class AssStmt : public Stmt {
  public:
    AssStmt(const Var *var, std::unique_ptr<Expr> expr) :
        Stmt(SIR_STMT_ASS), var(var), expr(std::move(expr)) {
      Assert(this->var->GetType() == this->expr->GetType(), "Different types are given");
    }

    [[nodiscard]] const Var *GetVar() const { return var; }

    [[nodiscard]] const Expr *GetExpr() const { return expr.get(); }

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  private:
    const Var *var;
    std::unique_ptr<Expr> expr;
  };

  /// A RetStmt represents a return of a series of variables.
  class RetStmt : public Stmt {
  public:
    explicit RetStmt(const std::vector<const Var *> vars) : Stmt(SIR_STMT_RET), vars(vars) {}

    [[nodiscard]] size_t GetNumVars() const { return vars.size(); }

    [[nodiscard]] const Var *GetVar(size_t i) const {
      Assert(i < vars.size(), "Index out of bounds");
      return vars[i];
    }

    [[nodiscard]] const std::vector<const Var *> &GetVars() const { return vars; }

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  private:
    std::vector<const Var *> vars;
  };

  class Target : public Stmt {
  public:
    explicit Target(ID irId) : Stmt(irId) {}

    [[nodiscard]] virtual size_t GetNumSuccessors() const = 0;
  };

  /// A Branch represents jumping to which targets are controlled by a condition.
  class Branch : public Target {
  public:
    Branch(std::unique_ptr<Cond> cond, std::string truLab, std::string falLab) :
        Target(SIR_TGT_BRA), cond(std::move(cond)), truLab(std::move(truLab)),
        falLab(std::move(falLab)) {}

    [[nodiscard]] const Cond *GetCond() const { return cond.get(); }

    [[nodiscard]] const std::string &GetTrueTarget() const { return truLab; }

    [[nodiscard]] const std::string &GetFalseTarget() const { return falLab; }

    [[nodiscard]] size_t GetNumSuccessors() const override { return 2; };

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  private:
    std::unique_ptr<Cond> cond;
    std::string truLab;
    std::string falLab;
  };

  /// A Goto represents an unconditional jump.
  class Goto : public Target {
  public:
    explicit Goto(std::string label) : Target(SIR_TGT_GOTO), label(std::move(label)) {}

    [[nodiscard]] const std::string &GetTarget() const { return label; }

    [[nodiscard]] size_t GetNumSuccessors() const override { return 1; };

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  private:
    std::string label;
  };

  /// A Block represents a basic block of a series of statements and a target
  class Block : public SymIR {
  public:
    Block(
        // clang-format off
      std::string label,
      std::vector<std::unique_ptr<Stmt>> stmts,
      std::unique_ptr<Target> target
        // clang-format on
    ) :
        SymIR(SIR_BLK), label(std::move(label)), stmts(std::move(stmts)),
        target(std::move(target)) {}

    [[nodiscard]] std::string GetLabel() const { return label; }

    [[nodiscard]] const Target *GetTarget() const { return target.get(); }

    [[nodiscard]] size_t GetNumSuccessors() const {
      if (!target.get()) {
        return 0;
      } else {
        return target->GetNumSuccessors();
      }
    }

    [[nodiscard]] size_t GetNumStmts() const { return stmts.size() + (target.get() ? 0 : 1); }

    [[nodiscard]] Stmt *GetStmt(size_t i) const {
      Assert(i < GetNumStmts(), "Index out of bound");
      return i < stmts.size() ? stmts[i].get() : target.get();
    }

    [[nodiscard]] std::vector<const Stmt *> GetStmts() const {
      std::vector<const Stmt *> r;
      for (const auto &s: stmts) {
        r.push_back(s.get());
      }
      if (target.get()) {
        r.push_back(target.get());
      }
      return r;
    }

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  private:
    std::string label;
    std::vector<std::unique_ptr<Stmt>> stmts;
    std::unique_ptr<Target> target;
  };

  /// A Func is a series of basic blocks
  class Func : public SymIR, public WithType {
  public:
    Func(
        // clang-format off
      std::string name,
      Type retType,
      std::vector<std::unique_ptr<Var>> params,
      std::vector<std::unique_ptr<Block>> blocks
        // clang-format on
    ) :
        SymIR(SIR_FUNC), WithType(retType), name(std::move(name)), params(std::move(params)),
        blocks(std::move(blocks)) {
      Assert(this->params.size() > 0, "No params are given");
      Assert(this->blocks.size() > 0, "No blocks are given");
      for (auto i = 0; i < this->blocks.size(); i++) {
        const auto &blk = this->blocks[i];
        const auto &blkLabel = blk->GetLabel();
        Assert(!blockMap.contains(blkLabel), "Label conflicts with blocks: %s", blkLabel.c_str());
        blockMap[blkLabel] = blk.get();
      }
      for (auto i = 0; i < this->params.size(); i++) {
        const auto &prm = this->params[i];
        const auto &prmName = prm->GetName();
        Assert(!paramMap.contains(prmName), "Name conflicts with variables: %s", prmName.c_str());
        paramMap[prmName] = prm.get();
      }
    }

    [[nodiscard]] Type GetRetType() const { return GetType(); }

    [[nodiscard]] const std::string &GetName() const { return name; }

    [[nodiscard]] std::vector<const Var *> GetParams() const {
      std::vector<const Var *> r;
      for (const auto &v: params) {
        r.push_back(v.get());
      }
      return r;
    }

    [[nodiscard]] const Var *GetParam(const std::string &name) const {
      if (paramMap.contains(name)) {
        return paramMap.find(name)->second;
      } else {
        return nullptr;
      }
    }

    [[nodiscard]] std::vector<const Block *> GetBlocks() const {
      std::vector<const Block *> r;
      for (const auto &b: blocks) {
        r.push_back(b.get());
      }
      return r;
    }

    [[nodiscard]] const Block *GetBlock(const std::string &label) const {
      if (blockMap.contains(label)) {
        return blockMap.find(label)->second;
      } else {
        return nullptr;
      }
    }

    [[nodiscard]] std::vector<const Stmt *> GetStmts() const {
      std::vector<const Stmt *> ret;
      for (auto i = 0; i < blocks.size(); ++i) {
        auto sr = blocks[i]->GetStmts();
        ret.insert(ret.end(), sr.begin(), sr.end());
      }
      return ret;
    }

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  private:
    std::string name;
    std::vector<std::unique_ptr<Var>> params;
    std::vector<std::unique_ptr<Block>> blocks;
    std::map<std::string, const Block *> blockMap;
    std::map<std::string, const Var *> paramMap;
  };

  ///////////////////////////////////////////////////////////////////////
  // SymIR Program Builders
  ///////////////////////////////////////////////////////////////////////

  template<typename ParentBuilder, typename SIR>
  class SymIRBuilder {
  public:
    explicit SymIRBuilder(const ParentBuilder *ctx) : ctx(ctx) {}

    virtual ~SymIRBuilder() = default;

    virtual std::unique_ptr<SIR> Build() = 0;

  protected:
    const ParentBuilder *GetParent() const { return ctx; }

    [[nodiscard]] bool isActive() const { return active; }

    void deactivate() { active = false; }

  private:
    // Our residing builder
    const ParentBuilder *ctx;
    // Whether the builder is active or not
    bool active = true;
  };

  /// The top-level builder for all other builders
  class RootBuilder : SymIRBuilder<void, void> {
    explicit RootBuilder() : SymIRBuilder(nullptr) {}
  };

  class FuncBuilder;

  /// Builder to facilitate building a basic block
  ///
  /// Example:
  ///
  /// -----------------------------------------------------------
  ///   auto b = std::make_unique<BlockBuilder>("BB1")
  ///   b->SymAssign(
  ///     v0, b->SymAddExpr({
  ///       b->SymTerm(Term::OP_MUL, "12", v1),
  ///       b->SymTerm(Term::OP_MUL, "23", v1)
  ///     })
  ///   );
  ///   b->SymBranch(
  ///     "BB1", "BB2",
  ///     b->SymGtzCond(
  ///       b->SymSubExpr({
  ///         b->SymTerm(Term::OP_MUL, "123", v2),
  ///         b->SymTerm(Term::OP_MUL, "33", v1)
  ///       })
  ///     )
  ///   );  // We cannot call anything any more after Branch
  /// ----------------------------------------------------------
  class BlockBuilder final : public SymIRBuilder<FuncBuilder, Block> {
  public:
    using TermID = size_t;
    using ExprID = size_t;
    using CondID = size_t;

  public:
    using BlockBody = std::function<void(BlockBuilder *)>;

    BlockBuilder(const FuncBuilder *ctx, std::string label) :
        SymIRBuilder<FuncBuilder, Block>(ctx), label(std::move(label)), target(nullptr) {}

    /// Create a Term and return the ID to use it. The term can be used only once.
    TermID SymTerm(Term::Op op, const std::string &coef, const Var *var);
#define XX(val, capt, ...)                                                                         \
  TermID Sym##capt##Term(const std::string &coef, const Var *var) {                                \
    return SymTerm(Term::OP_##val, coef, var);                                                     \
  }
    SYMIR_TERMOP_LIST(XX)
#undef XX

    /// Create an Expr and return the ID to use it. The expr can be used only once.
    ExprID SymExpr(Expr::Op op, const std::vector<TermID> &terms);
#define XX(val, capt, ...)                                                                         \
  ExprID Sym##capt##Expr(const std::vector<TermID> &termIds) {                                     \
    return SymExpr(Expr::OP_##val, termIds);                                                       \
  }
    SYMIR_EXPROP_LIST(XX)
#undef XX

    /// Create a Cond and return the ID to use it. The cond can be used only once.
    CondID SymCond(Cond::Op, ExprID eid);
#define XX(val, capt, ...)                                                                         \
  CondID Sym##capt##Cond(ExprID eid) { return SymCond(Cond::OP_##val, eid); }
    SYMIR_CONDOP_LIST(XX)
#undef XX

    /// Create and commit an AssStmt to the builder..
    void SymAssign(const Var *var, ExprID eid);

    /// Create and commit a RetStmt to the builder.
    void SymReturn();

    /// Create and commit a Branch target to the builder.
    /// After calling this function, the ::Build() should be called to commit the block
    /// and the builder cannot be used any more to create more SIRs.
    void SymBranch(const std::string &truLab, const std::string &falLab, CondID cid);

    /// Create and commit a Goto target to the builder.
    /// After calling this function, the ::Build() should be called to commit the block
    /// and the builder cannot be used any more to create more SIRs.
    void SymGoto(const std::string &label);

    /// Indicate that the basic block being built has no target blocks.
    /// After calling this function, the ::Build() should be called to commit the block
    /// and the builder cannot be used any more to create more SIRs.
    void SymFallThro();

    /// Build and commit the basic block.
    std::unique_ptr<Block> Build() override;

  private:
    // Ingredients for the basic block being built
    std::string label;
    std::unique_ptr<Target> target;
    std::vector<std::unique_ptr<Stmt>> stmts{};

    // Management of temporary objects created by users
    TermID numCreatedTerms = 0;
    ExprID numCreatedExprs = 0;
    CondID numCreatedConds = 0;
    std::map<TermID, std::unique_ptr<Term>> createdTerms{};
    std::map<ExprID, std::unique_ptr<Expr>> createdExprs{};
    std::map<CondID, std::unique_ptr<Cond>> createdConds{};
  };

  /// Builder to facilitate building a function
  ///
  /// Example:
  ///
  /// -----------------------------------------------------------
  ///   auto b = std::make_unique<FuncBuilder>("f0", SymIR::Type::I32)
  ///   auto v0 = b->SymVar("v0);
  ///   auto bb1 = b.SymBlock("BB1", [=](BlockBuilder *blk) {
  ///     blk->SymBranch(
  ///       "BB1", "BB2",
  ///       blk->SymGtzCond(
  ///         blk->SymSubExpr({
  ///           blk->SymTerm(Term::OP_MUL, "123", v2),
  ///           blk->SymTerm(Term::OP_MUL, "33", v1)
  ///         })
  ///       )
  ///     );
  ///   });  // We cannot call anything any more after Branch
  ///   auto f0 = b.Build();
  /// ----------------------------------------------------------
  class FuncBuilder final : SymIRBuilder<RootBuilder, Func> {

  public:
    explicit FuncBuilder(std::string name, SymIR::Type retType = SymIR::I32) :
        SymIRBuilder<RootBuilder, Func>(nullptr), name(std::move(name)), retType(retType) {}

    /// Get all defined parameters
    [[nodiscard]] std::vector<const Var *> GetParams() const {
      std::vector<const Var *> r;
      for (auto i = 0; i < params.size(); i++) {
        r.push_back(params[i].get());
      }
      return r;
    }

    /// Define and commit a new Var
    const Var *SymVar(const std::string &name, SymIR::Type type = SymIR::I32);

    /// Define and commit a new basic block
    const Block *SymBlock(const std::string &label, const BlockBuilder::BlockBody &body);

    /// Find a defined Var
    [[nodiscard]] const Var *FindVar(const std::string &name) const;

    /// Find a defined basic block
    [[nodiscard]] const Block *FindBlock(const std::string &label) const;

    /// Build the function.
    /// After calling this function, the builder is no longer usable.
    std::unique_ptr<Func> Build() override;

  private:
    // Ingredients for building a function
    std::string name;
    SymIR::Type retType;
    std::vector<std::unique_ptr<Var>> params{};
    std::vector<std::unique_ptr<Block>> blocks{};

    // For ease of managing and manipulating
    std::map<std::string, const Block *> blockMap{};
    std::map<std::string, const Var *> paramMap{};
  };
} // namespace symir

#endif // GRAPHFUZZ_LANG_HPP
