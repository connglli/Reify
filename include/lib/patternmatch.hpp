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
#include <cstdint>
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

  template<typename Node, typename Comparable>
  struct m_Eq: Pattern<Node> {
    m_Eq(Comparable val) : val(val) {}
    inline bool match(Node n) const override {
      return n == val;
    }
    Comparable val;
  };

  template<typename Node, typename Comparable>
  struct m_Lt: Pattern<Node> {
    m_Lt(Comparable val) : val(val) {}
    inline bool match(Node n) const override {
      return n < val;
    }
    Comparable val;
  };

  template<typename Node, typename Comparable>
  struct m_Lte: Pattern<Node> {
    m_Lte(Comparable val) : val(val) {}
    inline bool match(Node n) const override {
      return n <= val;
    }
    Comparable val;
  };

  template<typename Node, typename Comparable>
  struct m_Gt: Pattern<Node> {
    m_Gt(Comparable val) : val(val) {}
    inline bool match(Node n) const override {
      return n > val;
    }
    Comparable val;
  };

  template<typename Node, typename Comparable>
  struct m_Gte: Pattern<Node> {
    m_Gte(Comparable val) : val(val) {}
    inline bool match(Node n) const override {
      return n >= val;
    }
    Comparable val;
  };

  template<typename Node, typename Comparable>
  struct m_Range: Pattern<Node> {
    m_Range(Comparable low, Comparable upp) : low(low), upp(upp) {}
    inline bool match(Node n) const override {
      return low <= n && n <= upp;
    }
    Comparable low;
    Comparable upp;
  };

  template<typename Node>
  struct m_And : Pattern<Node> {
    m_And(const Pattern<Node> &N1, const Pattern<Node> &N2) : N1(N1), N2(N2) {}
    inline bool match(Node n) const override {
      return N1.match(n) && N2.match(n);
    }
    const Pattern<Node> &N1;
    const Pattern<Node> &N2;
  };


  template<typename Node>
  struct m_Or : Pattern<Node> {
    m_Or(const Pattern<Node> &N1, const Pattern<Node> &N2) : N1(N1), N2(N2) {}
    inline bool match(Node n) const override {
      return N1.match(n) || N2.match(n);
    }
    const Pattern<Node> &N1;
    const Pattern<Node> &N2;
  };

  template<typename Node>
  struct m_Xor : Pattern<Node> {
    m_Xor(const Pattern<Node> &N1, const Pattern<Node> &N2) : N1(N1), N2(N2) {}
    inline bool match(Node n) const override {
      bool a = N1.match(n);
      bool b = N2.match(n);
      return (a || b) && !(a && b);
    }
    const Pattern<Node> &N1;
    const Pattern<Node> &N2;
  };

  template<typename Node>
  struct m_Not : Pattern<Node> {
    m_Not(const Pattern<Node> &N) : N(N) {}
    inline bool match(Node n) const override {
      return !N.match(n);
    }
    const Pattern<Node> &N;
  };

  template<typename Node>
  struct m_WildCard : Pattern<Node> {
    inline bool match(Node N) const override { return true; }
  };

  template<typename Node>
  struct m_NoMatch : Pattern<Node> {
    inline bool match(Node N) const override { return false; }
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
  struct m_AnyAfter : Pattern<std::vector<Node>> {
    m_AnyAfter(size_t N, const Pattern<Node> &P) : N(N), P(P) {}
    inline bool match(std::vector<Node> Vs) const override {
      for (size_t i = N; i < Vs.size(); i++) {
        if (P.match(Vs[i])) return true;
      }
      return false;
    }
    size_t N;
    const Pattern<Node> &P;
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
  struct m_AtleastN : Pattern<std::vector<Node>> {
    m_AtleastN(const Pattern<Node> &P, size_t N) : P(P), N(N) {}
    inline bool match(std::vector<Node> Vs) const override {
      size_t count = 0;
      for (Node &V : Vs) {
        count += P.match(V) ? 1 : 0;
      }
      return count >= N;
    }
    const Pattern<Node> &P;
    size_t N;
  };

  template<typename Node>
    struct m_FirstN : Pattern<std::vector<Node>> {
      m_FirstN(size_t N, const Pattern<Node> &P) : N(N), P(P) {}

      inline bool match(std::vector<Node> Vs) const override {
        if (Vs.size() < N) return false;
        bool matchFound = true;
        for (size_t i = 0; i < N; i++) {
          matchFound &= P.match(Vs[i]);
        }
        return matchFound;
      }

      size_t N;
      const Pattern<Node> &P;
    };

  template<typename Node, size_t N>
  struct m_NMany: Pattern<std::vector<Node>> {
    template<typename... Args>
    m_NMany(const Args&... args) : patterns{args...} {}

    inline bool match(std::vector<Node> Vs) const override {
      if (Vs.size() != N) return false;

      for (size_t i = 0; i < N; i++) {
        if (!patterns[i].get().match(Vs[i])) return false;
      }
      return true;
    }
    std::array<std::reference_wrapper<const Pattern<Node>>, N> patterns;
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
      return Vs.size() == 3 && N1.match(Vs[0]) && N2.match(Vs[1]) && N3.match(Vs[2]);
    }
    const Pattern<Node> &N1;
    const Pattern<Node> &N2;
    const Pattern<Node> &N3;
  };

  template<typename Node>
  struct m_Four : Pattern<std::vector<Node>> {
    m_Four(const Pattern<Node> &N1, const Pattern<Node> &N2,const Pattern<Node> &N3, const Pattern<Node> &N4) : N1(N1), N2(N2), N3(N3), N4(N4) {}
    inline bool match(std::vector<Node> Vs) const override {
      return Vs.size() == 4 && N1.match(Vs[0]) && N2.match(Vs[1]) && N3.match(Vs[2]);
    }
    const Pattern<Node> &N1;
    const Pattern<Node> &N2;
    const Pattern<Node> &N3;
    const Pattern<Node> &N4;
  };

  template<typename Node, size_t N>
  struct m_AnyNSeq : Pattern<std::vector<Node>> {
    template<typename... Args>
    m_AnyNSeq(const Args&... args) : patterns{args...} {}

    inline bool match(std::vector<Node> Vs) const override {
      if (Vs.size() < N) return false;
      for (size_t i = 0; i < Vs.size() - N; i++) {
        bool matchFound = true;
        for (size_t j = 0; j < N; j++) {
          matchFound &= patterns[j].get().match(Vs[i + j]);
        }
        if (matchFound) return true;
      }
      return false;
    }

    std::array<std::reference_wrapper<const Pattern<Node>>, N> patterns;
  };

  template<typename Node>
  struct m_Length : Pattern<std::vector<Node>> {
    m_Length(const Pattern<size_t>& L) : L(L) {}
    inline bool match(std::vector<Node> Vs) const override {
      return L.match(Vs.size());
    }
    const Pattern<size_t> &L;
  };

  template<typename Node>
  struct m_AnyTwoSeq : Pattern<std::vector<Node>> {
    m_AnyTwoSeq(const Pattern<Node> &N1, const Pattern<Node> &N2) : N1(N1), N2(N2) {}
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
  struct m_AnyThreeSeq : Pattern<std::vector<Node>> {
    m_AnyThreeSeq(const Pattern<Node> &N1, const Pattern<Node> &N2, const Pattern<Node> &N3) : N1(N1), N2(N2), N3(N3) {}
    inline bool match(std::vector<Node> Vs) const override {
      for (size_t i = 0; i < Vs.size() - 2; i++) {
        if (N1.match(Vs[i]) && N2.match(Vs[i + 1]) && N3.match(Vs[i + 2])) return true;
      }
      return false;
    }
    const Pattern<Node> &N1;
    const Pattern<Node> &N2;
    const Pattern<Node> &N3;
  };

  template<typename Node>
  struct m_AnyFourSeq : Pattern<std::vector<Node>> {
    m_AnyFourSeq(const Pattern<Node> &N1, const Pattern<Node> &N2, const Pattern<Node> &N3, const Pattern<Node> &N4) : N1(N1), N2(N2), N3(N3), N4(N4) {}
    inline bool match(std::vector<Node> Vs) const override {
      for (size_t i = 0; i < Vs.size() - 3; i++) {
        if (N1.match(Vs[i]) && N2.match(Vs[i + 1]) && N3.match(Vs[i + 2]) && N4.match(Vs[i + 3])) return true;
      }
      return false;
    }
    const Pattern<Node> &N1;
    const Pattern<Node> &N2;
    const Pattern<Node> &N3;
    const Pattern<Node> &N4;
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
  
  struct m_Branch : Pattern<const symir::Stmt *> {
    m_Branch(
      const Pattern<const symir::Cond *> &C,
      const Pattern<const std::string> &T,
      const Pattern<const std::string> &F
    ) : C(C), T(T), F(F) {}
    inline bool match(const symir::Stmt *t) const override {
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

  struct m_Goto : Pattern<const symir::Stmt *> {
    m_Goto(const Pattern<const std::string> &T) : T(T) {}
    inline bool match(const symir::Stmt *t) const override {
      if (t->GetIRId() != symir::SymIR::SIR_TGT_GOTO) return false;
      const symir::Goto *g = static_cast<const symir::Goto *>(t);
      return T.match(g->GetTarget());
    }
    const Pattern<const std::string> &T;
  };

  // ==================== Cond ====================

#define XX(val, capt, smal, sym)                                                   \
  struct m_##capt##Cond : Pattern<const symir::Cond *> {                             \
    m_##capt##Cond(const Pattern<const symir::Expr *> &E) : E(E) {}                  \
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
  struct m_##capt##Expr : Pattern<const symir::Expr *> {                             \
    m_##capt##Expr(const Pattern<std::vector<const symir::Term *>> &Ts) : Ts(Ts) {}  \
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
  struct m_##capt##Term : Pattern<const symir::Term *> {                                                 \
    m_##capt##Term(const Pattern<const symir::Coef *> &C, const Pattern<const symir::VarUse *> &V) : C(C), V(V) {}      \
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

  /// checks if a term trivially just contains the variable (e.g. 1 * x or 0 +- x etc)
  struct m_VarTerm : Pattern<const symir::Term *> {
    m_VarTerm(const symir::VarUse **v = nullptr) : v(v) {}
    inline bool match (const symir::Term *t) const override {
      switch (t->GetOp()) {
      case symir::Term::OP_ADD: {
        symir::Coef *c = t->GetCoef();
        if (v != nullptr) *v = t->GetVar();
        return c->IsSolved() && c->GetI32Value() == 0;
      } break;
      case symir::Term::OP_MUL: {
        symir::Coef *c = t->GetCoef();
        if (v != nullptr) *v = t->GetVar();
        return c->IsSolved() && c->GetI32Value() == 1;
      } break;
      case symir::Term::OP_AND: {
        symir::Coef *c = t->GetCoef();
        if (v != nullptr) *v = t->GetVar();
        return c->IsSolved() && c->GetI32Value() == -1;
      } break;
      case symir::Term::OP_OR:
      case symir::Term::OP_SHR: {
        symir::Coef *c = t->GetCoef();
        if (v != nullptr) *v = t->GetVar();
        return c->IsSolved() && c->GetI32Value() == 0;
      } break;
      default: return false;
      }
    }
    const symir::VarUse **v;
  };

  // ==================== Coef ====================

  struct m_Solved : Pattern<const symir::Coef *> {
    m_Solved(const symir::Coef **c = nullptr) : c(c) {}
    inline bool match(const symir::Coef *C) const override {
      if (c != nullptr) *c = C;
      return C->IsSolved();
    }
    const symir::Coef **c;
  };

  struct m_Value : Pattern<const symir::Coef *> {
    m_Value(const Pattern<int> &N) : N(N) {}
    inline bool match(const symir::Coef *c) const override {
      return c->IsSolved() && N.match(c->GetI32Value());
    }
    const Pattern<int> &N;
  };

  template<>
  struct m_Eq<const symir::Coef *, int32_t> : Pattern<const symir::Coef *> {
    m_Eq(int32_t val) : val(val) {}
    inline bool match(const symir::Coef *c) const override {
      return c->IsSolved() && c->GetI32Value() == val;
    }
    int32_t val;
  };

  template<>
  struct m_Lt<const symir::Coef *, int32_t>: Pattern<const symir::Coef *> {
    m_Lt(int32_t val) : val(val) {}
    inline bool match(const symir::Coef * c) const override {
      return c->IsSolved() && c->GetI32Value() < val;
    }
    int32_t val;
  };

  template<>
  struct m_Lte<const symir::Coef *, int32_t>: Pattern<const symir::Coef *> {
    m_Lte(int32_t val) : val(val) {}
    inline bool match(const symir::Coef * c) const override {
      return c->IsSolved() && c->GetI32Value() <= val;
    }
    int32_t val;
  };

  template<>
  struct m_Gt<const symir::Coef *, int32_t>: Pattern<const symir::Coef *> {
    m_Gt(int32_t val) : val(val) {}
    inline bool match(const symir::Coef * c) const override {
      return c->IsSolved() && c->GetI32Value() > val;
    }
    int32_t val;
  };

  template<>
  struct m_Gte<const symir::Coef *, int32_t>: Pattern<const symir::Coef *> {
    m_Gte(int32_t val) : val(val) {}
    inline bool match(const symir::Coef * c) const override {
      return c->IsSolved() && c->GetI32Value() >= val;
    }
    int32_t val;
  };

  template<>
  struct m_Range<const symir::Coef *, int32_t>: Pattern<const symir::Coef *> {
    m_Range(int32_t low, int32_t upp) : low(low), upp(upp) {}
    inline bool match(const symir::Coef *c) const override {
      if (!c->IsSolved()) return false;
      int32_t n = c->GetI32Value();
      return low <= n && n <= upp;
    }
    int32_t low;
    int32_t upp;
  };

  // ==================== VarUse ====================

  struct m_NoVar : Pattern<const symir::VarUse *> {
    inline bool match(const symir::VarUse *v) const override { return v == nullptr; }
  };

  struct m_Var : Pattern<const symir::VarUse *> {
    m_Var(const symir::VarUse **v = nullptr) : v(v) {}
    inline bool match(const symir::VarUse *V) const override { 
      if (v != nullptr) *v = V;
      return V != nullptr; 
    }
    const symir::VarUse **v;
  };

  struct m_SpecificVar : Pattern<const symir::VarUse *> {
    m_SpecificVar(const symir::VarUse *v) : v(v) {}
    inline bool match(const symir::VarUse *V) const override { 
      if (v->GetDef() != V->GetDef()) return false;
      auto givenAccess = v->GetAccess();
      auto matchingAccess = V->GetAccess();
      if (givenAccess.size() != matchingAccess.size()) return false;
      for (size_t i = 0; i < givenAccess.size(); i++) {
        if (givenAccess[i] != matchingAccess[i]) return false;
      }
      return true;
    }
    const symir::VarUse *v;
  };

  struct m_WithName : Pattern<const symir::VarUse *> {
    m_WithName(const std::string name, const symir::VarUse **v = nullptr) : v(v), name(name) {}
    inline bool match(const symir::VarUse *V) const override {
      if (v != nullptr) *v = V;
      return V->GetName() == name;; 
    }
    const symir::VarUse **v;
    const std::string name;
  };

  struct m_WithType : Pattern<const symir::VarUse *> {
    m_WithType(symir::SymIR::Type type, const symir::VarUse **v = nullptr) : v(v), type(type) {}
    inline bool match(const symir::VarUse *V) const override {
      if (v != nullptr) *v = V;
      return V->GetType() == type; 
    }
    const symir::VarUse **v;
    symir::SymIR::Type type;
  };

  struct m_ScalarVar : Pattern<const symir::VarUse *> {
    m_ScalarVar(const symir::VarUse **v = nullptr) : v(v) {}
    inline bool match(const symir::VarUse *V) const override {
      if (v != nullptr) *v = V;
      return V->GetType() == symir::SymIR::Type::I32 && !V->IsVector(); 
    }
    const symir::VarUse **v;
  };

  struct m_VectorVar : Pattern<const symir::VarUse *> {
    m_VectorVar(const symir::VarUse **v = nullptr) : v(v) {}
    inline bool match(const symir::VarUse *V) const override {
      if (v != nullptr) *v = V;
      return V->IsVector(); 
    }
    const symir::VarUse **v;
  };

  struct m_StructVar : Pattern<const symir::VarUse *> {
    m_StructVar(const symir::VarUse **v = nullptr) : v(v) {}
    inline bool match(const symir::VarUse *v) const override {
      return !v->IsVector() && v->GetDef()->GetStructName() != ""; 
    }
    const symir::VarUse **v;
  };

  // ==================== int ====================


  struct m_Int : Pattern<int> {
    m_Int(int *val = nullptr) : val(val) {}
    inline bool match(int i) const override {
      if (val != nullptr) *val = i;
      return true;
    }
    int *val;
  };

  struct m_UnsetBits : Pattern<int> {
    m_UnsetBits(uint32_t mask) : mask(mask) {}
    inline bool match(int i) const override {
      return !(((uint32_t) i) & mask);
    }
    uint32_t mask;
  };

  // ==================== string ====================

  struct m_String : Pattern<const std::string> {
    m_String(std::string *val = nullptr) : val(val) {}
    inline bool match(const std::string s) const override {
      if (val != nullptr) *val = std::string(s);
      return true;
    }
    std::string *val;
  };

  template<typename Node>
  /// Pattern matches the passed symir Class `N` based on the Pattern `P` passed
  bool match(Node N, const Pattern<Node> &P) { return P.match(N); }

  /// Create a lambda function that captures the pattern to match e.g. `make_matcher(P)(n)` is equivalent to match(n, P)
#define make_matcher(Node, P) ([&](Node N) { return patternmatch::match(N, (P)); })
}

#endif // REIFY_PATTERNMATCH_HPP
