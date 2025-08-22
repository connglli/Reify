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
#include "lib/logger.hpp"

namespace symir {
  std::ostream SymIRLower::devNull(nullptr);

  void SymSexpLower::Visit(const VarUse &v) {
    out << v.GetName();
    if (v.IsVector()) {
      for (const auto &c: v.GetAccess()) {
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
    out << KW_PAR << " " << p.GetName();
    for (const auto &l: p.GetVecDims()) {
      out << "[" << l << "]";
    }
    out << " " << SymIR::GetTypeSName(p.GetType()) << ")";
  }

  void SymSexpLower::Visit(const Local &l) {
    indent();
    if (l.IsVolatile()) {
      out << "(" << KW_VOL << " " << KW_LOC << " " << l.GetName() << " ";
    } else {
      out << "(" << KW_LOC << " " << l.GetName() << " ";
    }
    l.GetCoef()->Accept(*this);
    out << " " << SymIR::GetTypeSName(l.GetType()) << ")" << std::endl;
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
  }

  void SymCxLower::Visit(const VarUse &v) {
    out << v.GetName();
    if (v.IsVector()) {
      for (const auto &c: v.GetAccess()) {
        out << "[";
        c->Accept(*this);
        out << "]";
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

  void SymCxLower::Visit(const AssStmt &a) {
    indent();
    a.GetVar()->Accept(*this);
    out << " = ";
    a.GetExpr()->Accept(*this);
    out << ";" << std::endl;
  }

  void SymCxLower::Visit(const RetStmt &r) {
    indent();
    auto vars = r.GetVars();
    out << "return " << StatelessChecksum::GetComputeName() << "(" << vars.size() << ", (int["
        << vars.size() << "]){";
    for (size_t i = 0; i < vars.size(); ++i) {
      vars[i]->Accept(*this);
      if (i != vars.size() - 1) {
        out << ", ";
      }
    }
    out << "});" << std::endl;
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
    out << SymIR::GetTypeCName(p.GetType()) << " " << p.GetName();
    for (const auto &l: p.GetVecDims()) {
      out << "[" << l << "]";
    }
  }

  void SymCxLower::Visit(const Local &l) {
    indent();
    if (l.IsVolatile()) {
      out << "volatile" << " " << SymIR::GetTypeCName(l.GetType()) << " " << l.GetName() << " = ";
    } else {
      out << SymIR::GetTypeCName(l.GetType()) << " " << l.GetName() << " = ";
    }
    l.GetCoef()->Accept(*this);
    out << ";" << std::endl;
  }

  void SymCxLower::Visit(const Block &b) {
    out << b.GetLabel() << ":" << std::endl;
    for (auto s: b.GetStmts()) {
      s->Accept(*this);
    }
  }

  void SymCxLower::Visit(const Funct &f) {
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
        const auto value = std::stoi(c.GetValue());
        const auto index = clazz->addInteger(value);
        method->instList().addLdc(jnif::Opcode::ldc, index);
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

  void SymJavaBytecodeLower::Visit(const AssStmt &a) {
    const auto v = a.GetVar();
    if (v->GetType() != SymIR::Type::I32) {
      Panic("Unsupported assignment for type %s", SymIR::GetTypeName(v->GetType()).c_str());
    }
    if (!v->IsVector()) {
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
    method->instList().addLdc(jnif::Opcode::ldc, clazz->addInteger(numArgs));
    // See https://docs.oracle.com/javase/specs/jvms/se8/html/jvms-6.html#jvms-6.5.newarray
    method->instList().addNewArray(jnif::model::NewArrayInst::NewArrayType::NEWARRAYTYPE_INT);
    // Fill the array with the variables that we need to return
    for (int i = 0; i < numArgs; i++) {
      const auto &a = args[i];
      if (a->GetType() != SymIR::Type::I32) {
        Panic("Unsupported return variable type %s", SymIR::GetTypeName(a->GetType()).c_str());
      }
      if (a->IsVector()) {
        Panic("Returning vector variables is not supported");
      }
      // Duplicate the array to avoid frequent load-store and save a local
      method->instList().addZero(jnif::Opcode::dup);
      switch (i) {
        case 0:
          method->instList().addZero(jnif::Opcode::iconst_0);
          break;
        case 1:
          method->instList().addZero(jnif::Opcode::iconst_1);
          break;
        case 2:
          method->instList().addZero(jnif::Opcode::iconst_2);
          break;
        case 3:
          method->instList().addZero(jnif::Opcode::iconst_3);
          break;
        case 4:
          method->instList().addZero(jnif::Opcode::iconst_4);
          break;
        case 5:
          method->instList().addZero(jnif::Opcode::iconst_5);
          break;
        default:
          method->instList().addLdc(jnif::Opcode::ldc, clazz->addInteger(i));
          break;
      }
      method->instList().addVar(jnif::Opcode::iload, locals[a->GetName()]);
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

  void SymJavaBytecodeLower::Visit(const Local &l) {
    if (l.GetType() != SymIR::Type::I32) {
      Panic("Unsupported local variable type %s", SymIR::GetTypeName(l.GetType()).c_str());
    }
    l.GetCoef()->Accept(*this);
    method->instList().addVar(jnif::Opcode::istore, locals[l.GetName()]);
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
    const int numParams = static_cast<int>(f.GetParams().size());
    const std::string methodDesc = "(" + std::string(numParams, 'I') + ")I";
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
