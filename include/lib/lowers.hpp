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

#include <algorithm>
#include <fstream>
#include <random>
#include "jnif/jnif.hpp"
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
    void Visit(const ScaParam &p) override;
    void Visit(const VecParam &p) override;
    void Visit(const ScaLocal &l) override;
    void Visit(const VecLocal &l) override;
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
    void Visit(const ScaParam &p) override;
    void Visit(const VecParam &p) override;
    void Visit(const ScaLocal &l) override;
    void Visit(const VecLocal &l) override;
    void Visit(const Block &b) override;
    void Visit(const Funct &f) override;
  };

  /// An "SymIR -> Wasm" lower
  class SymWasmLower : public SymIRLower {
  public:
    struct Config {
      int ANON_DECL_PCT = 50;    // Chance that a variable is declared without a name (and later referred to by index)
      int ANON_USAGE_PCT = 50;   // Chance that a variable use refers to a variable by index instead of by name
      int HEAP_PCT = 20;         // Chance that a local variable is allocated on the heap and accessed via pointer instead of being a Wasm local
      int SEXP_PCT = 50;         // Chance that a construct is emitted in s-expression-style rather than line-by-line style
      int UNREACHABLE_PCT = 50;  // Chance that a block that is not on the execution path is emitted as an "unreachable" block
      int FOLDING_PCT = 50;      // Chance that a local definition is folded (and anonymized) with a random amount of neighboring locals
    };
    explicit SymWasmLower(std::ostream &out) : SymIRLower(out) {}
    void SymWasmLowerFunction(const Funct &f, const std::vector<std::string> execPath) {
      this->exec_path = execPath;
      Lower(f);
    }
    void SymWasmConfigure(Config cfg) {
      config = cfg;
    }

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
    void Visit(const ScaParam &p) override;
    void Visit(const VecParam &p) override;
    void Visit(const ScaLocal &l) override;
    void Visit(const VecLocal &l) override;
    void Visit(const Block &b) override;
    void Visit(const Funct &f) override;

  protected:
    std::string getWasmType(SymIR::Type type) {
      switch (type) {
        case SymIR::Type::I32:  return "i32";
        default: return "unknown";
      }
    }

  protected:
    bool force_sexp = false;                    // Whether current emission must be sexp style
    std::vector<const Block *> blocks;          // Blocks in order of appearance
    std::vector<std::string> exec_path;         // Execution path using block numbers
    std::map<int, int> block_inds{};            // Map from block label nums to dispatch indices
    std::map<const std::string, int> locals{};  // Map from variable names to local indices
    std::set<std::string> heap_locals;          // If variable in this set, must be allocated on heap and accessed via pointer
    std::set<std::string> is_anonymous;         // If variable in this set, must use index to refer to

  protected:
    Config config;  // Configuration for randomization in Wasm lowering, which can be set by the user for testing different styles of Wasm code

  protected:
    inline static const std::string WASM_DISPATCHER = "dispatcher";
    inline static const std::string WASM_NEXT_BLOCK = "next_block";
  };

  /// An "SymIR -> Java Bytecode" lower
  class SymJavaBytecodeLower : public SymIRLower {

  public:
    explicit SymJavaBytecodeLower(const std::string &className) :
        SymIRLower(devNull), clazz(std::make_unique<jnif::ClassFile>(className.c_str())) {}

    /// Get the generated Java class (the ownership is not transferred)
    [[nodiscard]] jnif::ClassFile *GetJavaClass() const { return clazz.get(); }

    /// Take the generated Java class, after which the lower cannot be used anymore
    [[nodiscard]] std::unique_ptr<jnif::ClassFile> TakeJavaClass() {
      Assert(fun != nullptr, "No function has been lowered yet, cannot take the generated class");
      Assert(
          method != nullptr, "No functions have been lowered yet, cannot take the generated class"
      );
      Assert(clazz != nullptr, "The class has already been taken");
      return std::move(clazz);
    }

    /// Lower the given function into Java bytecode
    void Lower(const Funct &f) override {
      Assert(fun == nullptr, "There has been a lowered function, cannot lower another function");
      Assert(method == nullptr, "There has been a lowered function, cannot another function");
      SymIRLower::Lower(f);
      clazz->computeFrames(nullptr);
    }

    /// Get the method that we have generated bytecode for
    [[nodiscard]] jnif::Method *GetJavaMethod() {
      Assert(fun != nullptr, "No function has been lowered yet, cannot obtain the method");
      Assert(method != nullptr, "No functions have been lowered yet,, cannot obtain the method");
      return method;
    }

    /// Get the method descriptor for the method we are generating bytecode for
    [[nodiscard]] std::string GetJavaMethodDesc() const {
      Assert(fun != nullptr, "No function is being lowered, cannot obtain the method desc");
      std::stringstream oss;
      oss << "(";
      for (const auto &p: fun->GetParams()) {
        if (p->IsScalar()) {
          oss << "I";
        } else {
          oss << std::string(p->GetVecNumDims(), '[') + "I";
        }
      }
      oss << ")I";
      return oss.str();
    }

    /// Get the method index in the constant pool
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

    /// Get the main method of the class
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

    /// Export the generated class to a .class file
    void Export(const std::string &file) const {
      // ReSharper disable once CppDFAUnusedValue
      jnif::stream::OClassFileStream s(file.c_str(), clazz.get());
    }

    /// Create a new array for the variable on stack and initialize it with inits
    void CreateArray(jnif::Method &met, const VarDef &var, const std::vector<Coef *> &inits = {});

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
    void Visit(const ScaParam &p) override;
    void Visit(const VecParam &p) override;
    void Visit(const ScaLocal &l) override;
    void Visit(const VecLocal &l) override;
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
    std::map<const std::string, jnif::LabelInst *> labels{
    }; // Map from block labels to bytecode labels
  };
} // namespace symir


#endif // REIFY_LOWERS_HPP
