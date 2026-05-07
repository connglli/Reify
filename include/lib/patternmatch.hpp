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

#ifndef REIFY_PATTERNMATCH_HPP
#define REIFY_PATTERNMATCH_HPP

#include "lib/lang.hpp"
#include <string>

namespace patternmatch {
  template<typename Node>
  struct Pattern {
    Pattern() {  }
    virtual ~Pattern() = default;
    virtual bool match(Node N) const = 0;
  private:
    // Avoid any derived pattern being assigned or copied
    Pattern(const Pattern& obj) {  }
    Pattern &operator=(const Pattern& tmp_obj) {  }
  };

  // ==================== Generic ====================

  template<typename Node>
  struct m_WildCard : Pattern<Node> {
    inline bool match(Node N) const override { return true; }
  };

  // ==================== Generic: Vectors ====================

  template<typename Node>
  struct m_Any : Pattern<std::vector<Node>> {
    m_Any(const Pattern<Node> &N) : N(N) {}
    inline bool match(std::vector<Node> Vs) const override {
      for (Node &V : Vs) {
        if (N.match(V)) return true;
      }
      return false;
    }
    const Pattern<Node> &N;
  };

  template<typename Node>
  struct m_All : Pattern<std::vector<Node>> {
    m_All(const Pattern<Node> &N) : N(N) {}
    inline bool match(std::vector<Node> Vs) const override {
      bool res = true;
      for (Node &V : Vs) {
        res &= N.match(V);
      }
      return res;
    }
    const Pattern<Node> &N;
  };

  template<typename Node>
  struct m_One : Pattern<std::vector<Node>> {
    m_One(const Pattern<Node> &N) : N(N) {}
    inline bool match(std::vector<Node> Vs) const override {
      return Vs.size() == 1 && N.match(Vs[0]);
    }
    const Pattern<Node> &N;
  };

  template<typename Node>
  struct m_Two : Pattern<std::vector<Node>> {
    m_Two(const Pattern<Node> &N1, const Pattern<Node> &N2) : N1(N1), N2(N2) {}
    inline bool match(std::vector<Node> Vs) const override {
      return Vs.size() == 2 && N1.match(Vs[0]) && N2.match(Vs[1]);
    }
    const Pattern<Node> &N1;
    const Pattern<Node> &N2;
  };

  template<typename Node>
  struct m_Three : Pattern<std::vector<Node>> {
    m_Three(const Pattern<Node> &N1, const Pattern<Node> &N2,const Pattern<Node> &N3) : N1(N1), N2(N2), N3(N3) {}
    inline bool match(std::vector<Node> Vs) const override {
      return Vs.size() == 2 && N1.match(Vs[0]) && N2.match(Vs[1]) && N3.match(Vs[2]);
    }
    const Pattern<Node> &N1;
    const Pattern<Node> &N2;
    const Pattern<Node> &N3;
  };

  template<typename Node>
  struct m_AnySeqTwo : Pattern<std::vector<Node>> {
    m_AnySeqTwo(const Pattern<Node> &N1, const Pattern<Node> &N2) : N1(N1), N2(N2) {}
    inline bool match(std::vector<Node> Vs) const override {
      for (size_t i = 0; i < Vs.size() - 1; i++) {
        if (N1.match(Vs[i]) && N2.match(Vs[i + 1])) return true;
      }
      return false;
    }
    const Pattern<Node> &N1;
    const Pattern<Node> &N2;
  };

  template<typename Node>
  struct m_AnySeqThree : Pattern<std::vector<Node>> {
    m_AnySeqThree(const Pattern<Node> &N1, const Pattern<Node> &N2,const Pattern<Node> &N3) : N1(N1), N2(N2), N3(N3) {}
    inline bool match(std::vector<Node> Vs) const override {
      for (size_t i = 0; i < Vs.size() - 2; i++) {
        if (N1.match(Vs[i]) && N2.match(Vs[i + 1]) && N3.match(Vs[i + 2]) ) return true;
      }
      return false;
    }
    const Pattern<Node> &N1;
    const Pattern<Node> &N2;
    const Pattern<Node> &N3;
  };

  // ==================== Funct ====================

  // TODO: If nessessary add Param, Local and StructDef Matchers
  struct m_Funct : Pattern<const symir::Funct *> {
    m_Funct(
      const Pattern<const std::string> &N,
      const Pattern<std::vector<const symir::Block *>> &B
    ) : N(N), B(B) {}
    inline bool match(const symir::Funct *f) const override {
      return N.match(f->GetName())
          && B.match(f->GetBlocks());
    }
      const Pattern<const std::string> &N;
      const Pattern<std::vector<const symir::Block *>> &B;
  };

  // ==================== Block ====================

  struct m_Block : Pattern<const symir::Block *> {
    m_Block(
      const Pattern<const std::string> &L,
      const Pattern<std::vector<const symir::Stmt *>> &B,
      const Pattern<const symir::Target *> &T
    ) : L(L), B(B), T(T) {}
    inline bool match(const symir::Block *b) const override {
      return L.match(b->GetLabel())
          && B.match(b->GetStmts())
          && T.match(b->GetTarget());
    }
    const Pattern<const std::string> &L;
    const Pattern<std::vector<const symir::Stmt *>> &B;
    const Pattern<const symir::Target *> &T;
  };

  // ==================== Stmt ====================

  struct m_AssStmt : Pattern<const symir::Stmt *> {
    m_AssStmt(const Pattern<const symir::VarUse *> &V, const Pattern<const symir::Expr *> &E) : V(V), E(E) {}
    inline bool match(const symir::Stmt *s) const override {
      if (s->GetIRId() != symir::SymIR::SIR_STMT_ASS) return false;
      const symir::AssStmt *a = static_cast<const symir::AssStmt *>(s);
      return V.match(a->GetVar()) && E.match(a->GetExpr());
    }
    const Pattern<const symir::VarUse *> &V;
    const Pattern<const symir::Expr *> &E;
  };

  struct m_ForStmt : Pattern<const symir::Stmt *> {
    m_ForStmt(
      const Pattern<const symir::VarUse *> &V,
      const Pattern<const symir::Expr *> &I,
      const Pattern<const symir::Cond *> &C,
      const Pattern<const symir::Expr *> &E,
      const Pattern<std::vector<const symir::Stmt *>> &B
    ) : V(V), I(I), C(C), E(E), B(B) {}
    inline bool match(const symir::Stmt *s) const override {
      if (s->GetIRId() != symir::SymIR::SIR_STMT_FOR) return false;
      const symir::ForStmt *f = static_cast<const symir::ForStmt *>(s);
      return V.match(f->GetVar())
          && I.match(f->GetInit())
          && C.match(f->GetCond())
          && E.match(f->GetIncrement())
          && B.match(f->GetBody());
    }
    const Pattern<const symir::VarUse *> &V;
    const Pattern<const symir::Expr *> &I;
    const Pattern<const symir::Cond *> &C;
    const Pattern<const symir::Expr *> &E;
    const Pattern<std::vector<const symir::Stmt *>> &B;
  };

  struct m_WhileStmt : Pattern<const symir::Stmt *> {
    m_WhileStmt(
      const Pattern<const symir::Cond *> &C,
      const Pattern<std::vector<const symir::Stmt *>> &B
    ) : C(C), B(B) {}
    inline bool match(const symir::Stmt *s) const override {
      if (s->GetIRId() != symir::SymIR::SIR_STMT_WHILE) return false;
      const symir::WhileStmt *w = static_cast<const symir::WhileStmt *>(s);
      return C.match(w->GetCond()) && B.match(w->GetBody());
    }
    const Pattern<const symir::Cond *> &C;
    const Pattern<std::vector<const symir::Stmt *>> &B;
  };

  struct m_IfStmt : Pattern<const symir::Stmt *> {
    m_IfStmt(
      const Pattern<std::vector<const symir::Cond *>> &C,
      const Pattern<std::vector<std::vector<const symir::Stmt *>>> &B
    ) : C(C), B(B) {}
    inline bool match(const symir::Stmt *s) const override {
      if (s->GetIRId() != symir::SymIR::SIR_STMT_IF) return false;
      const symir::IfStmt *i = static_cast<const symir::IfStmt *>(s);
      return C.match(i->GetConds()) && B.match(i->GetBodies());
    }
    const Pattern<std::vector<const symir::Cond *>> &C;
    const Pattern<std::vector<std::vector<const symir::Stmt *>>> &B;
  };

  struct m_ModAssStmt : Pattern<const symir::Stmt *> {
    m_ModAssStmt(const Pattern<const symir::VarUse *> &V, const Pattern<const symir::ModExpr *> &E) : V(V), E(E) {}
    inline bool match(const symir::Stmt *s) const override {
      if (s->GetIRId() != symir::SymIR::SIR_STMT_MODASS) return false;
      const symir::ModAssStmt *a = static_cast<const symir::ModAssStmt *>(s);
      return V.match(a->GetVar()) && E.match(a->GetExpr());
    }
    const Pattern<const symir::VarUse *> &V;
    const Pattern<const symir::ModExpr *> &E;
  };

  struct m_RetStmt : Pattern<const symir::Stmt *> {
    m_RetStmt(const Pattern<std::vector<const symir::VarUse *>> &V) : V(V) {}
    inline bool match(const symir::Stmt *s) const override {
      if (s->GetIRId() != symir::SymIR::SIR_STMT_RET) return false;
      const symir::RetStmt *r = static_cast<const symir::RetStmt *>(s);
      return V.match(r->GetVars());
    }
    const Pattern<std::vector<const symir::VarUse *>> &V;
  };

  // ==================== Target ====================
  
  struct m_Branch : Pattern<const symir::Target *> {
    m_Branch(
      const Pattern<const symir::Cond *> &C,
      const Pattern<const std::string> &T,
      const Pattern<const std::string> &F
    ) : C(C), T(T), F(F) {}
    inline bool match(const symir::Target *t) const override {
      if (t->GetIRId() != symir::SymIR::SIR_TGT_BRA) return false;
      const symir::Branch *b = static_cast<const symir::Branch *>(t);
      return C.match(b->GetCond())
          && T.match(b->GetTrueTarget()) 
          && F.match(b->GetFalseTarget());
    }
    const Pattern<const symir::Cond *> &C;
    const Pattern<const std::string> &T;
    const Pattern<const std::string> &F;
  };

  struct m_Goto : Pattern<const symir::Target *> {
    m_Goto(const Pattern<const std::string> &T) : T(T) {}
    inline bool match(const symir::Target *t) const override {
      if (t->GetIRId() != symir::SymIR::SIR_TGT_GOTO) return false;
      const symir::Goto *g = static_cast<const symir::Goto *>(t);
      return T.match(g->GetTarget());
    }
    const Pattern<const std::string> &T;
  };

  // ==================== Cond ====================

#define XX(val, capt, smal, sym)                                                   \
  struct m_Cond##capt : Pattern<const symir::Cond *> {                             \
    m_Cond##capt(const Pattern<const symir::Expr *> &E) : E(E) {}                  \
    inline bool match(const symir::Cond *c) const override {                       \
      return c->GetOp() == symir::Cond::OP_##val && E.match(c->GetExpr());  \
    }                                                                              \
    const Pattern<const symir::Expr *> &E;                                         \
  };
SYMIR_CONDOP_LIST(XX)
#undef XX

  struct m_Cond : Pattern<const symir::Cond *> {
    m_Cond(const Pattern<const symir::Expr *> &E) : E(E) {}
    inline bool match(const symir::Cond *c) const override {
      return E.match(c->GetExpr());
    }
    const Pattern<const symir::Expr *> &E;
  };

  // ==================== ModExpr ====================
  struct m_ModExpr : Pattern<const symir::ModExpr *> {
    m_ModExpr(
      const Pattern<std::vector<const symir::Coef *>> &C,
      const Pattern<std::vector<const symir::VarUse *>> &V,
      const Pattern<std::vector<int>> &P,
      const Pattern<int> &M
    ) : C(C), V(V), P(P), M(M) {}
    inline bool match(const symir::ModExpr *e) const override {
      return C.match(e->GetCoeffs())
          && V.match(e->GetVars())
          && P.match(e->GetPolynomial())
          && M.match(e->GetMod());
    }
    const Pattern<std::vector<const symir::Coef *>> &C;
    const Pattern<std::vector<const symir::VarUse *>> &V;
    const Pattern<std::vector<int>> &P;
    const Pattern<int> &M;
  };

  // ==================== Expr ====================

#define XX(val, capt, smal, sym)                                                   \
  struct m_Expr##capt : Pattern<const symir::Expr *> {                             \
    m_Expr##capt(const Pattern<std::vector<const symir::Term *>> &Ts) : Ts(Ts) {}  \
    inline bool match(const symir::Expr *e) const override {                       \
      return e->GetOp() == symir::Expr::OP_##val && Ts.match(e->GetTerms());       \
    }                                                                              \
    const Pattern<std::vector<const symir::Term *>> &Ts;                           \
  };
SYMIR_EXPROP_LIST(XX)
#undef XX

  struct m_Expr : Pattern<const symir::Expr *> {
    m_Expr(const Pattern<std::vector<const symir::Term *>> &Ts) : Ts(Ts) {}
    inline bool match(const symir::Expr *e) const override {
      return Ts.match(e->GetTerms());
    }
    const Pattern<std::vector<const symir::Term *>> &Ts;
  };

  // ==================== Term ====================

#define XX(val, capt, smal, sym)                                                                     \
  struct m_Term##capt : Pattern<const symir::Term *> {                                                 \
    m_Term##capt(const Pattern<const symir::Coef *> &C, const Pattern<const symir::VarUse *> &V) : C(C), V(V) {}      \
    inline bool match(const symir::Term *t) const override {                                                      \
      return t->GetOp() == symir::Term::OP_##val && C.match(t->GetCoef()) && V.match(t->GetVar()); \
    }                                                                                                \
    const Pattern<const symir::Coef *> &C;                                                                   \
    const Pattern<const symir::VarUse *> &V;                                                                 \
  };
SYMIR_TERMOP_LIST(XX)
#undef XX

  struct m_Term : Pattern<const symir::Term *> {
    m_Term(const Pattern<const symir::Coef *> &C, const Pattern<const symir::VarUse *> &V) : C(C), V(V) {}
    inline bool match(const symir::Term *t) const override {
      return C.match(t->GetCoef()) && V.match(t->GetVar());
    }
    const Pattern<const symir::Coef *> &C;
    const Pattern<const symir::VarUse *> &V;
  };

  // ==================== Coef ====================

  struct m_Solved : Pattern<const symir::Coef *> {
    inline bool match(const symir::Coef *c) const override {
      return c->IsSolved();
    }
  };

  struct m_Specific : Pattern<const symir::Coef *> {
    m_Specific(int32_t val) : val(val) {}
    inline bool match(const symir::Coef *c) const override {
      return c->IsSolved() && c->GetI32Value() == val;
    }
    int32_t val;
  };

  // ==================== VarUse ====================

  struct m_NoVar : Pattern<const symir::VarUse *> {
    inline bool match(const symir::VarUse *v) const override { return v == nullptr; }
  };

  struct m_WithName : Pattern<const symir::VarUse *> {
    m_WithName(const std::string name) : name(name) {}
    inline bool match(const symir::VarUse *v) const override {
      return v->GetName() == name;; 
    }
    const std::string name;
  };

  struct m_WithType : Pattern<const symir::VarUse *> {
    m_WithType(symir::SymIR::Type type) : type(type) {}
    inline bool match(const symir::VarUse *v) const override {
      return v->GetType() == type;; 
    }
    symir::SymIR::Type type;
  };

  // ==================== int ====================
  struct m_Int : Pattern<int> {
    m_Int(int val) : val(val) {}
    inline bool match(int i) const override {
      return i == val;
    }
    int val;
  };

  // ==================== string ====================

  struct m_String : Pattern<const std::string> {
    m_String(const std::string val) : val(val) {}
    inline bool match(const std::string s) const override {
      return s == val;
    }
    const std::string val;
  };

  template<typename Node>
  /// Pattern matches the passed symir Class `N` based on the Pattern `P` passed
  bool match(Node N, const Pattern<Node> &P) { return P.match(N); }

  /// Create a lambda function that captures the pattern to match e.g. `make_matcher(P)(n)` is equivalent to match(n, P)
#define make_matcher(Node, P) ([&](Node N) { return patternmatch::match(N, (P)); })
}

#endif // REIFY_PATTERNMATCH_HPP
