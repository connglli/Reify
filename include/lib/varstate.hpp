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

#ifndef REIFY_VARSTATE_HPP
#define REIFY_VARSTATE_HPP

#include "lib/lang.hpp"
#include "lib/symexec.hpp"
#include "lib/ubfree.hpp"
#include <bitwuzla/cpp/bitwuzla.h>

class VariableState;

namespace varstate {
  std::vector<VariableState> allFromJsonFile(std::string filepath);
  void print32_t_state(size_t nr_variables, std::vector<int32_t> states);
}

class VariableState : symir::SymIRVisitor {
public:
  nlohmann::json toJson();
  void fromJson(nlohmann::json json);
  std::pair<size_t, std::vector<int32_t>> query(size_t blockIndex, size_t stmtIndex);
  void extract(SymExec *symexec);
  std::vector<int32_t> getPathBlocksIndices();
  std::vector<std::string> getPathBlocksLabels();

  std::map<size_t, std::string> GetVarMap() { return this->varNamesMap; }

private:
  void pushTerm(bitwuzla::Term term) { this->termStack.push(std::move(term)); }
  bitwuzla::Term popTerm() {
    auto term = this->termStack.top();
    this->termStack.pop();
    return term;
  }
  struct blockState {
    // first: Index, second: Label
    std::pair<int32_t, std::string> blockId;
    // every 2 int32_ts are Variable index followed by target value
    std::vector<std::pair<int32_t, int32_t>> assignments;
  };
  void Visit(const symir::VarUse &v) override;
  void Visit(const symir::Coef &c) override;
  void Visit(const symir::Term &t) override;
  void Visit(const symir::ModExpr &e) override;
  void Visit(const symir::Expr &e) override;
  void Visit(const symir::Cond &c) override;
  void Visit(const symir::ModAssStmt &a) override;
  void Visit(const symir::AssStmt &a) override;
  void Visit(const symir::RetStmt &r) override;
  void Visit(const symir::Branch &b) override;
  void Visit(const symir::Goto &g) override;
  void Visit(const symir::ScaParam &p) override;
  void Visit(const symir::VecParam &p) override;
  void Visit(const symir::StructParam &p) override;
  void Visit(const symir::UnInitLocal &l) override;
  void Visit(const symir::ScaLocal &l) override;
  void Visit(const symir::VecLocal &l) override;
  void Visit(const symir::StructLocal &l) override;
  void Visit(const symir::StructDef &s) override;
  void Visit(const symir::Block &b) override;
  void Visit(const symir::Funct &f) override;


private:
  std::vector<int32_t> init;
  std::map<size_t, std::string> varNamesMap;
  std::vector<blockState> executionState;

// Used only by extraction
  SymExec *symexec;
  size_t currBlock;
  std::map<std::string, int32_t> versions{};  // The SSA version table for each variable
  std::stack<bitwuzla::Term> termStack{}; // The expression stack for evaluating the SymIR program
};

#endif // REIFY_VARSTATE_HPP
