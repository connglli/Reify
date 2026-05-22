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

// Contains Utilitys used by transformation rules;

#include "lib/patternmatch.hpp"
#include "lib/lang.hpp"

#ifndef REIFY_TRANSFORMATION_UTILS_HPP
#define REIFY_TRANSFORMATION_UTILS_HPP

namespace transformations::utils {

  // TODO: Maybe apply Reservoir Sampling here to avoid copying the AST twice
  // TODO: This has become extreamly hacky, need to find a better solution
  /// Replaces a Expr/Cond or Term inside a given Stmt if matchFunction returns true
  template<typename Node>
  class StmtReplacer : public symir::StmtCopier {
  public:
    StmtReplacer(
     symir::FunctBuilder *funBd,
     symir::BlockBuilder *blockBd
    ) : symir::StmtCopier(funBd, blockBd) {}
    void ReplaceStmt(
      const symir::Stmt *s, 
      std::function<bool(const Node *)> matchFunction,
      std::function<size_t(symir::FunctBuilder *, symir::BlockBuilder *, const Node &, void **)> replaceFunction,
      double randThreshold = 1
    );
  
    void *getExtractedDataRef() { return this->data; }
  
  protected:
    void Visit(const Node &e) override;
    void Visit(const symir::Branch &b) override;
  private:
    bool match(const Node &e) {
      return !this->hasReplaced && this->rand() <= this->randThreshold && this->matchFunction(&e);
    }
    ExprID replace(const Node &e) { 
      this->hasReplaced = true;
      return this->replaceFunction(funBd, blockBd, e, &this->data); 
    }
    double rand() { return this->randUniform(); }
  
  private:
    std::function<bool(const Node *)> matchFunction;
    std::function<size_t(symir::FunctBuilder *, symir::BlockBuilder *, const Node &, void **)> replaceFunction;
    std::function<double()> randUniform;
    void *data = nullptr;
    double randThreshold = 1;
    bool hasReplaced = false;
  };

  /// Copies the access vector of use
  std::vector<symir::Coef *> copyAccess(symir::FunctBuilder *funBd, const symir::VarUse *use);

  /// returns a symir CondID that corresponds to a condition that is trivially evaluates to 'condTarget' (e.g. (0) == 0 or (1) == 0)
  symir::BlockBuilder::CondID triviallyCondFor(bool condTarget, symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd);

  /// returns a random valid assignment;
  symir::BlockBuilder::StmtID trivialAssignment(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::VarDef *var,
    std::vector<symir::Coef *> access
  );

  /// Splits the given BlockBuilder into two blocks where the first (the given one mutated) contains all stmts up to and including stmt at
  /// splitIdx and the second one (the returned one) contains all after
  symir::BlockBuilder * splitBlockAt(
    symir::FunctBuilder * funBd,
    symir::BlockBuilder * blockBd,
    std::string secondLabel,
    size_t splitIdx
  );

  void insertBlockBd(std::vector<symir::BlockBuilder *> &currBlockBds, std::vector<symir::BlockBuilder *> newBlocks, size_t index);

  std::string nameLabel(std::string functName, std::string prefix);

  std::string nameVariable(std::string domBlockName, std::string prefix);

  using namespace patternmatch;
  bool matchSubExprInAnyStmt(const symir::Stmt *stmt, const Pattern<const symir::Expr *> &E);

}

#endif //REIFY_TRANSFORMATION_UTILS_HPP
