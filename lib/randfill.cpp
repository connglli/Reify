

#include "lib/randfill.hpp"
#include <string>
#include "lib/random.hpp"

void RandFill::Fill() { this->fun->Accept(*this); }

void RandFill::Visit(const symir::VarUse &v) {
  for (auto &c: v.GetAccess()) {
    c->Accept(*this);
  }
}

void RandFill::Visit(const symir::Coef &c) {
  const auto rand = Random::Get().Uniform(this->min, this->max);
  if (c.IsSolved()) {
    return;
  }

  const int val = rand();
  // hacky, but not UB since the underlying Coef is non-const, see SymCoef in FunctBuilder
  symir::Coef &mut_c = const_cast<symir::Coef &>(c);
  mut_c.SetValue(std::to_string(val));
  Log::Get().Out() << "Define symbols: sym=" << c.GetName() << ", val=" << val
                   << " (for unexecuted basic blocks)" << std::endl;
}

void RandFill::Visit(const symir::Term &t) {
  symir::Term::Op op = t.GetOp();

  switch (op) {
    case symir::Term::OP_ADD:
    case symir::Term::OP_SUB:
    case symir::Term::OP_MUL:
    case symir::Term::OP_DIV:
    case symir::Term::OP_REM:
    case symir::Term::OP_AND:
    case symir::Term::OP_XOR:
    case symir::Term::OP_OR: {
      t.GetVar()->Accept(*this);
    }
      [[fallthrough]];
    case symir::Term::OP_CST: {
      t.GetCoef()->Accept(*this);
    }; break;
    case symir::Term::OP_NOT: {
      t.GetVar()->Accept(*this);
      if (t.GetCoef() != nullptr)
        t.GetCoef()->Accept(*this);
    }; break;
    case symir::Term::OP_SHL:
    case symir::Term::OP_SHR: {
      int old_min = this->min;
      int old_max = this->max;

      this->min = std::max(this->min, 0);
      this->max = std::min(this->max, 31);
      Assert(this->min <= this->max, "Minimum must be less then Maximum");

      t.GetVar()->Accept(*this);
      t.GetCoef()->Accept(*this);

      this->min = old_min;
      this->max = old_max;
    }; break;
    default: {
      Panic("Is Unreachable");
    }
  }
}

void RandFill::Visit(const symir::Expr &e) {
  const auto &terms = e.GetTerms();
  for (const auto &t: terms) {
    t->Accept(*this);
  }
}

void RandFill::Visit(const symir::ModExpr &e) {
  std::vector<symir::Coef *> coeffs;
  for (const auto &c: e.GetCoeffs()) {
    c->Accept(*this);
  }
  for (const auto &use: e.GetVars()) {
    use->Accept(*this);
  }
}

void RandFill::Visit(const symir::Cond &c) { c.GetExpr()->Accept(*this); }

void RandFill::Visit(const symir::AssStmt &a) {
  a.GetVar()->Accept(*this);
  a.GetExpr()->Accept(*this);
}

void RandFill::Visit(const symir::ModAssStmt &a) {
  a.GetVar()->Accept(*this);
  a.GetExpr()->Accept(*this);
}

void RandFill::Visit(const symir::RetStmt &r) { /* Nothing to do here */ }

void RandFill::Visit(const symir::Branch &b) { b.GetCond()->Accept(*this); }

void RandFill::Visit(const symir::Goto &g) { /* Nothing to do here */ }

void RandFill::Visit(const symir::ScaParam &p) { /* Nothing to do here */ }

void RandFill::Visit(const symir::VecParam &p) { /* Nothing to do here */ }

void RandFill::Visit(const symir::StructParam &p) { /* Nothing to do here */ }

void RandFill::Visit(const symir::ScaLocal &l) { /* Nothing to do here */ }

void RandFill::Visit(const symir::VecLocal &l) { /* Nothing to do here */ }

void RandFill::Visit(const symir::StructLocal &l) { /* Nothing to do here */ }

void RandFill::Visit(const symir::StructDef &s) { /* Nothing to do here */ }

void RandFill::Visit(const symir::Block &b) {
  for (const auto &s: b.GetStmts()) {
    s->Accept(*this);
  }
}

void RandFill::Visit(const symir::Funct &f) {
  for (const auto &b: f.GetBlocks()) {
    b->Accept(*this);
  }
}
