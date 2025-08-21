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
#include "lib/bpf/bpf.h"
#include "lib/chksum.hpp"
#include "lib/logger.hpp"

namespace symir {
  std::ostream SymIRLower::devNull(nullptr);

  void SymSexpLower::Visit(const VarUse &v) { out << v.GetName(); }

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

  void SymSexpLower::Visit(const Param &p) {
    if (p.IsVolatile()) {
      out << "(" << KW_VOL << " " << KW_PAR << " " << p.GetName() << " "
          << SymIR::GetTypeSName(p.GetType()) << ")";
    } else {
      out << "(" << KW_PAR << " " << p.GetName() << " " << SymIR::GetTypeSName(p.GetType()) << ")";
    }
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

  void SymCxLower::Visit(const VarUse &v) { out << v.GetName(); }

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

  void SymCxLower::Visit(const Param &p) {
    if (p.IsVolatile()) {
      out << "volatile" << " " << SymIR::GetTypeCName(p.GetType()) << " " << p.GetName();
    } else {
      out << SymIR::GetTypeCName(p.GetType()) << " " << p.GetName();
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
        method->instList().addVar(jnif::Opcode::iload, locals[v.GetName()]);
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
    a.GetExpr()->Accept(*this);
    if (a.GetVar()->GetType() != SymIR::Type::I32) {
      Panic(
          "Unsupported assignment for type %s", SymIR::GetTypeName(a.GetVar()->GetType()).c_str()
      );
    }
    method->instList().addVar(jnif::Opcode::istore, locals[a.GetVar()->GetName()]);
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
      method->instList().addZero(
          jnif::Opcode::dup
      ); // Duplicate the array to avoid frequent load-store and save a local
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

  void SymJavaBytecodeLower::Visit(const Param &p) {}

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

/* C++ has the following strange warning, which is basically fp in C.
 * Suppress it in the eBPFLower code area.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"

  void eBPFLower::Visit(const VarUse &v) { Panic("Unreachable"); }

  void eBPFLower::Visit(const Coef &c) {
    Assert(c.GetType() == SymIR::Type::I32, "Unsupported coefficient type");
    Assert(c.IsValueSet(), "Coefficient value is not set");
  }

  void eBPFLower::Visit(const Term &t) {
    /* term := op coef var
     * This: stores coef to AX0, and get the reg of var
     * compute the result into AX0
     */

    t.GetCoef()->Accept(*this);
    int coef = std::stoi(t.GetCoef()->GetValue());
    insns.push_back(BPF_MOV32_IMM(REG_AX0, coef));

    u8 op;
    u16 off = 0;
    switch (t.GetOp()) {
      case Term::Op::OP_ADD:
        op = BPF_ADD;
        break;
      case Term::Op::OP_SUB:
        op = BPF_SUB;
        break;
      case Term::Op::OP_MUL:
        op = BPF_MUL;
        break;
      case Term::Op::OP_DIV:
        op = BPF_DIV;
        off = 1; // BPF_SDIV
        break;
      case Term::Op::OP_REM:
        op = BPF_MOD;
        off = 1; // BPF_SMOD
        break;
      case Term::Op::OP_CST:
        // Do nothing
        return;
      default:
        Panic("Unsupported term type");
    }
    if (t.GetVar() != nullptr) {
      auto insn = BPF_ALU32_REG(op, REG_AX0, GetReg(t.GetVar()));
      insn.off = off;
      insns.push_back(insn);
    }
  }

  void eBPFLower::Visit(const Expr &e) {
    /* expr := op term1 term2 ... termN
     * This is computed by:
     *   ax1 = term1
     *   for i = 2 to N:
     *     ax1 = op(ax1, termi)
     */

    e.GetTerm(0)->Accept(*this);
    insns.push_back(BPF_MOV32_REG(REG_AX1, REG_AX0));

    u8 op;
    switch (e.GetOp()) {
      case Expr::Op::OP_ADD:
        op = BPF_ADD;
        break;
      case Expr::Op::OP_SUB:
        op = BPF_SUB;
        break;
      default:
        Panic("Unsupported expression type");
    }

    for (size_t i = 1; i < e.GetTerms().size(); ++i) {
      e.GetTerm(i)->Accept(*this);
      insns.push_back(BPF_ALU32_REG(op, REG_AX1, REG_AX0));
    }
  }

  void eBPFLower::Visit(const Cond &c) { c.GetExpr()->Accept(*this); }

  void eBPFLower::Visit(const AssStmt &a) {
    u8 reg = GetReg(a.GetVar());
    a.GetExpr()->Accept(*this);
    insns.push_back(BPF_MOV32_REG(reg, REG_AX1));
  }

  void eBPFLower::Visit(const RetStmt &r) {
    /* csum = v1 ^ v2 ^ ... ^ vN
     * The below adds an oracle:
     *  if (csum == csum_computed_during_gen_exe)
     *      verifier_sink();
     * The sink is expected to be detected; otherwise, a false negative.
     */

    insns.push_back(BPF_MOV32_IMM(BPF_REG_0, 0));
    for (const auto &v: r.GetVars())
      insns.push_back(BPF_ALU32_REG(BPF_XOR, BPF_REG_0, GetReg(v)));
    insns.push_back(BPF_EXIT_INSN());
  }

  void eBPFLower::Visit(const Branch &b) {
    /* Jmp in this ir has two targets, while bpf only has one.
     * For `br cond l0 l1`, we translate it to:
     *    jmp cond l0
     *    jmp l1
     */

    b.GetCond()->Accept(*this);

    u8 op;
    switch (b.GetCond()->GetOp()) {
      case Cond::Op::OP_GTZ:
        op = BPF_JSGT;
        break;
      case Cond::Op::OP_LTZ:
        op = BPF_JSLT;
        break;
      case Cond::Op::OP_EQZ:
        op = BPF_JEQ;
        break;
      default:
        Panic("Unsupported condition type");
    }
    AddJmp(BPF_JMP32_IMM(op, REG_AX1, 0, 0), b.GetTrueTarget());
    AddJmp(BPF_JMP_A(0), b.GetFalseTarget());
  }

  void eBPFLower::Visit(const Goto &g) { AddJmp(BPF_JMP_A(0), g.GetTarget()); }

  void eBPFLower::Visit(const Param &p) { GetReg(&p, true); }

  void eBPFLower::Visit(const Local &l) {
    l.GetCoef()->Accept(*this);
    insns.push_back(BPF_MOV32_IMM(GetReg(&l), std::stoi(l.GetCoef()->GetValue())));
  }

  void eBPFLower::Visit(const Block &b) {
    u32 insn_cnt_before = insns.size();

    labels[b.GetLabel()] = insns.size();
    for (const auto &s: b.GetStmts()) {
      s->Accept(*this);
    }
    Assert(insn_cnt_before != insns.size(), "Empty block");
  }

  void eBPFLower::Visit(const Funct &f) {
    for (const auto &p: f.GetParams()) {
      p->Accept(*this);
    }

    for (const auto &l: f.GetLocals()) {
      l->Accept(*this);
    }

    for (const auto &b: f.GetBlocks()) {
      b->Accept(*this);
    }

    // Fix block jump offsets
    for (const auto &[insn_off, target]: jmp_fixups) {
      const auto br_off = labels.at(target);
      Assert(br_off != insn_off, "Dead jmp");
      int off = static_cast<int>(br_off) - static_cast<int>(insn_off);
      // eBPF jump offset is relative to the next instruction
      off -= 1;
      Assert(off >= INT16_MIN && off <= INT16_MAX, "Jump offset too large");
      insns[insn_off].off = off;
    }
    jmp_fixups.clear();
  }

#pragma GCC diagnostic pop


} // namespace symir
