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

#ifndef REIFY_UBINJECT_HPP
#define REIFY_UBINJECT_HPP

#include <bitwuzla/cpp/bitwuzla.h>
#include <memory>
#include "lib/lang.hpp"
#include "lib/logger.hpp"
#include "lib/ubbase.hpp"

/// IntUBInject injects int-related undefined behaviors (UBs) into a given block of a given program.
/// It leverages bitvec theory to collect constraints.
class IntUBInject : public UBVisitorBase {
public:
  IntUBInject() : UBVisitorBase(std::make_unique<bitwuzla::TermManager>(), bitwuzla::Sort()) {
    bvSort = tm->mk_bv_sort(33); // 33-bit for overflow detection
  }

  // Get the collected constraints that ensures the execution to be UB-free over integer arithmetic
  [[nodiscard]] const std::vector<bitwuzla::Term> &GetConstraints() const { return constraints; }

  // Get the Bitwuzla term manager for collecting constraints
  [[nodiscard]] bitwuzla::TermManager &GetTermManager() { return *tm; }

  // Inject undefined behavior into the given block of the given function
  // Return a new function with the injected UBs if successful, otherwise return nullptr.
  // Our goal is to ensure that the function is UB-definitive and also makes the compiler realize
  // the UBs as early as possible. So instead of running a function-level abstract interpretation,
  // we run it in the block level. We ensure one of the statement in a block is UB-definitive and
  // inserts some initializations (i.e., assigning some initial values to the used variables) before
  // running the block.
  std::unique_ptr<symir::Funct> InjectUBs(const symir::Funct *f, const symir::Block *b);

  // Reset the visitor to its initial state
  virtual void Reset() {
    solver = nullptr; // Delete the old solver
    constraints.clear();
    ubConstraints.clear();
    Assert(
        exprStack.empty(),
        "The expression stack is not empty when resetting the UB injector: %zu elements left",
        exprStack.size()
    );
    versions.clear();
    paramTerms.clear();
    coefTerms.clear();
  }

  // Expose base class methods publicly
  using UBVisitorBase::CreateCoefExpr;
  using UBVisitorBase::CreateScaExpr;
  using UBVisitorBase::CreateStructFieldExpr;
  using UBVisitorBase::CreateVecElExpr;
  using UBVisitorBase::GetStructFieldName;
  using UBVisitorBase::GetVecElName;
  using UBVisitorBase::IsCoefManaged;

protected:
  // Expose base class push/pop expression methods
  using UBVisitorBase::popExpression;
  using UBVisitorBase::pushExpression;

protected:
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
  void ensureValidInitsForUses(const symir::Block *b);
  void extractAndInitializeUses(
      const symir::Block *b, symir::FunctBuilder *funBd, symir::BlockBuilder *blkBd
  );
  void extractSymbolsFromModel(const symir::Funct *f, const symir::Block *b);

  static std::string nextCoefNameForBlock(const std::string &label) {
    static int counter = 0;
    return "__intubinj_blk" + label + "_coefval" + std::to_string(counter++);
  }

  void makeCoefsInteresting(const std::vector<symir::Coef *> &coefs);

private:
  std::unique_ptr<bitwuzla::Bitwuzla> solver; // The solver for checking the constraints

  std::vector<bitwuzla::Term> constraints;     // The constraints generated by this visitor
  std::vector<bitwuzla::Term> ubConstraints{}; // The UB-specific constraints that ensure UBs

  const symir::Funct *currentFun = nullptr;
};

#endif // REIFY_UBINJECT_HPP
