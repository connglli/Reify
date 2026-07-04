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

#ifndef REIFY_RANDFILL_HPP
#define REIFY_RANDFILL_HPP

#include <bitwuzla/cpp/bitwuzla.h>
#include <climits>
#include "lib/lang.hpp"
#include "lib/logger.hpp"


class RandFill : public symir::SymIRVisitor {
  public: 
    RandFill(const symir::Funct *fun, int min = INT_MIN, int max = INT_MAX): fun(fun), min(min), max(max) {}

    void Fill();

  private:
    void Visit(const symir::VarUse &v);
    void Visit(const symir::Coef &c);
    void Visit(const symir::Term &t);
    void Visit(const symir::Expr &e);
    void Visit(const symir::ModExpr &e);
    void Visit(const symir::Cond &c);
    void Visit(const symir::AssStmt &a);
    void Visit(const symir::ModAssStmt &a);
    void Visit(const symir::RetStmt &r);
    void Visit(const symir::Branch &b);
    void Visit(const symir::Goto &g);
    void Visit(const symir::ScaParam &p);
    void Visit(const symir::VecParam &p);
    void Visit(const symir::StructParam &p);
    void Visit(const symir::ScaLocal &l);
    void Visit(const symir::VecLocal &l);
    void Visit(const symir::StructLocal &l);
    void Visit(const symir::StructDef &s);
    void Visit(const symir::Block &b);
    void Visit(const symir::Funct &f);

  private:
    const symir::Funct *fun;
    int min;
    int max;
};

#endif
