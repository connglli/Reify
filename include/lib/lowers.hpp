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

#ifndef REIFY_LOWERS_HPP
#define REIFY_LOWERS_HPP

#include <fstream>
#include "jnif/jnif.hpp"
#include "lib/bpf/bpf.h"
#include "lib/lang.hpp"

#define SYMIR_LOWER_INDENTATION_SIZE 2

#define SYMIR_KEYWORD_LIST(XX)                                                                     \
  XX(FUN, fun)                                                                                     \
  XX(VOL, vol)                                                                                     \
  XX(PAR, par)                                                                                     \
  XX(LOC, loc)                                                                                     \
  XX(BBL, bbl)                                                                                     \
  XX(ASS, asn)                                                                                     \
  XX(RET, ret)                                                                                     \
  XX(BRH, brh)                                                                                     \
  XX(GOTO, goto)

namespace symir {
  using namespace symir;

  class SymIRLower : protected SymIRVisitor {
  public:
    static std::ostream devNull;

  public:
    explicit SymIRLower(std::ostream &out) : out(out) {}

    virtual void Lower(const Funct &f) { f.Accept(*this); }

  protected:
    [[nodiscard]] std::string getIndent() const {
      return std::string(numIndent * SYMIR_LOWER_INDENTATION_SIZE, ' ');
    }

    void indent() const { out << getIndent(); }

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
    static constexpr char KW_COEF_VAL_SYM = '#';

  public:
    explicit SymSexpLower(std::ostream &out) : SymIRLower(out) {}

  protected:
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
    void Visit(const Funct &f) override;
  };

  /// An "SymIR -> C/C++" lower
  class SymCxLower : public SymIRLower {
  public:
    explicit SymCxLower(std::ostream &out) : SymIRLower(out) {}

  protected:
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
    void Visit(const Funct &f) override;
  };

  /// An "SymIR -> Java Bytecode" lower
  class SymJavaBytecodeLower : public SymIRLower {

  public:
    explicit SymJavaBytecodeLower(const std::string &className) :
        SymIRLower(devNull), clazz(std::make_unique<jnif::ClassFile>(className.c_str())) {}

    [[nodiscard]] jnif::ClassFile *GetJavaClass() const { return clazz.get(); }

    [[nodiscard]] std::unique_ptr<jnif::ClassFile> TakeJavaClass() {
      Assert(fun != nullptr, "No function has been lowered yet, cannot take the generated class");
      Assert(
          method != nullptr, "No functions have been lowered yet, cannot take the generated class"
      );
      Assert(clazz != nullptr, "The class has already been taken");
      return std::move(clazz);
    }

    void Lower(const Funct &f) override {
      Assert(fun == nullptr, "There has been a lowered function, cannot lower another function");
      Assert(method == nullptr, "There has been a lowered function, cannot another function");
      SymIRLower::Lower(f);
      clazz->computeFrames(nullptr);
    }

    [[nodiscard]] jnif::Method *GetJavaMethod() {
      Assert(fun != nullptr, "No function has been lowered yet, cannot obtain the method");
      Assert(method != nullptr, "No functions have been lowered yet,, cannot obtain the method");
      return method;
    }

    [[nodiscard]] jnif::ConstPool::Index GetJavaMethodIndex() {
      Assert(fun != nullptr, "No function has been lowered yet, cannot obtain the method index");
      Assert(
          method != nullptr, "No functions have been lowered yet,, cannot obtain the method index"
      );
      if (methodIndex == jnif::ConstPool::NULLENTRY) {
        methodIndex = clazz->addMethodRef(
            clazz->thisClassIndex, clazz->addNameAndType(method->nameIndex, method->descIndex)
        );
      }
      return methodIndex;
    }

    [[nodiscard]] jnif::Method *GetJavaMainMethod() {
      Assert(fun != nullptr, "No function has been lowered yet, cannot get the main method");
      Assert(method != nullptr, "No functions have been lowered yet, cannot get the main method");
      if (main == nullptr) {
        main = &clazz->addMethod(
            "main", "([Ljava/lang/String;)V",
            jnif::Method::Flags::PUBLIC | jnif::Method::Flags::STATIC
        );
      }
      return main;
    }

    void Export(const std::string &file) const {
      jnif::stream::OClassFileStream s(file.c_str(), clazz.get());
    }

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

  protected:
    std::unique_ptr<jnif::ClassFile> clazz;
    jnif::Method *main = nullptr;   // The main method of the class
    jnif::Method *method = nullptr; // The method that we're to generate bytecode for
    jnif::ConstPool::Index methodIndex =
        jnif::ConstPool::NULLENTRY;            // The method index in the constant pool
    const Funct *fun = nullptr;                // The function that we're currently lowering
    std::map<const std::string, int> locals{}; // Map from variable names to local variable indices
    std::map<const std::string, jnif::LabelInst *>
        labels{}; // Map from block labels to bytecode labels
  };

  /// Translates SymIR to eBPF bytecode with register allocation, arithmetic operations,
  /// control flow handling, and oracle-based verification for bug detection.
  class eBPFLower : public SymIRLower {
  public:
    explicit eBPFLower(std::vector<struct bpf_insn> &insns) : SymIRLower(devNull), insns(insns) {}

  protected:
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
    void Visit(const Funct &f) override;

  private:
    using u32 = std::uint32_t;
    using u16 = std::uint16_t;
    using u8 = std::uint8_t;

    /* BPF has 11 regs, R0~R10:
     *   R0: return reg
     *   R1: ctx pointer
     *   R2~R5: other function parameters
     *   R6~R9: locals
     *   R10: stack pointer
     * In this translation, we use:
     *   R2~R5: params
     *   R6~R7: locals
     *   R8: tmp reg (for the lower)
     *   R9: tmp reg
     * So 6 regs are available (MAX_REG), AX0 and AX1 are tmps.
     */
    static const u8 MAX_REG = 6;
    static const u8 REG_AX0 = BPF_REG_8; // for imm result of term
    static const u8 REG_AX1 = BPF_REG_9; // for expr

    template<typename T>
    u8 GetReg(const T *t, bool param = false) {
      switch (t->GetType()) {
        case SymIR::Type::I32: {
          const auto name = t->GetName();
          if (regs.find(name) == regs.end()) {
            Assert(regs.size() < MAX_REG, "Too many variables");
            u8 reg_n;
            if (param) {
              reg_n = preg_gen++;
              Assert(reg_n <= BPF_REG_5, "Too many parameters");
            } else {
              reg_n = lreg_gen++;
              Assert(reg_n <= BPF_REG_7, "Too many locals");
            }
            regs[name] = reg_n;
          }
          return regs[name];
        }
        default: {
          Panic("Unsupported var type");
        }
      }
    }

    void AddJmp(struct bpf_insn insn, const std::string &target) {
      jmp_fixups[static_cast<u32>(insns.size())] = target;
      insns.push_back(insn);
    }

    std::vector<struct bpf_insn> &insns;
    std::unordered_map<std::string, u8> regs;
    u8 preg_gen = BPF_REG_2; // Next param reg, starting from R2
    u8 lreg_gen = BPF_REG_6; // Next local reg, starting from R6
    std::unordered_map<std::string, u32> labels;
    std::unordered_map<u32, std::string> jmp_fixups;
  };


} // namespace symir


#endif // REIFY_LOWERS_HPP
