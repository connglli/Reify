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

  template<typename Node>
  class StmtReplacer : public symir::StmtCopier {
  public:
    StmtReplacer(
     symir::FunctBuilder *funBd,
     symir::BlockBuilder *blockBd
    ) : symir::StmtCopier(funBd, blockBd) {}
    /// Copies Stmt with while also replacing any Subexpression that matches 'matchFunction' with the return value of 'replaceFunction'
    StmtID CopyStmtWithReplacement(
      const symir::Stmt *s, 
      std::function<bool(const Node *)> matchFunction,
      std::function<size_t(symir::FunctBuilder *, symir::BlockBuilder *, const Node &, void **)> replaceFunction,
      double randThreshold = 1
    );
  
    void *getExtractedDataRef() { return this->data; }
  
  protected:
    void Visit(const Node &e) override;
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

  /// returns a symir CondID that corresponds to a condition that is trivially false (e.g. (1) == 0)
  symir::BlockBuilder::CondID triviallyFalseCond(symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd);

  /// returns a symir CondID that corresponds to a condition that is trivially true (e.g. (0) == 0)
  symir::BlockBuilder::CondID triviallyTrueCond(symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd);

  /// matches any Stmt that has pattern E as a subexpression
  bool matchSubExprInAnyStmt(const symir::Stmt *stmt, const patternmatch::Pattern<const symir::Expr *> &E);

}
#endif //REIFY_TRANSFORMATION_UTILS_HPP
