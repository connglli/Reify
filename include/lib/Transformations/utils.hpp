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
#include "lib/varstate.hpp"
#include "lib/random.hpp"

#include <flint/ulong_extras.h>
#include <flint/nmod.h>
#include <flint/nmod_mat.h>

#ifndef REIFY_TRANSFORMATION_UTILS_HPP
#define REIFY_TRANSFORMATION_UTILS_HPP

namespace transformations::utils {

  std::vector<symir::Coef *> unflattenAccess(symir::FunctBuilder *funBuilder, const symir::VarDef *var, size_t flattenedIndex);

  class VarFilter {
  public:
    VarFilter(
      symir::FunctBuilder *funBd,
      VariableState &varState
    ): funBd(funBd), varState(varState) {}
    void randomlyFilter();
    void uniqueFilter();
    
  public:
    std::vector<int32_t> filteredVarState{};
    std::vector<const symir::VarDef *> filteredVars{};
    std::vector<std::vector<symir::Coef *>> filteredAccesses{};

  private:
    symir::FunctBuilder *funBd;
    VariableState &varState;

  };

  class PrimeInterpolation {
  public:
    PrimeInterpolation(int32_t mod) {
      Assert(mod <= 46337, "To avoid overflow mod must be less then 46337");
      Assert(n_is_prime(mod), "PrimeInterpolation requires that mod is prime");
      nmod_init(&this->mod, mod);
    }
  
    void interpolate(size_t nrVariables, size_t nrIterations, std::vector<int32_t> varState, int32_t target);
  
    void interpolate(size_t nrVariables, size_t nrIterations, std::vector<int32_t> varState, std::vector<int32_t> targets);
  
    std::vector<int32_t> getPolynomial() {
      return std::vector(this->polynomial);
    }
  
    std::vector<int32_t> getCoeffs() {
      return std::vector(this->coeffs);
    }
  
    /// Triggers an assert if the last interpolation does not correctly yield 'target' when evaluated over 'varState'
    void assertCorrectness(size_t nrVariables, size_t nrIterations, std::vector<int32_t> varState, int32_t target);

    /// Triggers an assert if the last interpolation does not correctly yield 'targets' when evaluated over 'varState'
    void assertCorrectness(size_t nrVariables, size_t nrIterations, std::vector<int32_t> varState, std::vector<int32_t> targets);

  private:
  
    /// Find a new unused iteration according to varState
    std::vector<int32_t> findUniqueIteration(size_t nrVariables, size_t nrIterations, std::vector<int32_t> varState);
    
    /// see https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle
    void shuffel(const size_t n, int32_t *arr) {
      for (int i_ = n - 1; i_ >= 1; i_--) {
        size_t r = static_cast<size_t>(Random::Get().Uniform(0, i_)());
        size_t i = static_cast<size_t>(i_);
        int32_t temp = arr[r];
        arr[r] = arr[i];
        arr[i] = temp;
      }
    }
    
    // This is a quite biased in its selection.
    // Monomial ordering: https://people.math.sc.edu/Burkardt/c_src/monomial/monomial.html
    /// Samples a new random polynomial
    void randomizePolynomial(
      const size_t nrVariables,
      const size_t nrMonomials
    );

    /// Returns the (mathematical) modulo of 'var' (e.g. 'var' mod 'this->mod.n')
    ulong reduceMod(int32_t var) {
      ulong res;
      if (var < 0) {
        NMOD_RED(res, static_cast<ulong>(-static_cast<int64_t>(var)), this->mod);
        res = res != 0 ? this->mod.n - res : res;
      } else {
        NMOD_RED(res, static_cast<ulong>(var), this->mod);
      }
      Assert(res < this->mod.n, "reduceMod has produced 'res' not in Z_%ld", this->mod.n);
      return res;
    }

  private:
    nmod_t mod;
    std::vector<int32_t> polynomial;
    std::vector<int32_t> coeffs;
  };

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
  
  public:
    void *data = nullptr;

  private:
    std::function<bool(const Node *)> matchFunction;
    std::function<size_t(symir::FunctBuilder *, symir::BlockBuilder *, const Node &, void **)> replaceFunction;
    std::function<double()> randUniform;
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
    std::vector<symir::Coef *> access = {}
  );

  /// returns a term that is equal to just the value in the passed variable, access
  symir::BlockBuilder::TermID variableTerm(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::VarDef *var,
    std::vector<symir::Coef *> access = {}
  );

  symir::Expr::Op randomExprOp();

  /// Splits the given BlockBuilder into two blocks where the first (the given one mutated) contains all stmts up to and including stmt at
  /// splitIdx and the second one (the returned one) contains all after
  symir::BlockBuilder * splitBlockAt(
    symir::FunctBuilder * funBd,
    symir::BlockBuilder * blockBd,
    std::string secondLabel,
    size_t splitIdx
  );

  void insertBlockBd(std::vector<symir::BlockBuilder *> &currBlockBds, std::vector<symir::BlockBuilder *> newBlocks, size_t index);

  std::string nameLabel(const std::string functName, const std::string prefix);

  void clearNameLabel();

  std::string nameVariable(const std::string functName, const std::string domBlockName, const std::string prefix);
  std::string nameVariable(const std::string functName, const std::string domBlockName, const std::string prefix, size_t size);

  void clearNameVariable();

  const symir::VarDef *getVariable(symir::FunctBuilder *funBd, const std::string domBlockName, const std::string prefix);
  const symir::VarDef *getVariable(symir::FunctBuilder *funBd, const std::string domBlockName, const std::string prefix, size_t size);

  bool noAddOverflow(int32_t a, int32_t b);
  bool noSubOverflow(int32_t a, int32_t b);

  using namespace patternmatch;
  bool matchSubExprInAnyStmt(const symir::Stmt *stmt, const Pattern<const symir::Expr *> &E);

}

#endif //REIFY_TRANSFORMATION_UTILS_HPP
