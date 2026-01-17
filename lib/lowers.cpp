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
    for (const auto &l: p.GetVecShape()) {
      out << "[" << l << "]";
    }
    out << " " << SymIR::GetTypeSName(p.GetType()) << ")";
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
    out << KW_LOC << " " << l.GetName();
    for (auto len: l.GetVecShape()) {
      out << "[" << len << "]";
    }
    out << " ";
    for (auto c: l.GetCoefs()) {
      c->Accept(*this);
      out << " ";
    }
    out << SymIR::GetTypeSName(l.GetType()) << ")" << std::endl;
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
    for (const auto &l: p.GetVecShape()) {
      out << "[" << l << "]";
    }
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
    out << SymIR::GetTypeCName(l.GetType()) << " " << l.GetName();
    for (auto len: l.GetVecShape()) {
      out << "[" << len << "]";
    }
    out << " = {";
    const auto &cs = l.GetCoefs();
    const auto ncs = l.GetVecNumEls();
    for (int i = 0; i < ncs; ++i) {
      auto c = cs[i];
      c->Accept(*this);
      if (i != ncs - 1) {
        out << ", ";
      }
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

  void SymWasmLower::Visit(const VarUse &v) {
    // option 1: get by name
    std::string var_name = v.GetName();
    bool use_index = is_anonymous.count(var_name) || (rand() % 2 == 0);
    // option 2: get by index (0-based, first params, then locals)
    std::string var_id = use_index ? std::to_string(locals[var_name]) : ("$" + var_name);
    bool use_sexp = force_sexp || (rand() % 2 == 0);

    if (use_sexp) out << "(";
    out << "local.get " << var_id;
    if (use_sexp) out << ")";
  }

  void SymWasmLower::Visit(const Coef &c) {
    // must be solved to emit value
    Assert(c.IsSolved(), "The coefficient has not been solved, cannot lower to Wasm");
    
    std::string type;
    switch (c.GetType()) {
      case SymIR::Type::I32: type = "i32"; break;
      default: Panic("Unsupported variable type");
    }

    // TODO: will currently only work for i32
    int32_t value = c.GetI32Value();
    bool use_sexp = force_sexp || (rand() % 2 == 0);

    // option 1: c
    if (use_sexp) out << "(";
    out << type << ".const " << value;
    if (use_sexp) out << ")";

    // option 2: ((c + R) - R)  // TODO: make this also vary between sexp/stackbased
    // note: should keep simple in order to replace with function call?
  }

  void SymWasmLower::Visit(const Term &t) { 
    if (t.GetType() != SymIR::Type::I32) {
      Panic("Unsupported term type");
    }

    if (t.GetOp() == Term::Op::OP_CST) {
      // just coef
      t.GetCoef()->Accept(*this);
      return;
    }

    std::string op;
    switch (t.GetOp()) {
      case Term::Op::OP_ADD: op = "i32.add"; break;
      case Term::Op::OP_SUB: op = "i32.sub"; break;
      case Term::Op::OP_MUL: op = "i32.mul"; break;
      case Term::Op::OP_DIV: op = "i32.div_s"; break;
      case Term::Op::OP_REM: op = "i32.rem_s"; break;
      default: Panic("Unsupported operation");
    }

    bool use_sexp = force_sexp || (rand() % 2 == 0);

    if (use_sexp) {
      bool old_force = force_sexp;
      force_sexp = true;
      
      out << "(" << op;
      if (t.GetCoef()) {
        out << " ";
        t.GetCoef()->Accept(*this);
      }
      if (t.GetVar()) {
        out << " ";
        t.GetVar()->Accept(*this);
      }
      out << ")";
      
      force_sexp = old_force;
    } else {
      if (t.GetCoef()) {
        t.GetCoef()->Accept(*this);
        out << std::endl; indent();
      }
      if (t.GetVar()) { 
        t.GetVar()->Accept(*this);
        out << std::endl; indent();
      }
      out << op;
    }
  }

  void SymWasmLower::Visit(const Expr &e) {
    if (e.GetType() != SymIR::I32) {
      Panic("Unsupported expression type");
    }
    
    const auto& terms = e.GetTerms();
    if (terms.empty()) {
      // using 0 as a default  // TODO: will this ever occur? should maybe error instead?
      out << (force_sexp ? "(i32.const 0)" : "i32.const 0");
      return;
    }
    
    if (terms.size() == 1) {
      terms[0]->Accept(*this);
      return;
    }

    std::string op;
    switch(e.GetOp()) {
      case Expr::Op::OP_ADD: op = "i32.add"; break;
      case Expr::Op::OP_SUB: op = "i32.sub"; break;
      default: Panic("Unsupported expression operation");
    }

    bool use_sexp = force_sexp || (rand() % 2 == 0);

    if (use_sexp) {
      bool old_force = force_sexp;
      force_sexp = true; 

      // for left-assoc, have to emit (op (op (op 1 2) 3) 4)
      for (size_t i = 0; i < terms.size() - 1; ++i) {
        out << "(" << op << " ";
      }
      
      terms[0]->Accept(*this); // first/core term
      
      for (size_t i = 1; i < terms.size(); ++i) {
        out << " ";
        terms[i]->Accept(*this);
        out << ")";
      }

      force_sexp = old_force;
    } else {
      // stackbased is naturally left-assoc in order
      terms[0]->Accept(*this);
      for (size_t i = 1; i < terms.size(); ++i) {
        out << std::endl; indent();
        terms[i]->Accept(*this);
        out << std::endl; indent();
        out << op;
      }
    }
  }

  void SymWasmLower::Visit(const Cond &c) {
    if (c.GetType() != SymIR::I32) {
      Panic("Unsupported condition type");
    }

    bool use_sexp = force_sexp || (rand() % 2 == 0);
    Cond::Op op = c.GetOp();

    if (use_sexp) {
      bool old_force = force_sexp;
      force_sexp = true;

      if (op == Cond::Op::OP_EQZ) {
        out << "(i32.eqz ";
        c.GetExpr()->Accept(*this);
        out << ")";
      } else {
        // represent gtz and ltz by comparing with 0 (pushed const)
        std::string op_str = (op == Cond::Op::OP_GTZ) ? "i32.gt_s" : "i32.lt_s";
        out << "(" << op_str << " ";
        c.GetExpr()->Accept(*this);
        out << " (i32.const 0))";
      }

      force_sexp = old_force;
    } else {
      c.GetExpr()->Accept(*this);
      out << std::endl; indent();

      if (op == Cond::Op::OP_EQZ) {
        out << "i32.eqz";
      } else {
        // represent gtz and ltz by comparing with 0 (pushed const)
        out << "i32.const 0";
        out << std::endl; indent();
        out << ((op == Cond::Op::OP_GTZ) ? "i32.gt_s" : "i32.lt_s");
      }
    }
  }

  void SymWasmLower::Visit(const AssStmt &e) {
    std::string var_name = e.GetVar()->GetName();
    bool use_index = is_anonymous.count(var_name) || (rand() % 2 == 0);
    std::string var_id = use_index ? std::to_string(locals[var_name]) : ("$" + var_name);
    
    bool use_sexp = force_sexp || (rand() % 2 == 0);

    if (use_sexp) {
      bool old_force = force_sexp;
      force_sexp = true;

      out << "(local.set " << var_id << " ";
      e.GetExpr()->Accept(*this);
      out << ")";

      force_sexp = old_force;
    } else {
      e.GetExpr()->Accept(*this);
      out << std::endl; indent();
      out << "local.set " << var_id;
    }
  }

  // note: returning all vars with multi-return
  void SymWasmLower::Visit(const RetStmt &r) {
    bool use_sexp = force_sexp || (rand() % 2 == 0);

    if (use_sexp) {
      bool old_force = force_sexp;
      force_sexp = true;

      out << "(return";
      for (auto &v : r.GetVars()) {
        out << " ";
        v->Accept(*this);
      }
      out << ")";
      
      force_sexp = old_force;
    } else {
      for (auto &v : r.GetVars()) {
        v->Accept(*this);
        out << std::endl; indent();
      }
      out << "return";
    }
  }

  void SymWasmLower::Visit(const Branch &b) {
    int true_id = block_inds[std::stoi(b.GetTrueTarget().substr(2))];
    int false_id = block_inds[std::stoi(b.GetFalseTarget().substr(2))];
    bool use_sexp = force_sexp || (rand() % 2 == 0);
    std::string next_block_id = is_anonymous.count(WASM_NEXT_BLOCK) ? std::to_string(locals[WASM_NEXT_BLOCK]) : ("$" + WASM_NEXT_BLOCK);
    
    if (use_sexp) {
      bool old_force = force_sexp;
      out << "(local.set " << next_block_id << " ";
      force_sexp = true; 
      
      out << "(if (result i32) ";
      b.GetCond()->Accept(*this);
      out << " (then (i32.const " << true_id << "))";
      out << " (else (i32.const " << false_id << "))";
      out << "))";
      
      force_sexp = old_force;
    } else {
      // cond, if, true_block, else, false_block, end, set next_block
      b.GetCond()->Accept(*this); 
      out << std::endl; indent();

      out << "if (result i32)" << std::endl;
      incIndent(); indent();
      out << "i32.const " << true_id << std::endl;
      decIndent(); indent();
      
      out << "else" << std::endl;
      incIndent(); indent();
      out << "i32.const " << false_id << std::endl;
      decIndent(); indent();

      out << "end" << std::endl;
      indent();

      out << "local.set " << next_block_id;
    }

    // randomize again for br
    use_sexp = force_sexp || (rand() % 2 == 0);

    out << std::endl; indent();
    if (use_sexp) out << "(";
    out << "br $" << WASM_DISPATCHER;
    if (use_sexp) out << ")";
  }

  void SymWasmLower::Visit(const Goto &g) {
    int target_id = block_inds[std::stoi(g.GetTarget().substr(2))];
    bool use_sexp = force_sexp || (rand() % 2 == 0);
    std::string next_block_id = is_anonymous.count(WASM_NEXT_BLOCK) ? std::to_string(locals[WASM_NEXT_BLOCK]) : ("$" + WASM_NEXT_BLOCK);

    if (use_sexp) {
      out << "(local.set " << next_block_id << " ";
      out << "(i32.const " << target_id << ")"; 
      out << ")";
    } else {
      out << "i32.const " << target_id << std::endl;
      indent();
      out << "local.set " << next_block_id;
    }

    // randomize again for br
    use_sexp = force_sexp || (rand() % 2 == 0);

    out << std::endl; indent();
    if (use_sexp) out << "(";
    out << "br $" << WASM_DISPATCHER;
    if (use_sexp) out << ")";
  }

  // unused
  void SymWasmLower::Visit(const ScaParam &p) {
    out << "(param $" << p.GetName() << " ";
      switch (p.GetType()) {
        case SymIR::Type::I32: out << "i32"; break;
        default: Panic("Unsupported parameter type %s", SymIR::GetTypeName(p.GetType()).c_str());
      }
    out << ")";
  }

  // unused
  void SymWasmLower::Visit(const VecParam &p) {
    out << "(param $" << p.GetName() << " v128)";
  }

  // unused
  void SymWasmLower::Visit(const ScaLocal &l) {
    out << "(local $" << l.GetName() << " ";
      switch (l.GetType()) {
        case SymIR::Type::I32: out << "i32"; break;
        default: Panic("Unsupported local variable type %s", SymIR::GetTypeName(l.GetType()).c_str());
      }
    out << ")";
  }

  // unused
  void SymWasmLower::Visit(const VecLocal &l) {
    out << "(local $" << l.GetName() << " v128)";
  }

  void SymWasmLower::Visit(const Block &b) {
    std::string label = b.GetLabel();
    bool gen_unreachable = std::find(exec_path.begin(), exec_path.end(), label) == exec_path.end() && (rand() % 2 == 0);
    
    if (gen_unreachable) {
      indent();
      if (force_sexp) out << "(";
      out << "unreachable";
      if (force_sexp) out << ")";
      out << std::endl;
    } else {
      // visit all instrs inside the block
      for (const auto &s: b.GetStmts()) {
        indent();
        s->Accept(*this);
        out << std::endl;
      }
    }
  }

  void SymWasmLower::Visit(const Funct &f) {
    incIndent();  // should by default be 1
    
    // initialize blocks and order
    for (const auto &b : f.GetBlocks()) {
      blocks.push_back(b);
    }

    // initialize in-scope-variable name : idx map
    int idx = 0;
    for (const auto &p : f.GetParams()) {
      locals[p->GetName()] = idx++;
    }
    locals[WASM_NEXT_BLOCK] = idx++;
    for (const auto &l : f.GetLocals()) {
      locals[l->GetName()] = idx++;
    }

    indent(); out << "(func $" << f.GetName() << " ";

    // loop directly here instead of visiting to allow folding/anonymization
    const auto& params = f.GetParams();
    size_t i = 0;
    while (i < params.size()) {
      // how many params are left
      int remaining = params.size() - i;
      // can only fold if there are 2+ remaining
      bool can_group = (remaining >= 2);
      bool group = can_group && (rand() % 2 == 0);

      if (group) {
        out << "(param";
        // random group size between 2 and 'remaining'
        int group_size = (rand() % (remaining - 1)) + 2; 
        for (int j = 0; j < group_size; ++j) {
          out << " " << getWasmType(params[i]->GetType());
          // update is_anonymous
          is_anonymous.insert(params[i]->GetName());
          i++;
        }
        out << ") ";
      } else {
        // named, ungrouped/single
        out << "(param $" << params[i]->GetName() << " " << getWasmType(params[i]->GetType()) << ") ";
        // update is_anonymous
        is_anonymous.erase(params[i]->GetName());
        i++;
      }
    }

    out << "(result";
    switch (f.GetRetType()) {
      case SymIR::Type::I32:
        // just return all params final state  // TODO: will need to modify with v128 addition
        for (size_t i = 0; i < f.GetParams().size(); i++) {
          out << " i32";
        }
        break;
      default: Panic("Unsupported return type %s in method %s", SymIR::GetTypeName(f.GetRetType()).c_str(), f.GetName().c_str());
    }

    out << ")" << std::endl;

    incIndent(); indent();

    // can just shuffle block order, since doesn't affect dispatcher pattern
    // IDEA: toggling force_sexp from the beginning can enable 'pretty print'
    if (!force_sexp) {
      std::random_device rd;
      std::mt19937 r(rd());
      std::shuffle(blocks.begin(), blocks.end(), r);
    }

    // initialize block_inds
    for (size_t i = 0; i < blocks.size(); i++) {
      block_inds[std::stoi(blocks[i]->GetLabel().substr(2))] = i;
    }

    // emit next_block, no folding but possible anonymization
    if (rand() % 2 == 0) {
      // anon
      out << "(local i32)" << std::endl;
      indent();
      is_anonymous.insert(WASM_NEXT_BLOCK);
    } else {
      // named
      out << "(local $" << WASM_NEXT_BLOCK << " i32)" << std::endl;
      indent();
      is_anonymous.erase(WASM_NEXT_BLOCK);
    }

    // loop directly here instead of visiting to allow folding/anonymization
    i = 0;
    const auto& ir_locals = f.GetLocals();
    while (i < ir_locals.size()) {
      int remaining = ir_locals.size() - i;
      bool can_group = (remaining >= 2);
      bool group = can_group && (rand() % 2 == 0);

      if (group) {
        out << "(local";
        int group_size = (rand() % (remaining - 1)) + 2; 
        for (int j = 0; j < group_size; ++j) {
          out << " " << getWasmType(ir_locals[i]->GetType());
          is_anonymous.insert(ir_locals[i]->GetName());
          i++;
        }
        out << ")" << std::endl; indent();
      } else {
        out << "(local $" << ir_locals[i]->GetName() << " " << getWasmType(ir_locals[i]->GetType()) << ")";
        out << std::endl; indent();
        is_anonymous.erase(ir_locals[i]->GetName());
        i++;
      }
    }

    if (block_inds[0] != 0) {
      out << std::endl; indent();
      // set nextblock to bb0idx if not 0
      std::string next_block_id = is_anonymous.count(WASM_NEXT_BLOCK) ? std::to_string(locals[WASM_NEXT_BLOCK]) : ("$" + WASM_NEXT_BLOCK);
      out << "(local.set " << next_block_id << " (i32.const " << block_inds[0] << "))" << std::endl;
      indent();
    }
    // init values of locals (have to actually just set them separately as they are auto init to 0)
    for (const auto &l : f.GetLocals()) {
      if (auto sca = dynamic_cast<const ScaLocal*>(l)) {
        int coef = sca->GetCoef()->GetI32Value();
        if (coef != 0) {
          // TODO: create an equivalent assn stmt and visit?
          std::string var_name = l->GetName();
          bool use_index = is_anonymous.count(var_name) || (rand() % 2 == 0);
          std::string var_id = use_index ? std::to_string(locals[var_name]) : ("$" + var_name);
          out << "(local.set " << var_id << " (i32.const " << coef << "))" << std::endl;
          indent();
        }
      }
      // TODO: veclocal
    }

    if (!blocks.empty()) {
      out << std::endl;

      // dispatcher header
      indent();
      out << "(loop $" << WASM_DISPATCHER << std::endl;;
      incIndent(); indent();

      for (int i = blocks.size() - 1; i >= 0; i--) {
        out << "(block $" << blocks[i]->GetLabel() << " ";
      }
      out << std::endl;

      // dispatcher body
      incIndent(); indent();
      std::string next_block_id = is_anonymous.count(WASM_NEXT_BLOCK) ? std::to_string(locals[WASM_NEXT_BLOCK]) : ("$" + WASM_NEXT_BLOCK);
      if (force_sexp || rand() % 2 == 0) {
        out << "(br_table";
        for (const auto &b : blocks) {
          out << " $" << b->GetLabel();
        }
        out << " (local.get " << next_block_id << "))" << std::endl;
        decIndent();
      } else {
        out << "local.get " << next_block_id << std::endl;
        indent();
        out << "br_table";
        for (const auto &b : blocks) {
          out << " $" << b->GetLabel();
        }
        out << std::endl;
        decIndent();
      }

      // block bodies
      for (const auto &b : blocks) {
        indent();
        out << ")" << std::endl;
        incIndent(); indent();
        out << ";; -- block " << b->GetLabel() << " --" << std::endl;
        b->Accept(*this);
        decIndent();
      }
    }

    // dispatcher close paren
    decIndent(); indent();
    out << ")" << std::endl;
    indent();
    out << "(return";
    for (size_t i = 0; i < f.GetParams().size(); i++) {
      out << " (i32.const 0)";  // required return after all blocks, just using 0 as default
    }

    out << ")" << std::endl;
    decIndent(); indent();
    out << ")" << std::endl;

    // (export "function_name" (func $function_name))
    out << std::endl; indent();
    out << "(export \"" << f.GetName() << "\" (func $" << f.GetName() << "))" << std::endl;
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
