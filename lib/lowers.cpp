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
#include "lib/chksum.hpp"
#include "lib/jnifutils.hpp"
#include "lib/lang.hpp"
#include "lib/logger.hpp"

namespace symir {
  std::ostream SymIRLower::devNull(nullptr);

  void SymSexpLower::Visit(const VarUse &v) {
    out << v.GetName();

    const VarDef *currVar = v.GetDef();
    SymIR::Type currType = currVar->GetType();
    SymIR::Type currBaseType = currVar->GetBaseType();
    std::string currStruct = (currType == SymIR::STRUCT) ? currVar->GetStructName() : "";
    int remainingDims = currVar->IsVector() ? currVar->GetVecNumDims() : 0;

    for (const auto &c: v.GetAccess()) {
      if (remainingDims > 0) {
        out << "[";
        c->Accept(*this);
        out << "]";
        remainingDims--;
        // After consuming all array dimensions, resolve to baseType
        if (remainingDims == 0 && currType == SymIR::ARRAY) {
          currType = currBaseType;
          // Note: currStruct should already be set correctly from field access
          // Don't overwrite it here!
        }
      } else if (currType == SymIR::STRUCT) {
        if (c->IsValueSet() && curFunct) {
          int idx = c->GetI32Value();
          const auto *sDef = curFunct->GetStruct(currStruct);
          if (sDef && idx >= 0 && idx < (int) sDef->GetFields().size()) {
            const auto &field = sDef->GetField(idx);
            out << "[" << field.name << "]";

            // Update type tracking similar to SymCxLower
            currType = field.type;
            currBaseType = field.baseType;
            if (field.type == SymIR::ARRAY) {
              remainingDims = field.shape.size();
              if (field.baseType == SymIR::STRUCT) {
                currStruct = field.structName;
              }
            } else if (field.type == SymIR::STRUCT) {
              currStruct = field.structName;
              remainingDims = 0;
            } else {
              remainingDims = 0;
            }
          } else {
            out << "[UNKNOWN_FIELD_" << idx << "]";
          }
        } else {
          out << "[";
          c->Accept(*this);
          out << "]";
        }
      } else {
        // Should not happen for scalar i32
        out << "[";
        c->Accept(*this);
        out << "]";
      }
    }
  }

  void SymSexpLower::Visit(const Coef &c) {
    if (c.IsValueSet()) {
      out << KW_COEF_VAL_SYM << c.GetValue();
    } else {
      out << c.GetName();
    }
  }

  void SymSexpLower::Visit(const Term &t) {
    out << "(" << Term::GetOpShort(t.GetOp()) << " ";
    t.GetCoef()->Accept(*this);
    if (t.GetOp() != Term::Op::OP_CST) {
      out << " ";
      t.GetVar()->Accept(*this);
    }
    out << ")";
  }

  void SymSexpLower::Visit(const ModExpr &e) {
    auto coeffs = e.GetCoeffs();
    auto vars = e.GetVars();
    auto polynomial = e.GetPolynomial();
    int mod = e.GetMod();
    out << "(emodadd ";
    for (size_t i = 0; i < coeffs.size() - 1; ++i) {
      this->out << "(";
    }
    for (size_t i = 0; i < coeffs.size() - 1; ++i) {
      out << "(emodmul ";
      coeffs[i]->Accept(*this);
      for (size_t j = 0; j < vars.size(); ++j) {
        for (int d = 0; d < polynomial[vars.size()* j + i]; d++) {
          this->out << " ";
          vars[j]->Accept(*this);
        }
      }
      this->out << "#" << mod << ")";
      this->out << " ";
    }
    coeffs.back()->Accept(*this);
    this->out << "#" << mod << ")";
  }

  void SymSexpLower::Visit(const Expr &e) {
    out << "(e" << Expr::GetOpShort(e.GetOp()) << " ";
    auto terms = e.GetTerms();
    for (size_t i = 0; i < terms.size(); i++) {
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

  void SymSexpLower::Visit(const ModAssStmt &a) {
    indent();
    out << "(" << KW_ASS << " ";
    a.GetVar()->Accept(*this);
    out << " ";
    incIndent();
    a.GetExpr()->Accept(*this);
    decIndent();
    out << ")" << std::endl;
  }

  void SymSexpLower::Visit(const AssStmt &a) {
    indent();
    out << "(" << KW_ASS << " ";
    a.GetVar()->Accept(*this);
    out << " ";
    incIndent();
    a.GetExpr()->Accept(*this);
    decIndent();
    out << ")" << std::endl;
  }

  void SymSexpLower::Visit(const RetStmt &r) {
    indent();
    out << "(" << KW_RET << ")" << std::endl;
  }

  void SymSexpLower::Visit(const Branch &b) {
    indent();
    out << "(" << KW_BRH << " " << b.GetTrueTarget() << " " << b.GetFalseTarget() << " ";
    incIndent();
    b.GetCond()->Accept(*this);
    decIndent();
    out << ")" << std::endl;
  }

  void SymSexpLower::Visit(const Goto &g) {
    indent();
    out << "(" << KW_GOTO << " " << g.GetTarget() << ")" << std::endl;
  }

  void SymSexpLower::Visit(const ScaParam &p) {
    if (p.IsVolatile()) {
      out << "(" << KW_VOL << " " << KW_PAR << " " << p.GetName() << " "
          << SymIR::GetTypeSName(p.GetType()) << ")";
    } else {
      out << "(" << KW_PAR << " " << p.GetName() << " " << SymIR::GetTypeSName(p.GetType()) << ")";
    }
  }

  void SymSexpLower::Visit(const VecParam &p) {
    out << "(";
    if (p.IsVolatile()) {
      out << KW_VOL << " ";
    }
    out << KW_PAR << " " << p.GetName() << " ";
    // Output type with array dimensions: Type[dim1][dim2]...
    if (p.GetBaseType() == SymIR::STRUCT) {
      out << p.GetStructName();
    } else {
      out << SymIR::GetTypeSName(p.GetBaseType());
    }
    for (const auto &l: p.GetVecShape()) {
      out << "[" << l << "]";
    }
    out << ")";
  }

  void SymSexpLower::Visit(const StructParam &p) {
    out << "(" << KW_PAR << " " << p.GetName() << " " << p.GetStructName() << ")";
  }

  void SymSexpLower::Visit(const UnInitLocal &l) {
    indent();
    if (l.IsVolatile()) {
      out << "(" << KW_VOL << " ";
    } else {
      out << "(" ;
    }
    out << KW_LOC << " " << l.GetName() << " " << SymIR::GetTypeSName(l.GetType()) << ")" << std::endl;
  }

  void SymSexpLower::Visit(const ScaLocal &l) {
    indent();
    if (l.IsVolatile()) {
      out << "(" << KW_VOL << " " << KW_LOC << " " << l.GetName() << " ";
    } else {
      out << "(" << KW_LOC << " " << l.GetName() << " ";
    }
    l.GetCoef()->Accept(*this);
    out << " " << SymIR::GetTypeSName(l.GetType()) << ")" << std::endl;
  }

  void SymSexpLower::Visit(const VecLocal &l) {
    indent();
    out << "(";
    if (l.IsVolatile()) {
      out << KW_VOL << " ";
    }
    out << KW_LOC << " " << l.GetName() << " ";
    // Output init values first
    for (auto c: l.GetCoefs()) {
      c->Accept(*this);
      out << " ";
    }
    // Then output type with array dimensions: Type[dim1][dim2]...
    if (l.GetBaseType() == SymIR::STRUCT) {
      out << l.GetStructName();
    } else {
      out << SymIR::GetTypeSName(l.GetBaseType());
    }
    for (auto len: l.GetVecShape()) {
      out << "[" << len << "]";
    }
    out << ")" << std::endl;
  }

  void SymSexpLower::Visit(const StructLocal &l) {
    indent();
    out << "(" << KW_LOC << " " << l.GetName() << " ";
    for (auto c: l.GetCoefs()) {
      c->Accept(*this);
      out << " ";
    }
    out << l.GetStructName() << ")" << std::endl;
  }

  void SymSexpLower::Visit(const StructDef &s) {
    out << "(" << KW_STRUCT << " " << s.GetName() << " (";
    for (size_t i = 0; i < s.GetFields().size(); ++i) {
      const auto &f = s.GetField(i);
      out << "(" << f.name << " ";
      // Output type first, then dimensions
      if (f.baseType == SymIR::STRUCT)
        out << f.structName;
      else
        out << SymIR::GetTypeSName(f.baseType);
      // Then output dimensions
      for (int d: f.shape)
        out << "[" << d << "]";
      out << ")";
      if (i != s.GetFields().size() - 1) {
        out << " ";
      }
    }
    out << "))" << std::endl;
  }

  void SymSexpLower::Visit(const Block &b) {
    indent();
    out << "(" << KW_BBL << " " << b.GetLabel() << " " << std::endl;
    for (auto s: b.GetStmts()) {
      incIndent();
      s->Accept(*this);
      decIndent();
    }
    indent();
    out << ")" << std::endl;
  }

  void SymSexpLower::Visit(const Funct &f) {
    curFunct = &f;
    for (const auto &s: f.GetStructs()) {
      s->Accept(*this);
    }
    out << "(" << KW_FUN << " " << f.GetName() << " " << SymIR::GetTypeSName(f.GetRetType());
    out << " (";
    auto params = f.GetParams();
    for (size_t i = 0; i < params.size(); ++i) {
      params[i]->Accept(*this);
      if (i != params.size() - 1) {
        out << " ";
      }
    }
    out << ")" << std::endl;
    for (const auto &l: f.GetLocals()) {
      incIndent();
      l->Accept(*this);
      decIndent();
    }
    for (const auto &b: f.GetBlocks()) {
      incIndent();
      b->Accept(*this);
      decIndent();
    }
    out << ")" << std::endl;
    curFunct = nullptr;
  }

  std::string SymCxLower::GetFunPrototype(const Funct &f) {
    std::ostringstream oss;
    for (const auto &s: f.GetStructs()) {
      oss << "struct " << s->GetName() << " {" << std::endl;
      for (const auto &field: s->GetFields()) {
        if (field.baseType == SymIR::STRUCT) {
          oss << "  struct " << field.structName << " " << field.name;
        } else {
          oss << "  " << symir::SymIR::GetTypeCName(field.baseType) << " " << field.name;
        }
        for (int d: field.shape)
          oss << "[" << d << "]";
        oss << ";" << std::endl;
      }
      oss << "};" << std::endl;
    }

    oss << symir::SymIR::GetTypeCName(f.GetRetType()) << " " << f.GetName() << "(";
    auto params = f.GetParams();
    for (size_t i = 0; i < params.size(); ++i) {
      const auto &p = params[i];
      if (p->IsVolatile()) {
        oss << "volatile ";
      }
      // For vector/array parameters, use GetBaseType() to get the element type
      if (p->IsVector() || p->GetType() == SymIR::ARRAY) {
        if (p->GetBaseType() == SymIR::STRUCT) {
          oss << "struct " << p->GetStructName() << " " << p->GetName();
        } else {
          oss << symir::SymIR::GetTypeCName(p->GetBaseType()) << " " << p->GetName();
        }
        for (const auto &l: p->GetVecShape()) {
          oss << "[" << l << "]";
        }
      } else if (p->GetType() == SymIR::STRUCT) {
        oss << "struct " << p->GetStructName() << " " << p->GetName();
      } else {
        oss << symir::SymIR::GetTypeCName(p->GetType()) << " " << p->GetName();
      }
      if (i != params.size() - 1) {
        oss << ", ";
      }
    }
    oss << ")";
    return oss.str();
  }

  void SymCxLower::Visit(const VarUse &v) {
    out << v.GetName();

    const VarDef *currVar = v.GetDef();
    SymIR::Type currType = currVar->GetType();
    SymIR::Type currBaseType = currVar->GetBaseType();
    std::string currStruct = (currType == SymIR::STRUCT) ? currVar->GetStructName() : "";
    int remainingDims = currVar->IsVector() ? currVar->GetVecNumDims() : 0;

    for (const auto &c: v.GetAccess()) {
      if (remainingDims > 0) {
        out << "[";
        c->Accept(*this);
        out << "]";
        remainingDims--;
        // After consuming all array dimensions, resolve to baseType
        if (remainingDims == 0 && currType == SymIR::ARRAY) {
          currType = currBaseType;
          // Note: currStruct should already be set correctly from field access
          // Don't overwrite it here!
        }
      } else if (currType == SymIR::STRUCT) {
        if (c->IsValueSet() && curFunct) {
          int idx = c->GetI32Value();
          const auto *sDef = curFunct->GetStruct(currStruct);
          if (sDef && idx >= 0 && idx < (int) sDef->GetFields().size()) {
            const auto &field = sDef->GetField(idx);
            out << "." << field.name;

            // Update type tracking: if field is an array, track dimensions
            // If field is a struct (or array of struct), track the struct name
            currType = field.type;
            currBaseType = field.baseType;
            if (field.type == SymIR::ARRAY) {
              remainingDims = field.shape.size();
              // After consuming array dimensions, we'll have baseType
              if (field.baseType == SymIR::STRUCT) {
                currStruct = field.structName;
              }
            } else if (field.type == SymIR::STRUCT) {
              currStruct = field.structName;
              remainingDims = 0;
            } else {
              // Scalar field (I32)
              remainingDims = 0;
            }
          } else {
            out << ".field_" << idx;
          }
        } else {
          out << ".UNKNOWN";
        }
      }
    }
  }

  void SymCxLower::Visit(const Coef &c) {
    if (c.IsValueSet()) {
      out << c.GetValue();
    } else {
      out << c.GetName();
    }
  }

  void SymCxLower::Visit(const Term &t) {
    out << "(";
    t.GetCoef()->Accept(*this);
    if (t.GetOp() != Term::Op::OP_CST) {
      out << " " << Term::GetOpSym(t.GetOp()) << " ";
      t.GetVar()->Accept(*this);
    }
    out << ")";
  }

  void SymCxLower::Visit(const ModExpr &e) {
    auto coeffs = e.GetCoeffs();
    auto vars = e.GetVars();
    auto polynomial = e.GetPolynomial();
    int mod = e.GetMod();
    for (size_t i = 0; i < coeffs.size() - 1; ++i) {
      // by precidence we must wrap each addition of a monomial in a bracket
      this->out << "(";
    }
    for (size_t i = 0; i < coeffs.size() - 1; ++i) {
      coeffs[i]->Accept(*this);
      for (size_t j = 0; j < vars.size(); ++j) {
        Assert(vars.size() * i + j < polynomial.size(), "polynomial array out of bounds");
        for (int d = 0; d < polynomial[vars.size() * i + j]; d++) {
          this->out << " * RM(";
          vars[j]->Accept(*this);
          this->out << ", " << mod << ")";
          this->out << " % " << mod;
        }
      }
      if (i >= 1)
        this->out << ") % " << mod;
      this->out << " + ";
    }
    coeffs.back()->Accept(*this);
    this->out << ") % " << mod;
  }

  void SymCxLower::Visit(const Expr &e) {
    auto terms = e.GetTerms();
    for (size_t i = 0; i < terms.size(); ++i) {
      e.GetTerm(i)->Accept(*this);
      if (i != terms.size() - 1) {
        out << " " << Expr::GetOpSym(e.GetOp()) + " ";
      }
    }
  }

  void SymCxLower::Visit(const Cond &c) {
    c.GetExpr()->Accept(*this);
    out << " " << Cond::GetOpSym(c.GetOp()) << " 0";
  }

  void SymCxLower::Visit(const ModAssStmt &a) {
    indent();
    a.GetVar()->Accept(*this);
    out << " = ";
    a.GetExpr()->Accept(*this);
    out << ";" << std::endl;
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
    out << "{" << std::endl;
    incIndent();
    indent();
    out << "int __chk_args[] = {";

    auto vars = r.GetVars();
    bool first = true;

    auto printVal = [&](std::function<void()> printer) {
      if (!first)
        out << ", ";
      first = false;
      printer();
    };

    std::function<void(const StructDef *, std::function<void()>)> printStruct;
    printStruct = [&](const StructDef *sDef, std::function<void()> printPrefix) {
      for (const auto &f: sDef->GetFields()) {
        if (f.baseType == SymIR::STRUCT) {
          const auto *subDef = curFunct->GetStruct(f.structName);
          std::function<void(const std::vector<int> &, std::function<void()>)> printArray =
              [&, this](const std::vector<int> &shape, std::function<void()> pP) {
                if (shape.empty()) {
                  printStruct(subDef, pP);
                } else {
                  int d = shape[0];
                  std::vector<int> subShape(shape.begin() + 1, shape.end());
                  for (int i = 0; i < d; ++i) {
                    printArray(subShape, [=, this]() {
                      pP();
                      out << "[" << i << "]";
                    });
                  }
                }
              };
          printArray(f.shape, [=, this]() {
            printPrefix();
            out << "." << f.name;
          });
        } else {
          std::function<void(const std::vector<int> &, std::function<void()>)> printArray =
              [&, this](const std::vector<int> &shape, std::function<void()> pP) {
                if (shape.empty()) {
                  printVal(pP);
                } else {
                  int d = shape[0];
                  std::vector<int> subShape(shape.begin() + 1, shape.end());
                  for (int i = 0; i < d; ++i) {
                    printArray(subShape, [=, this]() {
                      pP();
                      out << "[" << i << "]";
                    });
                  }
                }
              };
          printArray(f.shape, [=, this]() {
            printPrefix();
            out << "." << f.name;
          });
        }
      }
    };

    for (const auto *v: vars) {
      // The VarUse has already been expanded by SymReturn(), so we just need to print it
      printVal([=, this]() { v->Accept(*this); });
    }

    out << "};" << std::endl;
    indent();
    out << "return " << StatelessChecksum::GetComputeName()
        << "(sizeof(__chk_args)/sizeof(int), __chk_args);" << std::endl;

    decIndent();
    indent();
    out << "}" << std::endl;
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
    out << "goto " << g.GetTarget() << ";" << std::endl;
  }

  void SymCxLower::Visit(const ScaParam &p) {
    if (p.IsVolatile()) {
      out << "volatile" << " " << SymIR::GetTypeCName(p.GetType()) << " " << p.GetName();
    } else {
      out << SymIR::GetTypeCName(p.GetType()) << " " << p.GetName();
    }
  }

  void SymCxLower::Visit(const VecParam &p) {
    if (p.IsVolatile()) {
      out << "volatile" << " ";
    }
    // For ARRAY type, use GetBaseType() to get the element type
    if (p.GetBaseType() == SymIR::STRUCT) {
      out << "struct " << p.GetStructName() << " " << p.GetName();
    } else {
      out << SymIR::GetTypeCName(p.GetBaseType()) << " " << p.GetName();
    }
    for (const auto &l: p.GetVecShape()) {
      out << "[" << l << "]";
    }
  }

  void SymCxLower::Visit(const StructParam &p) {
    out << "struct " << p.GetStructName() << " " << p.GetName();
  }

  void SymCxLower::Visit(const UnInitLocal &l) {
    indent();
    if (l.IsVolatile()) {
      out << "volatile" << " ";
    }
    out << SymIR::GetTypeCName(l.GetType()) << " " << l.GetName() << ";" << std::endl;
  }

  void SymCxLower::Visit(const ScaLocal &l) {
    indent();
    if (l.IsVolatile()) {
      out << "volatile" << " " << SymIR::GetTypeCName(l.GetType()) << " " << l.GetName() << " = ";
    } else {
      out << SymIR::GetTypeCName(l.GetType()) << " " << l.GetName() << " = ";
    }
    l.GetCoef()->Accept(*this);
    out << ";" << std::endl;
  }

  void SymCxLower::Visit(const VecLocal &l) {
    indent();
    if (l.IsVolatile()) {
      out << "volatile" << " ";
    };
    // For ARRAY type, use GetBaseType() to get the element type
    if (l.GetBaseType() == SymIR::STRUCT) {
      out << "struct " << l.GetStructName() << " " << l.GetName();
    } else {
      out << SymIR::GetTypeCName(l.GetBaseType()) << " " << l.GetName();
    }
    for (auto len: l.GetVecShape()) {
      out << "[" << len << "]";
    }
    out << " = {";
    const auto &cs = l.GetCoefs();
    // Coefs list is flattened scalars.
    // We need to group them if we want nice output, but C allows flat initialization for arrays of
    // structs? "struct P arr[2] = {1, 2, 3, 4};" works? Yes, generally works (braces elision). But
    // SymCxLower iterates `ncs`. `GetVecNumEls` counts array elements. If element is struct,
    // `GetCoefs` has `ncs * struct_size` entries. So `cs.size() != ncs` if struct! We should
    // iterate `cs.size()`.

    const size_t totalScalars = l.GetCoefs().size();
    for (size_t i = 0; i < totalScalars; ++i) {
      auto c = cs[i];
      c->Accept(*this);
      if (i != totalScalars - 1) {
        out << ", ";
      }
    }
    out << "};" << std::endl;
  }

  void SymCxLower::Visit(const StructLocal &l) {
    indent();
    out << "struct " << l.GetStructName() << " " << l.GetName() << " = {";
    const auto &cs = l.GetCoefs();
    for (size_t i = 0; i < cs.size(); ++i) {
      cs[i]->Accept(*this);
      if (i != cs.size() - 1)
        out << ", ";
    }
    out << "};" << std::endl;
  }

  void SymCxLower::Visit(const StructDef &s) {
    out << "struct " << s.GetName() << " {" << std::endl;
    for (const auto &field: s.GetFields()) {
      // indent();
      // For ARRAY type, use baseType to get the element type
      // For STRUCT type, use "struct <structName>"
      // For scalar types (I32), use the type directly
      if (field.baseType == SymIR::STRUCT) {
        out << "  struct " << field.structName << " " << field.name;
      } else {
        out << "  " << SymIR::GetTypeCName(field.baseType) << " " << field.name;
      }
      for (int d: field.shape)
        out << "[" << d << "]";
      out << ";" << std::endl;
    }
    out << "};" << std::endl;
  }

  void SymCxLower::Visit(const Block &b) {
    out << b.GetLabel() << ":" << std::endl;
    for (auto s: b.GetStmts()) {
      s->Accept(*this);
    }
  }

  void SymCxLower::Visit(const Funct &f) {
    curFunct = &f;
    if (emitStructs) {
      for (const auto &s: f.GetStructs()) {
        s->Accept(*this);
      }
    }
    out << SymIR::GetTypeCName(f.GetRetType()) << " " << f.GetName() << "(";
    auto params = f.GetParams();
    for (size_t i = 0; i < params.size(); ++i) {
      params[i]->Accept(*this);
      if (i != params.size() - 1) {
        out << ", ";
      }
    }
    out << ") {" << std::endl;
    for (const auto &b: f.GetLocals()) {
      incIndent();
      b->Accept(*this);
      decIndent();
    }
    for (const auto &b: f.GetBlocks()) {
      incIndent();
      b->Accept(*this);
      decIndent();
    }
    out << "}" << std::endl;
    curFunct = nullptr;
  }

  void SymJavaBytecodeLower::CreateArray(
      jnif::Method &met, const VarDef &var, const std::vector<Coef *> &inits
  ) {
    Assert(
        var.GetVecNumEls() == static_cast<int>(inits.size()),
        "The number of initialization coefficients (%d) does not match the number of "
        "elements (%d) for variable %s",
        static_cast<int>(inits.size()), var.GetVecNumEls(), var.GetName().c_str()
    );
    const std::function<void(int, int)> createArray = [&](const int dim, const int offset) {
      const auto numDims = var.GetVecNumDims();
      const auto dimLen = var.GetVecDimLen(dim);
      const auto dimNumEls = var.GetVecNumEls(dim);
      if (dim == numDims - 1) {
        Assert(dimNumEls == 1, "The last dimension should have exactly one element");
        // The very last dimension is an [I
        jnif::PushInteger(*clazz, met, dimLen);
        met.instList().addNewArray(jnif::model::NewArrayInst::NewArrayType::NEWARRAYTYPE_INT);
        // Fill each element with its initialization value
        if (!inits.empty()) {
          for (int i = 0; i < dimLen; i++) {
            met.instList().addZero(jnif::Opcode::dup);
            jnif::PushInteger(*clazz, met, i);
            inits[offset + i]->Accept(*this);
            met.instList().addZero(jnif::Opcode::iastore);
          }
        }
        return;
      }
      // The other dimensions are [[...[[I.
      jnif::PushInteger(*clazz, met, dimLen);
      met.instList().addType(
          jnif::Opcode::anewarray,
          clazz->putClass((std::string(numDims - dim - 1, '[') + "I").c_str())
      );
      // Fill each element with a sub-array
      for (int i = 0; i < dimLen; i++) {
        met.instList().addZero(jnif::Opcode::dup);
        jnif::PushInteger(*clazz, met, i);
        createArray(dim + 1, offset + i * dimNumEls);
        met.instList().addZero(jnif::Opcode::aastore);
      }
    };
    createArray(0, 0);
  }

  void SymJavaBytecodeLower::Visit(const VarUse &v) {
    switch (v.GetType()) {
      case SymIR::Type::I32:
        if (v.IsVector()) {
          method->instList().addVar(jnif::Opcode::aload, locals[v.GetName()]);
          for (int i = 0; i < v.GetVecNumDims() - 1; i++) {
            v.GetDimAccess(i)->Accept(*this);
            method->instList().addZero(jnif::Opcode::aaload);
          }
          v.GetDimAccess(v.GetVecNumDims() - 1)->Accept(*this);
          method->instList().addZero(jnif::Opcode::iaload);
        } else {
          method->instList().addVar(jnif::Opcode::iload, locals[v.GetName()]);
        }
        break;
      default:
        Panic("Unsupported variable type");
    }
  }

  void SymJavaBytecodeLower::Visit(const Coef &c) {
    Assert(c.IsSolved(), "The coefficient has not been solved, cannot lower to bytecode");
    switch (c.GetType()) {
      case SymIR::Type::I32: {
        jnif::PushInteger(*clazz, *method, c.GetI32Value());
        break;
      }
      default:
        Panic("Unsupported variable type");
    }
  }

  void SymJavaBytecodeLower::Visit(const Term &t) {
    t.GetCoef()->Accept(*this);
    if (t.GetVar() != nullptr) {
      t.GetVar()->Accept(*this);
    }
    switch (t.GetType()) {
      case SymIR::Type::I32: {
        switch (t.GetOp()) {
          case Term::Op::OP_CST:
            method->instList().addZero(jnif::Opcode::nop);
            break;
          case Term::Op::OP_ADD:
            method->instList().addZero(jnif::Opcode::iadd);
            break;
          case Term::Op::OP_SUB:
            method->instList().addZero(jnif::Opcode::isub);
            break;
          case Term::Op::OP_MUL:
            method->instList().addZero(jnif::Opcode::imul);
            break;
          case Term::Op::OP_DIV:
            method->instList().addZero(jnif::Opcode::idiv);
            break;
          case Term::Op::OP_REM:
            method->instList().addZero(jnif::Opcode::irem);
            break;
          default:
            Panic("Unsupported term type %s", Term::GetOpName(t.GetOp()).c_str());
        }
        break;
      }
      default:
        Panic("Unsupported term for type %s", SymIR::GetTypeName(t.GetType()).c_str());
    }
  }

  void SymJavaBytecodeLower::Visit(const ModExpr &e) {
    Panic("TODO: Implement java ModExpr");
  }

  void SymJavaBytecodeLower::Visit(const Expr &e) {
    if (e.GetType() != SymIR::I32) {
      Panic("Unsupported expression for type %s", SymIR::GetTypeName(e.GetType()).c_str());
    }
    int numTerms = static_cast<int>(e.GetTerms().size());
    if (numTerms > 0) {
      e.GetTerm(0)->Accept(*this);
    }
    for (int i = 1; i < numTerms; ++i) {
      e.GetTerm(i)->Accept(*this);
      switch (e.GetOp()) {
        case Expr::Op::OP_ADD:
          method->instList().addZero(jnif::Opcode::iadd);
          break;
        case Expr::Op::OP_SUB:
          method->instList().addZero(jnif::Opcode::isub);
          break;
        default:
          Panic("Unsupported expression type %s", Expr::GetOpName(e.GetOp()).c_str());
      }
    }
  }

  void SymJavaBytecodeLower::Visit(const Cond &c) {
    if (c.GetType() != SymIR::I32) {
      Panic("Unsupported condition for type %s", SymIR::GetTypeName(c.GetType()).c_str());
    }
    c.GetExpr()->Accept(*this);
  }

  void SymJavaBytecodeLower::Visit(const ModAssStmt &a) {
    Panic("TODO: Implement java ModAssStmt");
  }

  void SymJavaBytecodeLower::Visit(const AssStmt &a) {
    const auto v = a.GetVar();
    if (v->GetType() != SymIR::Type::I32) {
      Panic("Unsupported assignment for type %s", SymIR::GetTypeName(v->GetType()).c_str());
    }
    if (v->IsScalar()) {
      a.GetExpr()->Accept(*this);
      method->instList().addVar(jnif::Opcode::istore, locals[v->GetName()]);
    } else {
      method->instList().addVar(jnif::Opcode::aload, locals[v->GetName()]);
      for (int i = 0; i < v->GetVecNumDims() - 1; i++) {
        v->GetDimAccess(i)->Accept(*this);
        method->instList().addZero(jnif::Opcode::aaload);
      }
      v->GetDimAccess(v->GetVecNumDims() - 1)->Accept(*this);
      a.GetExpr()->Accept(*this);
      method->instList().addZero(jnif::Opcode::iastore);
    }
  }

  void SymJavaBytecodeLower::Visit(const RetStmt &r) {
    const auto chksumClassIndex = clazz->putClass(JavaStatelessChecksum::GetClassName().c_str());
    const auto chksumMethodIndex = clazz->addMethodRef(
        chksumClassIndex, JavaStatelessChecksum::GetComputeName().c_str(),
        JavaStatelessChecksum::GetComputeDesc().c_str()
    );

    const auto &args = r.GetVars();
    int numArgs = static_cast<int>(args.size());
    jnif::PushInteger(*clazz, *method, numArgs);
    // See https://docs.oracle.com/javase/specs/jvms/se8/html/jvms-6.html#jvms-6.5.newarray
    method->instList().addNewArray(jnif::model::NewArrayInst::NewArrayType::NEWARRAYTYPE_INT);
    // Fill the array with the variables that we need to return
    for (int i = 0; i < numArgs; i++) {
      const auto &a = args[i];
      // Duplicate the array to avoid frequent load-store and save a local
      method->instList().addZero(jnif::Opcode::dup);
      jnif::PushInteger(*clazz, *method, i);
      a->Accept(*this);
      method->instList().addZero(jnif::Opcode::iastore);
    }

    // Invoke the checksum method with the array of arguments
    method->instList().addInvoke(jnif::Opcode::invokestatic, chksumMethodIndex);

    // Return the result of the checksum method
    method->instList().addZero(jnif::Opcode::ireturn);
  }

  void SymJavaBytecodeLower::Visit(const Branch &b) {
    b.GetCond()->Accept(*this);
    const auto &c = *b.GetCond();
    switch (c.GetOp()) {
      case Cond::Op::OP_GTZ: {
        method->instList().addJump(jnif::Opcode::ifgt, labels[b.GetTrueTarget()]);
        break;
      }
      case Cond::OP_LTZ:
        method->instList().addJump(jnif::Opcode::iflt, labels[b.GetTrueTarget()]);
        break;
      case Cond::OP_EQZ:
        method->instList().addJump(jnif::Opcode::ifeq, labels[b.GetTrueTarget()]);
        break;
      default:
        Panic("Unsupported condition for type %s", SymIR::GetTypeName(c.GetType()).c_str());
    }
    method->instList().addJump(jnif::Opcode::GOTO, labels[b.GetFalseTarget()]);
  }

  void SymJavaBytecodeLower::Visit(const Goto &g) {
    method->instList().addJump(jnif::Opcode::GOTO, labels[g.GetTarget()]);
  }

  void SymJavaBytecodeLower::Visit(const ScaParam &p) {}

  void SymJavaBytecodeLower::Visit(const VecParam &p) {}

  void SymJavaBytecodeLower::Visit(const StructParam &p) {
    Panic("Structs are not supported in Java backend yet");
  }

  void SymJavaBytecodeLower::Visit(const UnInitLocal &l) {
    Panic("TODO: Implement java UnInitLocal");
  }

  void SymJavaBytecodeLower::Visit(const ScaLocal &l) {
    if (l.GetType() != SymIR::Type::I32) {
      Panic("Unsupported local variable type %s", SymIR::GetTypeName(l.GetType()).c_str());
    }
    l.GetCoef()->Accept(*this);
    method->instList().addVar(jnif::Opcode::istore, locals[l.GetName()]);
  }

  void SymJavaBytecodeLower::Visit(const VecLocal &l) {
    if (l.GetType() != SymIR::Type::I32) {
      Panic("Unsupported local variable type %s", SymIR::GetTypeName(l.GetType()).c_str());
    }
    CreateArray(*method, l, l.GetCoefs());
    method->instList().addVar(jnif::Opcode::astore, locals[l.GetName()]);
  }

  void SymJavaBytecodeLower::Visit(const StructLocal &l) {
    Panic("Structs are not supported in Java backend yet");
  }

  void SymJavaBytecodeLower::Visit(const StructDef &s) {
    Panic("Structs are not supported in Java backend yet");
  }

  void SymJavaBytecodeLower::Visit(const Block &b) {
    Assert(method != nullptr, "Method is not set for block visit");
    method->instList().addLabel(labels[b.GetLabel()]);
    for (const auto &s: b.GetStmts()) {
      s->Accept(*this);
    }
  }

  void SymJavaBytecodeLower::Visit(const Funct &f) {
    fun = &f;
    const std::string methodDesc = GetJavaMethodDesc();
    method = &clazz->addMethod(
        f.GetName().c_str(), methodDesc.c_str(), jnif::Method::Flags::PUBLIC | jnif::Method::STATIC
    );
    for (const auto &p: f.GetParams()) {
      locals[p->GetName()] = static_cast<int>(locals.size());
    }
    for (const auto &l: f.GetLocals()) {
      locals[l->GetName()] = static_cast<int>(locals.size());
    }
    for (const auto &b: f.GetBlocks()) {
      labels[b->GetLabel()] = method->instList().createLabel();
    }
    for (const auto &p: f.GetParams()) {
      p->Accept(*this);
    }
    for (const auto &l: f.GetLocals()) {
      l->Accept(*this);
    }
    for (const auto &b: f.GetBlocks()) {
      b->Accept(*this);
    }
  }
} // namespace symir
