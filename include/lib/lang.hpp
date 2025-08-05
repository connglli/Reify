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
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "lib/dbgutils.hpp"

namespace symir {

  ///////////////////////////////////////////////////////////////////////
  // The SymIR Tiny Language
  ///////////////////////////////////////////////////////////////////////

  class Coef;
  class VarUse;
  class Term;
  class Expr;
  class Cond;
  class AssStmt;
  class RetStmt;
  class Branch;
  class Goto;
  class Param;
  class Local;
  class Block;
  class Funct;

  class SymIRVisitor {
  public:
    virtual ~SymIRVisitor() = default;
    virtual void Visit(const VarUse &v) = 0;
    virtual void Visit(const Coef &c) = 0;
    virtual void Visit(const Term &t) = 0;
    virtual void Visit(const Expr &e) = 0;
    virtual void Visit(const Cond &c) = 0;
    virtual void Visit(const AssStmt &a) = 0;
    virtual void Visit(const RetStmt &r) = 0;
    virtual void Visit(const Branch &b) = 0;
    virtual void Visit(const Goto &g) = 0;
    virtual void Visit(const Param &p) = 0;
    virtual void Visit(const Local &l) = 0;
    virtual void Visit(const Block &b) = 0;
    virtual void Visit(const Funct &f) = 0;
  };

  /**
   * SymIR is a simplified, symbolic IR for modeling a C-like function.
   * The main goal of SymIR is to generate code that are easier to model
   * UBs (undefined behavior) and easier for symbolic execution. The
   * symbols in SymIR including for example, coefficient. Additionally,
   * we want to ease the pressure on SMT sovlers, so we intentionally
   * restrict the expressiveness of expressions to only a few operations
   * on terms (an atomic expression of coefficients and variables),
   * rather than use all operations that are available in C/C++ and apply
   * them to terms and also recursively to expressions themselves.
   * Otherwise, the STM solver face non-linear variable calculations (for
   * example "var_a x var_b", "var_a ^ var_b") and bit-wise operations,
   * which are not needed for our purpose.
   *
   * Context-Free Grammar:
   *
   * -----------------------------------------------
   * Funct   -> 'function' Name Type (Type Var)+ Local* Block+
   * Local   -> 'local' Var Coef Type
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
   * Expr    -> ExprOp Term Term+   // We intentionally avoid "Expr -> ExprOp Expr Expr"
   * Term    -> TermOp Coef Var
   * ---
   * Name    -> f1, f2, f3, ...
   * Label   -> BB1, BB2, BB3, ...
   * Type    -> int
   * Var     -> v1, v2, v3, ...
   * Coef    -> c1, c2, 0, 1, 2, ...
   * CondOp  -> >, <, ==             // Remove other ops as they are identical
   * ExprOp  -> +, -                 // Avoid all other ops to reduce SMT solver's stress
   * TermOp  -> +, -, *, /, %        // Avoid bit-wise ops as we are not using bv theories
   * -----------------------------------------------
   *
   * Example:
   *
   * -----------------------------------------------
   * (fun f0 i32 ((par v0 i32) (par v1 i32))
   *   (loc v2 #100 i32)
   *   (loc v3 c2 i32)
   *   (bbl BB1
   *     (asn v0 (eadd (mul #12 v1) (mul c4 v1)))
   *     (brh BB1 BB2 (gtz (eadd (mul c5 v2) (mul #33 v1))))
   *   )
   *   (bbl BB2
   *     (asn v3 (eadd (mul #9 v1) (mul c8 v2)))
   *     (ret)
   *   )
   * )
   * -----------------------------------------------
   *
   * TODOs:
   *
   * 1. Add support for on-stack arrays and structs.
   * 2. Add support for heap and pointer arithemics.
   */
  class SymIR {
  public:
    using ID = int;

    enum {
      SIR_COEF,
      SIR_VAR_USE,
      SIR_TERM,
      SIR_EXPR,
      SIR_COND,
      SIR_STMT_ASS,
      SIR_STMT_RET,
      SIR_TGT_BRA,
      SIR_TGT_GOTO,
      SIR_PARAM,
      SIR_LOCAL,
      SIR_BLOCK,
      SIR_FUNCT,
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

    static Type GetTypeFromSName(const std::string &name) {
      static std::map<std::string, Type> map = {
#define XX(val, cname, sname) {sname, val},
          SYMIR_TYPE_LIST(XX)
#undef XX
      };
      return map[name];
    }

    explicit SymIR(ID irId) : irId(irId) {}

    virtual ~SymIR() = default;

    virtual void Accept(SymIRVisitor &v) const = 0;

    [[nodiscard]] ID GetIRId() const { return irId; }

  private:
    ID irId;
  };

  /// WithType is an annotator for SymIR nodes to mark nodes with type
  class WithType {
  public:
    explicit WithType(const SymIR::Type type = SymIR::Type::I32) : type(type) {}

    virtual ~WithType() = default;

    [[nodiscard]] SymIR::Type GetType() const { return type; }

  protected:
    void setType(const SymIR::Type type) { this->type = type; }

  protected:
    SymIR::Type type;
  };

  /// VarDef is an annotator for SymIR nodes which defines a variable
  class VarDef : public WithType {
  public:
    VarDef(std::string name, const SymIR::Type type) : WithType(type), name(std::move(name)) {}

    [[nodiscard]] const std::string &GetName() const { return name; }

  protected:
    std::string name;
  };

  /// SymDef is a definition of a symbolic value, either been solved or not yet.
  class SymDef : public WithType {
  public:
    explicit SymDef(std::string name, const SymIR::Type type = SymIR::I32) :
        WithType(type), name(std::move(name)), value(std::nullopt) {}

    SymDef(std::string name, std::string value, const SymIR::Type type = SymIR::I32) :
        WithType(type), name(std::move(name)), value(std::move(value)) {}

    [[nodiscard]] const std::string &GetName() const { return name; }

    [[nodiscard]] const std::optional<std::string> &GetValueOrNull() const { return value; }

    [[nodiscard]] const std::string &GetValue() const {
      Assert(IsSolved(), "This coefficient is not yet solved");
      return value.value();
    }

    template<typename T>
    [[nodiscard]] T GetTypedValue() const {
      Assert(IsSolved(), "This coefficient is not yet solved");
      const auto type = GetType();
      const auto *typeName = SymIR::GetTypeName(type).c_str();
      switch (type) {
        case SymIR::I32:
          Assert(
              typeid(T) == typeid(int),
              "The coefficient \"%s\" is an %s, cannot get its typed value as %s", name.c_str(),
              typeName, typeid(T).name()
          );
          return GetI32Value();
        default:
          Panic(
              "Unsupported type for coefficient \"%s\": %s, cannot get its typed value",
              name.c_str(), typeName
          );
          break;
      }
    }

    [[nodiscard]] int GetI32Value() const {
      Assert(IsSolved(), "This coefficient is not yet solved");
      const auto type = GetType();
      const auto typeName = SymIR::GetTypeName(type);
      Assert(
          type == SymIR::I32,
          "The coefficient \"%s\" (type=%s) is not an %s, cannot get its value as an %s",
          name.c_str(), typeName.c_str(), SymIR::GetTypeName(SymIR::I32).c_str(), "int"
      );
      try {
        return std::stoi(value.value());
      } catch (const std::invalid_argument &e) {
        Panic(
            "Failed to convert the value of the coefficient \"%s\" (type=%s) into an int: %s",
            name.c_str(), typeName.c_str(), e.what()
        );
      }
    }

    [[nodiscard]] bool IsValueSet() const { return value.has_value(); }

    [[nodiscard]] bool IsSolved() const { return IsValueSet(); }

    void SetValue(const std::string &v) { value = v; }

  protected:
    std::string name;
    std::optional<std::string> value;
  };

  /// The VarUse represent a usage of a defined variable
  class VarUse : public SymIR, public WithType {
  public:
    explicit VarUse(const VarDef *var) : SymIR(SIR_VAR_USE), var(var) {
      Assert(var != nullptr, "No var to use: a nullptr is given");
      setType(var->GetType());
    }

    [[nodiscard]] const VarDef *GetDef() const { return var; }

    [[nodiscard]] const std::string &GetName() const { return var->GetName(); }

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  protected:
    const VarDef *var;
  };

  /// A Coef represents an coefficient for updating a variable. Since we are a symbolic IR,
  /// the value of the coefficient can be not yet set which should be solved.
  class Coef : public SymIR, public SymDef {
  public:
    Coef(std::string name, const Type type) : SymIR(SIR_COEF), SymDef(std::move(name), type) {}

    Coef(std::string name, std::string value, const Type type) :
        SymIR(SIR_COEF), SymDef(std::move(name), std::move(value), type) {}

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }
  };

  /// A Term is an operation of a Coef and a Var
  class Term : public SymIR, public WithType {
  public:
#define SYMIR_TERMOP_LIST(XX)                                                                      \
  XX(CST, Cst, cst, @)                                                                             \
  XX(ADD, Add, add, +)                                                                             \
  XX(SUB, Sub, sub, -)                                                                             \
  XX(MUL, Mul, mul, *)                                                                             \
  XX(DIV, Div, div, /)                                                                             \
  XX(REM, Rem, rem, %)

    enum Op {

#define XX(val, capt, smal, sym) OP_##val,
      SYMIR_TERMOP_LIST(XX)
#undef XX
          NUM_OPS
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

    Term(const Op op, Coef *coef, std::unique_ptr<VarUse> var) :
        SymIR(SIR_TERM), op(op), coef(std::move(coef)), var(std::move(var)) {
      if (op == OP_CST) {
        Assert(this->var == nullptr, "CST can only be used without a variable");
      } else {
        Assert(this->var != nullptr, "No var to use: a nullptr is given for the variable");
        Assert(
            this->coef->GetType() == this->var->GetType(),
            "The coef (%s) and the var (%s) are of different types",
            GetTypeSName(this->coef->GetType()).c_str(), GetTypeSName(this->var->GetType()).c_str()
        );
      }
      setType(this->coef->GetType());
    }

    [[nodiscard]] Op GetOp() const { return op; }

    [[nodiscard]] Coef *GetCoef() const { return coef; }

    [[nodiscard]] const VarUse *GetVar() const { return var.get(); }

    [[nodiscard]] std::vector<const VarUse *> GetUses() const {
      return var == nullptr ? std::vector<const VarUse *>{}
                            : std::vector<const VarUse *>{var.get()};
    }

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  private:
    Op op;
    Coef *coef;
    std::unique_ptr<VarUse> var;
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
          NUM_OPS
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
      Assert(!this->terms.empty(), "The terms array should not be empty");
      const auto type_ = this->terms[0]->GetType();
      for (const auto &t: this->terms) {
        Assert(
            t->GetType() == type_,
            "At least one term is in different type: %s (the term) vs %s (others)",
            GetTypeSName(t->GetType()).c_str(), GetTypeSName(type_).c_str()
        );
      }
      setType(type_);
    }

    [[nodiscard]] Op GetOp() const { return op; }

    [[nodiscard]] size_t NumTerms() const { return terms.size(); }

    [[nodiscard]] std::vector<const Term *> GetTerms() const {
      std::vector<const Term *> r;
      for (const auto &t: terms) {
        r.push_back(t.get());
      }
      return r;
    }

    [[nodiscard]] const Term *GetTerm(size_t i) const {
      Assert(i < terms.size(), "The accessing index (%lu) is out of bound (%lu)", i, terms.size());
      return terms[i].get();
    }

    [[nodiscard]] std::vector<const VarUse *> GetUses() const {
      std::vector<const VarUse *> uses{};
      for (const auto &t: terms) {
        const auto u = t->GetUses();
        uses.insert(uses.end(), u.begin(), u.end());
      }
      return uses;
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
          NUM_OPS
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

    Cond(const Op op, std::unique_ptr<Expr> expr) : SymIR(SIR_COND), op(op), expr(std::move(expr)) {
      Assert(this->expr != nullptr, "The given expression is a nullptr");
      setType(this->expr->GetType());
    }

    [[nodiscard]] Op GetOp() const { return op; }

    [[nodiscard]] const Expr *GetExpr() const { return expr.get(); }

    [[nodiscard]] std::vector<const VarUse *> GetUses() const { return expr->GetUses(); }

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  private:
    Op op;
    std::unique_ptr<Expr> expr;
  };

  class Stmt : public SymIR {
  public:
    explicit Stmt(SymIR::ID irId) : SymIR(irId) {}

    /// Get the list of used variables it there exists in this statement or an empty list.
    [[nodiscard]] virtual std::vector<const VarUse *> GetUses() const = 0;

    /// Get the defined variable in this statement if there exists or nullptr.
    [[nodiscard]] virtual const VarDef *GetDefinition() const = 0;
  };

  /// An AssStmt represents the assignment of an expression to a variable
  class AssStmt : public Stmt {
  public:
    AssStmt(std::unique_ptr<VarUse> var, std::unique_ptr<Expr> expr) :
        Stmt(SIR_STMT_ASS), var(std::move(var)), expr(std::move(expr)) {
      Assert(
          this->expr->GetType() == this->var->GetType(),
          "The var (%s) and the expr (%s) are of different types",
          GetTypeSName(this->var->GetType()).c_str(), GetTypeSName(this->expr->GetType()).c_str()
      );
    }

    [[nodiscard]] const VarUse *GetVar() const { return var.get(); }

    [[nodiscard]] const Expr *GetExpr() const { return expr.get(); }

    [[nodiscard]] std::vector<const VarUse *> GetUses() const override { return expr->GetUses(); }

    [[nodiscard]] const VarDef *GetDefinition() const override { return var.get()->GetDef(); }

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  private:
    std::unique_ptr<VarUse> var;
    std::unique_ptr<Expr> expr;
  };

  /// A RetStmt represents a return of a series of variables.
  class RetStmt : public Stmt {
  public:
    explicit RetStmt(std::vector<std::unique_ptr<VarUse>> vars) :
        Stmt(SIR_STMT_RET), vars(std::move(vars)) {}

    [[nodiscard]] size_t NumVars() const { return vars.size(); }

    [[nodiscard]] const VarUse *GetVar(size_t i) const {
      Assert(i < vars.size(), "The accessing index (%lu) is out of bounds (%lu)", i, vars.size());
      return vars[i].get();
    }

    [[nodiscard]] std::vector<const VarUse *> GetVars() const {
      std::vector<const VarUse *> r;
      for (size_t i = 0; i < vars.size(); i++) {
        r.push_back(vars[i].get());
      }
      return r;
    }

    [[nodiscard]] std::vector<const VarUse *> GetUses() const override { return GetVars(); }

    [[nodiscard]] const VarDef *GetDefinition() const override { return nullptr; }

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  private:
    std::vector<std::unique_ptr<VarUse>> vars;
  };

  /// A Target indicates the target basic blocks of a basic block.
  class Target : public Stmt {
  public:
    explicit Target(ID irId) : Stmt(irId) {}

    [[nodiscard]] virtual size_t NumSuccessors() const = 0;

    [[nodiscard]] virtual bool HasTarget(const std::string &label) const = 0;

    virtual void ReplaceTarget(const std::string &from, const std::string &to) = 0;
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

    [[nodiscard]] size_t NumSuccessors() const override { return 2; };

    [[nodiscard]] virtual bool HasTarget(const std::string &label) const {
      return label == truLab || label == falLab;
    }

    [[nodiscard]] std::vector<const VarUse *> GetUses() const override { return cond->GetUses(); }

    [[nodiscard]] const VarDef *GetDefinition() const override { return nullptr; }

    virtual void ReplaceTarget(const std::string &from, const std::string &to) {
      if (from != truLab && from != falLab) {
        Assert(
            false, "The target label (%s) is not found in the branch targets (true=%s, false=%s)",
            from.c_str(), truLab.c_str(), falLab.c_str()
        );
      }
      if (from == truLab) {
        truLab = to;
      }
      if (from == falLab) {
        falLab = to;
      }
    }

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

    [[nodiscard]] size_t NumSuccessors() const override { return 1; };

    [[nodiscard]] virtual bool HasTarget(const std::string &label) const {
      return label == this->label;
    }

    [[nodiscard]] std::vector<const VarUse *> GetUses() const override { return {}; }

    [[nodiscard]] const VarDef *GetDefinition() const override { return nullptr; }

    virtual void ReplaceTarget(const std::string &from, const std::string &to) {
      if (from == label) {
        label = to;
      } else {
        Assert(
            false, "The target label (%s) is not found in the goto target (%s)", from.c_str(),
            label.c_str()
        );
      }
    }

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  private:
    std::string label;
  };

  /// A Param is a declaration of a function's parameter.
  class Param : public SymIR, public VarDef {
  public:
    explicit Param(std::string name, Type type = Type::I32) :
        SymIR(SIR_PARAM), VarDef(std::move(name), type) {}

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }
  };

  /// A Local is a declaration of a local variable with an initial value within the function.
  class Local : public Stmt, public VarDef {
  public:
    explicit Local(std::string name, Coef *init, Type type = Type::I32) :
        Stmt(SIR_LOCAL), VarDef(std::move(name), type), coef(std::move(init)) {
      Assert(this->coef != nullptr, "The coef is given a nullptr");
      Assert(
          type == this->coef->GetType(), "The coef (%s) and the var (%s) are of different types",
          GetTypeSName(this->coef->GetType()).c_str(), GetTypeSName(type).c_str()
      );
    }

    [[nodiscard]] Coef *GetCoef() const { return coef; }

    [[nodiscard]] std::vector<const VarUse *> GetUses() const override { return {}; }

    [[nodiscard]] const VarDef *GetDefinition() const override { return this; }

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  private:
    Coef *coef;
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
        SymIR(SIR_BLOCK), label(std::move(label)), stmts(std::move(stmts)),
        target(std::move(target)) {}

    [[nodiscard]] std::string GetLabel() const { return label; }

    [[nodiscard]] const Target *GetTarget() const { return target.get(); }

    [[nodiscard]] size_t NumSuccessors() const {
      if (target == nullptr) {
        return 0;
      } else {
        return target->NumSuccessors();
      }
    }

    [[nodiscard]] size_t NumStmts() const { return stmts.size() + (target.get() ? 0 : 1); }

    [[nodiscard]] Stmt *GetStmt(size_t i) const {
      Assert(i < NumStmts(), "The accessing index (%lu) is out of bound (%lu)", i, NumStmts());
      return i < stmts.size() ? stmts[i].get() : target.get();
    }

    [[nodiscard]] std::vector<const Stmt *> GetStmts() const {
      std::vector<const Stmt *> r;
      for (const auto &s: stmts) {
        r.push_back(s.get());
      }
      if (target != nullptr) {
        r.push_back(target.get());
      }
      return r;
    }

    /// Get the list of used variables in this block
    /// When removeDefs is true, the definitions of variables are removed from the list.
    [[nodiscard]] std::vector<const VarUse *> GetUses(bool removeDefs = true) const {
      return GetUses(stmts, removeDefs);
    }

    /// Get the list of defined variables in this block
    [[nodiscard]] std::vector<const VarDef *> GetDefinitions() const {
      return GetDefinitions(stmts);
    }

    /// Get the list of used variables in this block
    /// When removeDefs is true, the definitions of variables are removed from the list.
    [[nodiscard]] static std::vector<const VarUse *>
    GetUses(const std::vector<std::unique_ptr<Stmt>> &stmts, bool removeDefs = true) {
      std::vector<const VarDef *> defs;
      std::vector<const VarUse *> uses;
      for (const auto &s: stmts) {
        if (const auto stmtUses = s->GetUses(); !stmtUses.empty()) {
          for (const auto *u: stmtUses) {
            if (std::ranges::find(defs, u->GetDef()) == defs.end()) {
              // If the variable is not defined in this block, add it to the list
              uses.push_back(u);
            } else if (!removeDefs) {
              // If we do not remove definitions, add it to the list
              uses.push_back(u);
            }
          }
        }
        if (const auto *d = s->GetDefinition(); d != nullptr) {
          defs.push_back(d);
        }
      }
      return uses;
    }

    /// Get the list of defined variables in this block
    [[nodiscard]] static std::vector<const VarDef *>
    GetDefinitions(const std::vector<std::unique_ptr<Stmt>> &stmts) {
      std::vector<const VarDef *> defs;
      for (const auto &s: stmts) {
        if (const auto &d = s->GetDefinition(); d != nullptr) {
          defs.push_back(d);
        }
      }
      return defs;
    }

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  private:
    std::string label;
    std::vector<std::unique_ptr<Stmt>> stmts;
    std::unique_ptr<Target> target;
  };

  /// A Funct is a series of basic blocks
  class Funct : public SymIR, public WithType {
  public:
    Funct(
        // clang-format off
        std::string name,
        Type retType,
        std::vector<std::unique_ptr<Param>> params,
        std::vector<std::unique_ptr<Local>> locals,
        std::vector<std::unique_ptr<SymDef>> symbols,
        std::vector<std::unique_ptr<Block>> blocks
        // clang-format on
    ) :
        SymIR(SIR_FUNCT), WithType(retType), name(std::move(name)), params(std::move(params)),
        locals(std::move(locals)), symbols(std::move(symbols)), blocks(std::move(blocks)) {
      Assert(!this->params.empty(), "No parameters are given");
      Assert(!this->blocks.empty(), "No basic blocks are given");
      for (size_t i = 0; i < this->blocks.size(); i++) {
        const auto &blk = this->blocks[i];
        const auto &blkLabel = blk->GetLabel();
        Assert(
            !blockMap.contains(blkLabel), "Blocks with the same label \"%s\" already exists",
            blkLabel.c_str()
        );
        blockMap[blkLabel] = blk.get();
      }
      for (size_t i = 0; i < this->params.size(); i++) {
        const auto &prm = this->params[i];
        const auto &prmName = prm->GetName();
        Assert(
            !paramMap.contains(prmName), "Parameters with the same name \"%s\" is already defined",
            prmName.c_str()
        );
        paramMap[prmName] = prm.get();
      }
      for (size_t i = 0; i < this->symbols.size(); i++) {
        const auto &sym = this->symbols[i];
        const auto &symName = sym->GetName();
        Assert(
            !symMap.contains(symName), "Symbols with the same name \"%s\" is already defined",
            symName.c_str()
        );
        symMap[symName] = sym.get();
      }
      for (size_t i = 0; i < this->locals.size(); i++) {
        const auto &loc = this->locals[i];
        const auto &locName = loc->GetName();
        Assert(
            !paramMap.contains(locName), "Parameters with the same name \"%s\" is already defined",
            locName.c_str()
        );
        Assert(
            !localMap.contains(locName), "Locals with the same name \"%s\" is already defined",
            locName.c_str()
        );
        localMap[locName] = loc.get();
      }
    }

    [[nodiscard]] Type GetRetType() const { return GetType(); }

    [[nodiscard]] const std::string &GetName() const { return name; }

    [[nodiscard]] int NumParams() const { return params.size(); }

    [[nodiscard]] std::vector<const Param *> GetParams() const {
      std::vector<const Param *> r;
      for (const auto &v: params) {
        r.push_back(v.get());
      }
      return r;
    }

    [[nodiscard]] const Param *GetParam(const std::string &name) const {
      if (paramMap.contains(name)) {
        return paramMap.find(name)->second;
      } else {
        return nullptr;
      }
    }

    [[nodiscard]] int NumSymbols() const { return symbols.size(); }

    [[nodiscard]] std::vector<SymDef *> GetSymbols() const {
      std::vector<SymDef *> r;
      for (const auto &v: symbols) {
        r.push_back(v.get());
      }
      return r;
    }

    [[nodiscard]] SymDef *GetSymbol(const std::string &name) {
      if (symMap.contains(name)) {
        return symMap.find(name)->second;
      } else {
        return nullptr;
      }
    }

    [[nodiscard]] int NumLocals() const { return localMap.size(); }

    [[nodiscard]] std::vector<const Local *> GetLocals() const {
      std::vector<const Local *> r;
      for (const auto &v: locals) {
        r.push_back(v.get());
      }
      return r;
    }

    [[nodiscard]] const VarDef *GetLocal(const std::string &name) const {
      if (localMap.contains(name)) {
        return localMap.find(name)->second;
      } else {
        return nullptr;
      }
    }

    [[nodiscard]] int NumBlocks() const { return blocks.size(); }

    [[nodiscard]] const VarDef *FindVar(const std::string &name) const {
      const VarDef *s = FindParam(name);
      if (s != nullptr) {
        return s;
      }
      return FindLocal(name);
    }

    [[nodiscard]] const Param *FindParam(const std::string &name) const {
      if (paramMap.contains(name)) {
        return paramMap.find(name)->second;
      } else {
        return nullptr;
      }
    }

    [[nodiscard]] SymDef *FindSymbol(const std::string &name) const {
      if (symMap.contains(name)) {
        return symMap.find(name)->second;
      } else {
        return nullptr;
      }
    }

    [[nodiscard]] const Local *FindLocal(const std::string &name) const {
      if (localMap.contains(name)) {
        return localMap.find(name)->second;
      } else {
        return nullptr;
      }
    }

    [[nodiscard]] const Block *FindBlock(const std::string &label) const {
      if (blockMap.contains(label)) {
        return blockMap.find(label)->second;
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
      for (size_t i = 0; i < locals.size(); ++i) {
        ret.push_back(locals[i].get());
      }
      for (size_t i = 0; i < blocks.size(); ++i) {
        auto sr = blocks[i]->GetStmts();
        ret.insert(ret.end(), sr.begin(), sr.end());
      }
      return ret;
    }

    void Accept(SymIRVisitor &v) const override { return v.Visit(*this); }

  private:
    std::string name;
    std::vector<std::unique_ptr<Param>> params;
    std::map<std::string, const Param *> paramMap;
    std::vector<std::unique_ptr<Local>> locals;
    std::map<std::string, const Local *> localMap;
    std::vector<std::unique_ptr<SymDef>> symbols;
    std::map<std::string, SymDef *> symMap;
    std::vector<std::unique_ptr<Block>> blocks;
    std::map<std::string, const Block *> blockMap;
  };

  ///////////////////////////////////////////////////////////////////////
  // SymIR Program Builders
  ///////////////////////////////////////////////////////////////////////

  class SymIRBuilder {
  public:
    using TermID = size_t;
    using ExprID = size_t;
    using CondID = size_t;

    virtual ~SymIRBuilder() = default;

  protected:
    [[nodiscard]] virtual bool isActive() const { return active; }

    virtual void deactivate() { active = false; }

  protected:
    // Whether the builder is active or not
    bool active = true;
  };

  /// The base SIR builder. Each have a parent builder.
  template<typename ParentBuilder, typename SIR>
  class SymIRBuilderGeneric : public SymIRBuilder {
  public:
    explicit SymIRBuilderGeneric(ParentBuilder *ctx) : ctx(ctx) {}

    ParentBuilder *GetParent() const { return ctx; }

    virtual std::unique_ptr<SIR> Build() = 0;

  private:
    // Our residing builder
    ParentBuilder *ctx;
  };

  /// The top-level builder for all other builders
  class RootBuilder : SymIRBuilderGeneric<void, void> {
    explicit RootBuilder() : SymIRBuilderGeneric(nullptr) {}
  };

  class FunctBuilder;

  /// Builder to facilitate building a basic block
  ///
  /// Example:
  ///
  /// -----------------------------------------------------------
  ///   auto b = std::make_unique<BlockBuilder>("BB1")
  ///   b->SymAssign(
  ///     v0, b->SymAddExpr({
  ///       b->SymMulTerm(b->GetParent()->SymCoef("c1", "12"), v1),
  ///       b->SymSubTerm(b->GetParent()->SymCoef("c2"), v1)
  ///     })
  ///   );
  ///   b->SymBranch(
  ///     "BB1", "BB2",
  ///     b->SymGtzCond(
  ///       b->SymSubExpr({
  ///         b->SymAddTerm(b->GetParent()->SymCoef("c3", "123"), v2),
  ///         b->SymAddTerm(b->GetParent()->SymCoef("c4"), v1)
  ///       })
  ///     )
  ///   );  // We cannot call anything any more after Branch
  ///   auto blk = b.Build();
  /// ----------------------------------------------------------
  class BlockBuilder final : public SymIRBuilderGeneric<FunctBuilder, Block> {
  public:
    using BlockBody = std::function<void(BlockBuilder *)>;

    BlockBuilder(FunctBuilder *ctx, std::string label) :
        SymIRBuilderGeneric<FunctBuilder, Block>(ctx), label(std::move(label)) {}

    /// Return the label of the basic block being built
    [[nodiscard]] const std::string &GetLabel() const { return label; }

    /// Create a Term with a coef and return the ID to use it. The term can be used only once.
    TermID SymTerm(Term::Op op, Coef *coef, const VarDef *var);
#define XX(val, capt, ...)                                                                         \
  TermID Sym##capt##Term(Coef *coef, const VarDef *var) {                                          \
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
    void SymAssign(const VarDef *var, ExprID eid);

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

    /// Get the list of used variables in this block
    [[nodiscard]] std::vector<const VarUse *> GetUses(bool removeDefs = true) const {
      return Block::GetUses(stmts, removeDefs);
    }

    /// Get the list of defined variables in this block
    [[nodiscard]] std::vector<const VarDef *> GetDefinitions() const {
      return Block::GetDefinitions(stmts);
    }

    /// Build and commit the basic block.
    std::unique_ptr<Block> Build() override;

  protected:
    [[nodiscard]] bool isActive() const {
      return target == nullptr && SymIRBuilderGeneric::isActive();
    }

  private:
    // Ingredients for the basic block being built
    std::string label;
    std::unique_ptr<Target> target = nullptr;
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
  ///   auto b = std::make_unique<FunctBuilder>("f0", SymIR::Type::I32)
  ///   auto v0 = b->SymParam("v0");
  ///   auto v1 = b->SymLocal("v1");
  ///   auto bb1 = b.SymBlock("BB1", [&](BlockBuilder *blk) {
  ///     blk->SymBranch(
  ///       "BB1", "BB2",
  ///       blk->SymGtzCond(
  ///         blk->SymSubExpr({
  ///           blk->SymMulTerm(b.SymCoef("c1", "123"), v0),
  ///           blk->SymAddTerm(b.SymCoef("c2"), v1)
  ///         })
  ///       )
  ///     );
  ///   });  // We cannot call anything any more after Branch
  ///   auto f0 = b.Build();
  /// ----------------------------------------------------------
  class FunctBuilder final : SymIRBuilderGeneric<RootBuilder, Funct> {

  public:
    explicit FunctBuilder(std::string name, SymIR::Type retType = SymIR::I32) :
        SymIRBuilderGeneric<RootBuilder, Funct>(nullptr), name(std::move(name)), retType(retType) {}

    /// Get all defined parameters
    [[nodiscard]] std::vector<const Param *> GetParams() const {
      std::vector<const Param *> r;
      for (size_t i = 0; i < params.size(); i++) {
        r.push_back(params[i].get());
      }
      return r;
    }

    /// Get all defined locals
    [[nodiscard]] std::vector<const Local *> GetLocals() const {
      std::vector<const Local *> r;
      for (size_t i = 0; i < locals.size(); i++) {
        r.push_back(locals[i].get());
      }
      return r;
    }

    /// Get all defined blocks
    [[nodiscard]] std::vector<const Block *> GetBlocks() const {
      std::vector<const Block *> r;
      for (size_t i = 0; i < blocks.size(); i++) {
        r.push_back(blocks[i].get());
      }
      return r;
    }

    /// Get all defined symbols
    [[nodiscard]] std::vector<SymDef *> GetSymbols() const {
      std::vector<SymDef *> r;
      for (size_t i = 0; i < symbols.size(); i++) {
        r.push_back(symbols[i].get());
      }
      return r;
    }

    /// Find a defined variable
    [[nodiscard]] const VarDef *FindVar(const std::string &name) const {
      const VarDef *s = FindParam(name);
      if (s != nullptr) {
        return s;
      }
      return FindLocal(name);
    }

    /// Find a defined parameter
    [[nodiscard]] const Param *FindParam(const std::string &name) const {
      if (paramMap.contains(name)) {
        return paramMap.find(name)->second;
      } else {
        return nullptr;
      }
    }

    /// Find a defined symbol
    [[nodiscard]] SymDef *FindSymbol(const std::string &name) const {
      if (symMap.contains(name)) {
        return symMap.find(name)->second;
      } else {
        return nullptr;
      }
    }

    /// Find a defined local
    [[nodiscard]] const Local *FindLocal(const std::string &name) const {
      if (localMap.contains(name)) {
        return localMap.find(name)->second;
      } else {
        return nullptr;
      }
    }

    /// Find a defined basic block
    [[nodiscard]] const Block *FindBlock(const std::string &label) const {
      if (blockMap.contains(label)) {
        return blockMap.find(label)->second;
      } else {
        return nullptr;
      }
    }

    /// Define and commit a new unsolved coefficient
    Coef *SymCoef(const std::string &name, SymIR::Type type = SymIR::I32);

    /// Define and commit a new solved coefficient
    Coef *SymCoef(const std::string &name, const std::string &value, SymIR::Type type = SymIR::I32);

    /// Define and commit a new Param
    const Param *SymParam(const std::string &name, SymIR::Type type = SymIR::I32);

    /// Define and commit a new Local
    const Local *SymLocal(const std::string &name, Coef *coef, SymIR::Type type = SymIR::I32);

    /// Define and commit a new basic block with defined body
    const Block *SymBlock(const std::string &label, const BlockBuilder::BlockBody &body);

    /// Open a basic block to define new statements
    BlockBuilder *OpenBlock(const std::string &label);

    /// Close an existing basic block and append it into the current end of function.
    const Block *CloseBlock(BlockBuilder *bbl);

    /// Close an existing basic block and insert it before the given block.
    const Block *CloseBlockAt(BlockBuilder *bbl, const Block *atBlk);

    /// Build the function.
    /// After calling this function, the builder is no longer usable.
    std::unique_ptr<Funct> Build() override;

  private:
    // Ingredients for building a function
    std::string name;
    SymIR::Type retType;
    std::vector<std::unique_ptr<SymDef>> symbols{};
    std::vector<std::unique_ptr<Param>> params{};
    std::vector<std::unique_ptr<Local>> locals{};
    std::vector<std::unique_ptr<Block>> blocks{};

    // For ease of managing and manipulating
    std::map<std::string, SymDef *> symMap{};
    std::map<std::string, const Param *> paramMap{};
    std::map<std::string, const Local *> localMap{};
    std::map<std::string, const Block *> blockMap{};

    // For managing created basic blocks
    std::map<std::string, std::unique_ptr<BlockBuilder>> createdBlocks{};
  };

  /// Utility to deep-copy a built function.
  class FunctCopier : protected SymIRVisitor, SymIRBuilder {
  public:
    // Hooks right before opening a new block. The input is the label of the new block.
    using BeforeBlockOpenHook = std::function<void(FunctBuilder *, const std::string &)>;
    // Hooks right after opening a new block. The input is the block builder of the new block.
    using AfterBlockOpenedHook = std::function<void(FunctBuilder *, BlockBuilder *)>;
    // Hooks right before closing a block. The input is the block builder of the block being closed.
    using BeforeBlockCloseHook = std::function<void(FunctBuilder *, BlockBuilder *)>;
    // Hooks right after closing a block. The input is the block being closed.
    using AfterBlockClosedHook = std::function<void(FunctBuilder *, const Block *)>;

  public:
    explicit FunctCopier(
        // clang-format off
        const Funct *src,
        BeforeBlockOpenHook beforeBlockOpenHook = nullptr,
        AfterBlockOpenedHook afterBlockOpenedHook = nullptr,
        BeforeBlockCloseHook beforeBlockCloseHook = nullptr,
        AfterBlockClosedHook afterBlockClosedHook = nullptr
        // clang-format on
    ) :
        src(src), beforeBlockOpenHook(std::move(beforeBlockOpenHook)),
        afterBlockOpenedHook(std::move(afterBlockOpenedHook)),
        beforeBlockCloseHook(std::move(beforeBlockCloseHook)),
        afterBlockClosedHook(std::move(afterBlockClosedHook)) {
      Assert(src != nullptr, "The source function is a nullptr");
    }

    /// Copy the function and return a new Funct object.
    std::unique_ptr<Funct> Copy();

    /// Copy the function and return a new FunctBuilder object.
    std::unique_ptr<FunctBuilder> CopyAsBuilder();

  protected:
    void Visit(const VarUse &v) override;
    void Visit(const Coef &c) override;
    void Visit(const Term &t) override;
    void Visit(const Expr &e) override;
    void Visit(const Cond &c) override;
    void Visit(const AssStmt &a) override;
    void Visit(const RetStmt &r) override;
    void Visit(const Branch &b) override;
    void Visit(const Goto &g) override;
    void Visit(const Param &p) override;
    void Visit(const Local &l) override;
    void Visit(const Block &b) override;
    void Visit(const Funct &f) override;

  private:
    void pushCoef(Coef *c) { coefStack.push(c); }

    Coef *popCoef() {
      Coef *c = coefStack.top();
      coefStack.pop();
      return c;
    }

    void pushTerm(TermID tid) { termStack.push(tid); }

    TermID popTerm() {
      TermID tid = termStack.top();
      termStack.pop();
      return tid;
    }

    void pushExpr(ExprID eid) { exprStack.push(eid); }

    ExprID popExpr() {
      ExprID eid = exprStack.top();
      exprStack.pop();
      return eid;
    }

    void pushCond(CondID cid) { condStack.push(cid); }

    CondID popCond() {
      CondID cid = condStack.top();
      condStack.pop();
      return cid;
    }

  private:
    // The function being copied/cloned
    const Funct *src;
    // The function builder to build the copied function
    std::unique_ptr<FunctBuilder> builder = nullptr;
    // The builder for the current block being built
    BlockBuilder *currentBlock = nullptr;
    // Hooks to change the block being built
    BeforeBlockOpenHook beforeBlockOpenHook = nullptr;
    AfterBlockOpenedHook afterBlockOpenedHook = nullptr;
    BeforeBlockCloseHook beforeBlockCloseHook = nullptr;
    AfterBlockClosedHook afterBlockClosedHook = nullptr;
    // Stacks to manage objects during copying
    std::stack<Coef *> coefStack{};
    std::stack<TermID> termStack{};
    std::stack<ExprID> exprStack{};
    std::stack<CondID> condStack{};
  };
} // namespace symir

#endif // GRAPHFUZZ_LANG_HPP
