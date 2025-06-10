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

#include "lib/lang.hpp"

namespace symir {
  BlockBuilder::TermID
  BlockBuilder::SymTerm(Term::Op op, const std::string &coef, const VarDef *var) {
    Assert(isActive(), "BlockBuilder has already finished its work");
    TermID tid = numCreatedTerms++;
    createdTerms[tid] = std::make_unique<Term>(
        op, std::make_unique<Coef>(coef, var->GetType()), std::make_unique<VarUse>(var)
    );
    return tid;
  }

  BlockBuilder::TermID BlockBuilder::SymTerm(
      Term::Op op, const std::string &coefName, const std::string &coefVal, const VarDef *var
  ) {
    Assert(isActive(), "BlockBuilder has already finished its work");
    TermID tid = numCreatedTerms++;
    createdTerms[tid] = std::make_unique<Term>(
        op, std::make_unique<Coef>(coefName, coefVal, var->GetType()), std::make_unique<VarUse>(var)
    );
    return tid;
  }

  BlockBuilder::ExprID BlockBuilder::SymExpr(Expr::Op op, const std::vector<TermID> &termIds) {
    Assert(isActive(), "BlockBuilder has already finished its work");
    ExprID eid = numCreatedExprs++;
    std::vector<std::unique_ptr<Term>> terms;
    for (const auto tid: termIds) {
      auto it = createdTerms.find(tid);
      Assert(it != createdTerms.end(), "Term with ID=%lu does not exist", tid);
      terms.push_back(std::move(it->second));
      createdTerms.erase(it);
    }
    createdExprs[eid] = std::make_unique<Expr>(op, std::move(terms));
    return eid;
  }

  BlockBuilder::ExprID BlockBuilder::SymCond(Cond::Op op, ExprID eid) {
    Assert(isActive(), "BlockBuilder has already finished its work");
    CondID cid = numCreatedConds++;
    auto it = createdExprs.find(eid);
    Assert(it != createdExprs.end(), "Expr with ID=%lu does not exist", eid);
    createdConds[cid] = std::make_unique<Cond>(op, std::move(it->second));
    createdExprs.erase(it);
    return cid;
  }

  void BlockBuilder::SymAssign(const VarDef *var, ExprID eid) {
    Assert(isActive(), "BlockBuilder has already finished its work");
    auto it = createdExprs.find(eid);
    Assert(it != createdExprs.end(), "Expr with ID=%lu does not exist", eid);
    stmts.push_back(std::make_unique<AssStmt>(std::make_unique<VarUse>(var), std::move(it->second))
    );
    createdExprs.erase(it);
  }

  void BlockBuilder::SymReturn() {
    Assert(isActive(), "BlockBuilder has already finished its work");
    std::vector<std::unique_ptr<VarUse>> uses;
    for (const auto *p: GetParent()->GetParams()) {
      uses.push_back(std::make_unique<VarUse>(p));
    }
    stmts.push_back(std::make_unique<RetStmt>(std::move(uses)));
  }

  void BlockBuilder::SymBranch(const std::string &truLab, const std::string &falLab, CondID cid) {
    Assert(isActive(), "BlockBuilder has already finished its work");
    auto it = createdConds.find(cid);
    Assert(it != createdConds.end(), "Cond with ID=%lu does not exist", cid);
    target = std::make_unique<Branch>(std::move(it->second), truLab, falLab);
    createdConds.erase(it);
    deactivate();
  }

  void BlockBuilder::SymGoto(const std::string &label) {
    Assert(isActive(), "BlockBuilder has already finished its work");
    target = std::make_unique<Goto>(label);
    deactivate();
  }

  void BlockBuilder::SymFallThro() {
    Assert(isActive(), "BlockBuilder has already finished its work");
    deactivate();
  }

  std::unique_ptr<Block> BlockBuilder::Build() {
    Assert(!isActive(), "BlockBuilder has not yet finished its work");
    return std::make_unique<Block>(label, std::move(stmts), std::move(target));
  }

  const Param *FuncBuilder::SymParam(const std::string &name, SymIR::Type type) {
    Assert(isActive(), "FuncBuilder has already finished its work");
    Assert(
        !paramMap.contains(name), "A parameter with the same name is already defined: %s",
        name.c_str()
    );
    Assert(
        !localMap.contains(name), "A local with the same name is already defined: %s", name.c_str()
    );
    params.push_back(std::make_unique<Param>(name, type));
    auto v = params.back().get();
    paramMap[name] = v;
    return v;
  }

  const Local *
  FuncBuilder::SymLocal(const std::string &name, const std::string &coef, SymIR::Type type) {
    Assert(isActive(), "FuncBuilder has already finished its work");
    Assert(
        !paramMap.contains(name), "A parameter with the same name is already defined: %s",
        name.c_str()
    );
    Assert(
        !localMap.contains(name), "A local with the same name is already defined: %s", name.c_str()
    );
    locals.push_back(std::make_unique<Local>(name, std::make_unique<Coef>(coef, type), type));
    auto v = locals.back().get();
    localMap[name] = v;
    return v;
  }

  const Local *FuncBuilder::SymLocal(
      const std::string &name, const std::string &coefName, const std::string &coefVal,
      SymIR::Type type
  ) {
    Assert(isActive(), "FuncBuilder has already finished its work");
    Assert(
        !paramMap.contains(name), "A parameter with the same name is already defined: %s",
        name.c_str()
    );
    Assert(
        !localMap.contains(name), "A local with the same name is already defined: %s", name.c_str()
    );
    locals.push_back(
        std::make_unique<Local>(name, std::make_unique<Coef>(coefName, coefVal, type), type)
    );
    auto v = locals.back().get();
    localMap[name] = v;
    return v;
  }

  const Block *
  FuncBuilder::SymBlock(const std::string &label, const BlockBuilder::BlockBody &body) {
    Assert(isActive(), "FuncBuilder has already finished its work");
    Assert(!blockMap.contains(label), "Label conflicts with blocks: %s", label.c_str());
    BlockBuilder b(this, label);
    body(&b);
    blocks.push_back(b.Build());
    auto bb = blocks.back().get();
    blockMap[label] = bb;
    return bb;
  }

  std::unique_ptr<Func> FuncBuilder::Build() {
    Assert(isActive(), "FuncBuilder has already finished its work");
    deactivate();
    return std::make_unique<Func>(
        name, retType, std::move(params), std::move(locals), std::move(blocks)
    );
  }
} // namespace symir
