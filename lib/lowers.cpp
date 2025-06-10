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

#include "lib/lowers.hpp"

void SymSexpLower::Visit(const Var &v) { out << v.GetName(); }

void SymSexpLower::Visit(const Coef &c) { out << c.GetValue(); }

void SymSexpLower::Visit(const Term &t) {
  out << "(" << Term::GetOpShort(t.GetOp()) << " ";
  t.GetCoef()->Accept(*this);
  out << " ";
  t.GetVar()->Accept(*this);
  out << ")";
}

void SymSexpLower::Visit(const Expr &e) {
  out << "(e" << Expr::GetOpShort(e.GetOp()) << " ";
  auto terms = e.GetTerms();
  for (auto i = 0; i < terms.size(); i++) {
    terms[i]->Accept(*this);
    if (i != terms.size() - 1) {
      out << " ";
    }
  }
  out << ")";
}

void SymSexpLower::Visit(const Cond &c) {
  out << "(" << Cond::GetOpShort(c.GetOp()) << " ";
  c.GetExpr()->Accept(*this);
  out << ")";
}

void SymSexpLower::Visit(const AssStmt &a) {
  indent();
  out << "(asn ";
  a.GetVar()->Accept(*this);
  out << " ";
  incIndent();
  a.GetExpr()->Accept(*this);
  decIndent();
  out << ")" << std::endl;
}

void SymSexpLower::Visit(const RetStmt &r) {
  indent();
  out << "(ret)" << std::endl;
}

void SymSexpLower::Visit(const Branch &b) {
  indent();
  out << "(brh " << b.GetTrueTarget() << " " << b.GetFalseTarget() << " ";
  incIndent();
  b.GetCond()->Accept(*this);
  decIndent();
  out << ")" << std::endl;
}

void SymSexpLower::Visit(const Goto &g) {
  indent();
  out << "goto " << g.GetTarget() << std::endl;
}

void SymSexpLower::Visit(const Block &b) {
  indent();
  out << "(BB " << b.GetLabel() << " " << std::endl;
  for (auto s: b.GetStmts()) {
    incIndent();
    s->Accept(*this);
    decIndent();
  }
  indent();
  out << ")" << std::endl;
}

void SymSexpLower::Visit(const Func &f) {
  out << "(fun " << f.GetName() << " " << SymIR::GetTypeSName(f.GetRetType());
  out << " (";
  auto vars = f.GetParams();
  for (auto i = 0; i < vars.size(); ++i) {
    out << "(" << SymIR::GetTypeSName(vars[i]->GetType()) << " ";
    vars[i]->Accept(*this);
    out << ")";
    if (i != vars.size() - 1) {
      out << " ";
    }
  }
  out << ")" << std::endl;
  for (const auto &b: f.GetBlocks()) {
    incIndent();
    b->Accept(*this);
    decIndent();
  }
  out << ")" << std::endl;
}

void SymCxLower::Visit(const Var &v) { out << v.GetName(); }

void SymCxLower::Visit(const Coef &c) { out << c.GetValue(); }

void SymCxLower::Visit(const Term &t) {
  t.GetCoef()->Accept(*this);
  out << " " << Term::GetOpSym(t.GetOp()) << " ";
  t.GetVar()->Accept(*this);
}

void SymCxLower::Visit(const Expr &e) {
  auto terms = e.GetTerms();
  for (auto i = 0; i < terms.size(); ++i) {
    e.GetTerm(i)->Accept(*this);
    if (i != terms.size() - 1) {
      out << " " << Expr::GetOpSym(e.GetOp());
    }
  }
}

void SymCxLower::Visit(const Cond &c) {
  c.GetExpr()->Accept(*this);
  out << " " << Cond::GetOpSym(c.GetOp()) << " 0";
}

void SymCxLower::Visit(const AssStmt &a) {
  indent();
  a.GetVar()->Accept(*this);
  out << " = ";
  a.GetExpr()->Accept(*this);
  out << ";" << std::endl;
}

void SymCxLower::Visit(const RetStmt &r) {
  indent();
  out << "return computeStatelessChecksum(";
  auto vars = r.GetVars();
  for (auto i = 0; i < vars.size(); ++i) {
    out << SymIR::GetTypeSName(vars[i]->GetType()) << " ";
    vars[i]->Accept(*this);
    if (i != vars.size() - 1) {
      out << ", ";
    }
  }
  out << ");" << std::endl;
}

void SymCxLower::Visit(const Branch &b) {
  indent();
  out << "if (";
  b.GetCond()->Accept(*this);
  out << ") goto " << b.GetTrueTarget() << ";" << std::endl;
  indent();
  out << "goto " << b.GetFalseTarget() << ";" << std::endl;
}

void SymCxLower::Visit(const Goto &g) {
  indent();
  out << "goto " << g.GetTarget() << std::endl;
}

void SymCxLower::Visit(const Block &b) {
  out << b.GetLabel() << ":" << std::endl;
  for (auto s: b.GetStmts()) {
    s->Accept(*this);
  }
}

void SymCxLower::Visit(const Func &f) {
  out << SymIR::GetTypeCName(f.GetRetType()) << " " << f.GetName() << "(";
  auto vars = f.GetParams();
  for (auto i = 0; i < vars.size(); ++i) {
    out << SymIR::GetTypeCName(f.GetType()) << " ";
    vars[i]->Accept(*this);
    if (i != vars.size() - 1) {
      out << ", ";
    }
  }
  out << ") {" << std::endl;
  for (const auto &b: f.GetBlocks()) {
    incIndent();
    b->Accept(*this);
    decIndent();
  }
  out << "}" << std::endl;
}
