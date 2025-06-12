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

#ifndef GRAPHFUZZ_LOWERS_H
#define GRAPHFUZZ_LOWERS_H

#include <ostream>

#include "lang.hpp"

#define SYMIR_LOWER_INDENTATION_SIZE 2

#define SYMIR_KEYWORD_LIST(XX)                                                                     \
  XX(FUN, fun)                                                                                     \
  XX(PAR, par)                                                                                     \
  XX(LOC, loc)                                                                                     \
  XX(BBL, bbl)                                                                                     \
  XX(ASS, asn)                                                                                     \
  XX(RET, ret)                                                                                     \
  XX(BRH, brh)                                                                                     \
  XX(GOTO, goto)

namespace symir {
  using namespace symir;

  class SymIRLower : public SymIRVisitor {

  public:
    explicit SymIRLower(std::ostream &out) : out(out) {}

  protected:
    void indent() const {
      for (int i = 0; i < numIndent; i++) {
        for (int j = 0; j < SYMIR_LOWER_INDENTATION_SIZE; j++) {
          out << " ";
        }
      }
    }

    void incIndent(int inc = 1) { numIndent += inc; }

    void decIndent(int dec = 1) { numIndent -= dec; }

  protected:
    std::ostream &out;
    int numIndent = 0;
  };

  /// An "SymIR -> S Expression" lower
  class SymSexpLower : public SymIRLower {
  public:
#define XX(name, sname) static constexpr char const *KW_##name = #sname;
    SYMIR_KEYWORD_LIST(XX)
#undef XX

  public:
    explicit SymSexpLower(std::ostream &out) : SymIRLower(out) {}

    void Visit(const VarUse &v) override;
    void Visit(const Coef &c) override;
    void Visit(const Term &t) override;
    void Visit(const Expr &e) override;
    void Visit(const Cond &c) override;
    void Visit(const AssStmt &e) override;
    void Visit(const RetStmt &r) override;
    void Visit(const Branch &b) override;
    void Visit(const Goto &g) override;
    void Visit(const Param &p) override;
    void Visit(const Local &l) override;
    void Visit(const Block &b) override;
    void Visit(const Func &f) override;
  };

  /// An "SymIR -> C/C++" lower
  class SymCxLower : public SymIRLower {
  public:
    explicit SymCxLower(std::ostream &out) : SymIRLower(out) {}

    void Visit(const VarUse &v) override;
    void Visit(const Coef &c) override;
    void Visit(const Term &t) override;
    void Visit(const Expr &e) override;
    void Visit(const Cond &c) override;
    void Visit(const AssStmt &e) override;
    void Visit(const RetStmt &r) override;
    void Visit(const Branch &b) override;
    void Visit(const Goto &g) override;
    void Visit(const Param &p) override;
    void Visit(const Local &l) override;
    void Visit(const Block &b) override;
    void Visit(const Func &f) override;
  };
} // namespace symir


#endif // GRAPHFUZZ_LOWERS_H
