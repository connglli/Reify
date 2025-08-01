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
  SymIRBuilder::TermID BlockBuilder::SymTerm(Term::Op op, Coef *coef, const VarDef *var) {
    Assert(isActive(), "The BlockBuilder is no longer active");
    TermID tid = numCreatedTerms++;
    if (op == Term::Op::OP_CST) {
      createdTerms[tid] = std::make_unique<Term>(op, coef, nullptr);
    } else {
      createdTerms[tid] = std::make_unique<Term>(op, coef, std::make_unique<VarUse>(var));
    }
    return tid;
  }

  SymIRBuilder::ExprID BlockBuilder::SymExpr(Expr::Op op, const std::vector<TermID> &termIds) {
    Assert(isActive(), "The BlockBuilder is no longer active");
    ExprID eid = numCreatedExprs++;
    std::vector<std::unique_ptr<Term>> terms;
    for (const auto tid: termIds) {
      auto it = createdTerms.find(tid);
      Assert(it != createdTerms.end(), "Term with ID \"%lu\" does not exist", tid);
      terms.push_back(std::move(it->second));
      createdTerms.erase(it);
    }
    createdExprs[eid] = std::make_unique<Expr>(op, std::move(terms));
    return eid;
  }

  SymIRBuilder::ExprID BlockBuilder::SymCond(Cond::Op op, ExprID eid) {
    Assert(isActive(), "The BlockBuilder is no longer active");
    CondID cid = numCreatedConds++;
    auto it = createdExprs.find(eid);
    Assert(it != createdExprs.end(), "Expr with ID \"%lu\" does not exist", eid);
    createdConds[cid] = std::make_unique<Cond>(op, std::move(it->second));
    createdExprs.erase(it);
    return cid;
  }

  void BlockBuilder::SymAssign(const VarDef *var, ExprID eid) {
    Assert(isActive(), "The BlockBuilder is no longer active");
    auto it = createdExprs.find(eid);
    Assert(it != createdExprs.end(), "Expr with ID \"%lu\" does not exist", eid);
    stmts.push_back(std::make_unique<AssStmt>(std::make_unique<VarUse>(var), std::move(it->second))
    );
    createdExprs.erase(it);
  }

  void BlockBuilder::SymReturn() {
    Assert(isActive(), "The BlockBuilder is no longer active");
    std::vector<std::unique_ptr<VarUse>> uses;
    for (const auto *p: GetParent()->GetParams()) {
      uses.push_back(std::make_unique<VarUse>(p));
    }
    stmts.push_back(std::make_unique<RetStmt>(std::move(uses)));
  }

  void BlockBuilder::SymBranch(const std::string &truLab, const std::string &falLab, CondID cid) {
    Assert(isActive(), "The BlockBuilder is no longer active");
    auto it = createdConds.find(cid);
    Assert(it != createdConds.end(), "Cond with ID \"%lu\" does not exist", cid);
    target = std::make_unique<Branch>(std::move(it->second), truLab, falLab);
    createdConds.erase(it);
  }

  void BlockBuilder::SymGoto(const std::string &label) {
    Assert(isActive(), "The BlockBuilder is no longer active");
    target = std::make_unique<Goto>(label);
  }

  std::unique_ptr<Block> BlockBuilder::Build() {
    Assert(SymIRBuilderGeneric::isActive(), "The BlockBuilder is no longer active");
    deactivate();
    return std::make_unique<Block>(label, std::move(stmts), std::move(target));
  }

  /// Define and commit a new unsolved coefficient
  Coef *FunctBuilder::SymCoef(const std::string &name, SymIR::Type type) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    Assert(
        !symMap.contains(name), "Coefficients with the same name \"%s\" is already defined",
        name.c_str()
    );
    symbols.push_back(std::make_unique<Coef>(name, type));
    SymDef *s = symbols.back().get();
    symMap[name] = s;
    return dynamic_cast<Coef *>(s);
  }

  /// Define and commit a new solved coefficient
  Coef *FunctBuilder::SymCoef(const std::string &name, const std::string &value, SymIR::Type type) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    Assert(
        !symMap.contains(name), "Coefficients with the same name \"%s\" is already defined",
        name.c_str()
    );
    symbols.push_back(std::make_unique<Coef>(name, value, type));
    SymDef *s = symbols.back().get();
    symMap[name] = s;
    return dynamic_cast<Coef *>(s);
  }

  const Param *FunctBuilder::SymParam(const std::string &name, SymIR::Type type) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    Assert(
        !paramMap.contains(name), "Parameters with the same name \"%s\" is already defined",
        name.c_str()
    );
    Assert(
        !localMap.contains(name), "Locals with the same name \"%s\" is already defined",
        name.c_str()
    );
    params.push_back(std::make_unique<Param>(name, type));
    auto v = params.back().get();
    paramMap[name] = v;
    return v;
  }

  const Local *FunctBuilder::SymLocal(const std::string &name, Coef *coef, SymIR::Type type) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    Assert(
        !paramMap.contains(name), "Parameters with the same name \"%s\" is already defined",
        name.c_str()
    );
    Assert(
        !localMap.contains(name), "Locals with the same name \"%s\" is already defined",
        name.c_str()
    );
    locals.push_back(std::make_unique<Local>(name, coef, type));
    auto v = locals.back().get();
    localMap[name] = v;
    return v;
  }

  const Block *
  FunctBuilder::SymBlock(const std::string &label, const BlockBuilder::BlockBody &body) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    BlockBuilder *b = OpenBlock(label);
    body(b);
    return CloseBlock(b);
  }

  BlockBuilder *FunctBuilder::OpenBlock(const std::string &label) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    Assert(
        !blockMap.contains(label), "Blocks with the same label \"%s\" already exists", label.c_str()
    );
    for (auto &b: createdBlocks) {
      Assert(b.first != label, "Blocks with the same label \"%s\" already exists", label.c_str());
    }
    createdBlocks[label] = std::make_unique<BlockBuilder>(this, label);
    return createdBlocks[label].get();
  }

  const Block *FunctBuilder::CloseBlock(BlockBuilder *builder) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    const std::string &label = builder->GetLabel();
    const auto it = createdBlocks.find(label);
    Assert(
        it != createdBlocks.end(), "Blocks with the same label \"%s\" does not exists",
        label.c_str()
    );
    blocks.push_back(builder->Build());
    blockMap[label] = blocks.back().get();
    createdBlocks.erase(it);
    return blocks.back().get();
  }

  const Block *FunctBuilder::CloseBlockAt(BlockBuilder *builder, const Block *atBlk) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    const std::string &label = builder->GetLabel();
    const auto it = createdBlocks.find(label);
    Assert(
        it != createdBlocks.end(), "Blocks with the same label \"%s\" does not exists",
        label.c_str()
    );
    Assert(atBlk != nullptr, "The given block to insert before cannot be null");
    const auto atPos =
        std::ranges::find_if(blocks, [=](const auto &b) { return b.get() == atBlk; });
    Assert(
        atPos != blocks.end(), "The given block with label \"%s\" is not part of the function",
        atBlk->GetLabel().c_str()
    );
    blocks.insert(atPos, builder->Build());
    blockMap[label] = (*(atPos - 1)).get();
    createdBlocks.erase(it);
    return (*(atPos - 1)).get();
  }

  std::unique_ptr<Funct> FunctBuilder::Build() {
    Assert(isActive(), "The FunctBuilder is no longer active");
    deactivate();
    return std::make_unique<Funct>(
        name, retType, std::move(params), std::move(locals), std::move(symbols), std::move(blocks)
    );
  }
} // namespace symir
