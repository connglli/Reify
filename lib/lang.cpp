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

BlockBuilder::TermID BlockBuilder::MyTerm(Term::Op op, const std::string &coef, const Var *var) {
  Assert(!finished, "BlockBuilder has already finished its work");
  TermID tid = numCreatedTerms++;
  createdTerms[tid] =
      std::make_unique<::Term>(op, std::make_unique<Coef>("c", coef, var->GetType()), var);
  return tid;
}

BlockBuilder::ExprID BlockBuilder::MyExpr(Expr::Op op, const std::vector<TermID> &termIds) {
  Assert(!finished, "BlockBuilder has already finished its work");
  ExprID eid = numCreatedExprs++;
  std::vector<std::unique_ptr<::Term>> terms;
  for (const auto tid: termIds) {
    auto it = createdTerms.find(tid);
    Assert(it != createdTerms.end(), "Term with ID=%lu does not exist", tid);
    terms.push_back(std::move(it->second));
    createdTerms.erase(it);
  }
  createdExprs[eid] = std::make_unique<::Expr>(op, std::move(terms));
  return eid;
}

BlockBuilder::ExprID BlockBuilder::MyCond(Cond::Op op, ExprID eid) {
  Assert(!finished, "BlockBuilder has already finished its work");
  CondID cid = numCreatedConds++;
  auto it = createdExprs.find(eid);
  Assert(it != createdExprs.end(), "Expr with ID=%lu does not exist", eid);
  createdConds[cid] = std::make_unique<::Cond>(op, std::move(it->second));
  createdExprs.erase(it);
  return cid;
}

void BlockBuilder::MyAssign(const Var *var, ExprID eid) {
  Assert(!finished, "BlockBuilder has already finished its work");
  auto it = createdExprs.find(eid);
  Assert(it != createdExprs.end(), "Expr with ID=%lu does not exist", eid);
  stmts.push_back(std::make_unique<AssStmt>(var, std::move(it->second)));
  createdExprs.erase(it);
}

void BlockBuilder::MyReturn() {
  Assert(!finished, "BlockBuilder has already finished its work");
  std::vector<const Var *> vars = ctx->GetParams();
  stmts.push_back(std::make_unique<RetStmt>(vars));
}

void BlockBuilder::MyBranch(CondID cid, const std::string &truLab, const std::string &falLab) {
  Assert(!finished, "BlockBuilder has already finished its work");
  auto it = createdConds.find(cid);
  Assert(it != createdConds.end(), "Cond with ID=%lu does not exist", cid);
  target = std::make_unique<::Branch>(std::move(it->second), truLab, falLab);
  createdConds.erase(it);
  finished = true;
}

void BlockBuilder::MyGoto(const std::string &label) {
  Assert(!finished, "BlockBuilder has already finished its work");
  target = std::make_unique<::Goto>(label);
  finished = true;
}

void BlockBuilder::MyFallThro() {
  Assert(!finished, "BlockBuilder has already finished its work");
  finished = true;
}

std::unique_ptr<Block> BlockBuilder::Build() {
  Assert(finished, "BlockBuilder has not yet finished its work");
  return std::make_unique<Block>(label, std::move(stmts), std::move(target));
}

const Var *FuncBuilder::MyVar(const std::string &name, MyIR::Type type) {
  Assert(!finished, "BlockBuilder has already finished its work");
  Assert(!paramMap.contains(name), "Name conflicts with parameter: %s", name.c_str());
  params.push_back(std::make_unique<Var>(name, type));
  auto v = params.back().get();
  paramMap[name] = v;
  return v;
}

const Block *FuncBuilder::MyBlock(const std::string &label, const BlockBuilder::BlockBody &body) {
  Assert(!blockMap.contains(label), "Label conflicts with blocks: %s", label.c_str());
  BlockBuilder b(this, label);
  body(&b);
  blocks.push_back(b.Build());
  auto bb = blocks.back().get();
  blockMap[label] = bb;
  return bb;
}

const Var *FuncBuilder::FindVar(const std::string &name) const {
  if (paramMap.contains(name)) {
    return paramMap.find(name)->second;
  } else {
    return nullptr;
  }
}

const Block *FuncBuilder::FindBlock(const std::string &label) const {
  if (blockMap.contains(label)) {
    return blockMap.find(label)->second;
  } else {
    return nullptr;
  }
}

std::unique_ptr<Func> FuncBuilder::Build() {
  return std::make_unique<Func>(name, retType, std::move(params), std::move(blocks));
}
