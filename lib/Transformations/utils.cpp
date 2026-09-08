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

#include "lib/Transformations/utils.hpp"
#include <climits>
#include <utility>
#include "global.hpp"
#include "lib/dbgutils.hpp"
#include "lib/lang.hpp"
#include "lib/logger.hpp"
#include "lib/patternmatch.hpp"
#include "lib/random.hpp"

namespace transformations::utils {
  /// given a flattend intex recover the access vector needed to create a VarUse Object
  std::vector<symir::Coef *> UnflattenAccess(
      symir::FunctBuilder *funBuilder, const symir::VarDef *var, size_t flattenedIndex
  ) {
    Assert(var != nullptr, "Variable is nullptr");
    if (var->GetType() == symir::SymIR::Type::I32) {
      Assert(
          flattenedIndex == 0, "Trying to index into %s which is an (I32, %s) with %ld",
          var->GetName().c_str(),
          var->GetBaseType() == symir::SymIR::Type::I32
              ? "i32"
              : (var->GetBaseType() == symir::SymIR::Type::ARRAY ? "ARRAY" : "STRUCT"),
          flattenedIndex
      );
      return {};
    }
    std::vector<int32_t> accessVals;

    symir::SymIR::Type type = var->GetType();
    symir::SymIR::Type baseType = var->GetBaseType();
    std::string structName =
        (type == symir::SymIR::Type::STRUCT)
            ? var->GetStructName()
            : (baseType == symir::SymIR::Type::STRUCT ? var->GetStructName() : "");
    std::vector<int32_t> shape = var->IsVector() ? var->GetVecShape() : std::vector<int32_t>{};

    size_t remainingIndex = flattenedIndex;

    // walk down the type tree to generate the access vector
  typeLoop:
    while (type != symir::SymIR::I32) {
      switch (type) {
        case symir::SymIR::ARRAY: {
          size_t type_size = symir::IntSizeOfSymIRType(
              funBuilder->GetStructs(), baseType, baseType, {}, structName
          );
          for (const int32_t dimSize: shape) {
            type_size *= dimSize;
          }
          for (const int32_t dimSize: shape) {
            type_size /= dimSize;
            accessVals.push_back(remainingIndex / type_size);
            remainingIndex %= type_size;
          }
          type = baseType;
          shape = {};
        } break;
        case symir::SymIR::STRUCT: {
          const auto *sDef = funBuilder->FindStruct(structName);
          Assert(sDef, "Struct %s not found", structName.c_str());
          size_t fieldIdx = 0;
          for (const auto &field: sDef->GetFields()) {
            type = field.type;
            baseType = field.baseType;
            if (type == symir::SymIR::Type::STRUCT) {
              structName = field.structName;
            } else if (type == symir::SymIR::Type::ARRAY) {
              shape = field.shape;
              if (baseType == symir::SymIR::Type::STRUCT) {
                structName = field.structName;
              }
            }
            size_t field_size =
                IntSizeOfSymIRType(funBuilder->GetStructs(), type, baseType, shape, structName);
            if (field_size > remainingIndex) {
              accessVals.push_back(fieldIdx);
              goto typeLoop;
            }
            remainingIndex -= field_size;
            fieldIdx += 1;
          }
        } break;
        default:
          Panic("Unknown or Int Type while finding Access Path");
      }
    }
    Assert(
        remainingIndex == 0,
        "Var Access has overflown with remainingIndex: %ld, on var: %s, struct: %s, flattendIndex: "
        "%ld",
        remainingIndex, var->GetName().c_str(), var->GetStructName().c_str(), flattenedIndex
    );

    std::vector<symir::Coef *> access;
    access.reserve(accessVals.size());
    // hacky counter to avoid "same named coefficient" error for polynomial coeffs
    static size_t unique_counter = 0;
    for (size_t i = 0; i < accessVals.size(); i++) {
      access.push_back(funBuilder->SymCoef(
          "poly_var_access_" + var->GetName() + "_" + std::to_string(flattenedIndex) + "_" +
              std::to_string(i) + "_" + std::to_string(unique_counter),
          std::to_string(accessVals[i])
      ));
    }
    unique_counter += 1;
    return access;
  }

  void VarFilter::RandomlyFilter() {
    auto randDouble = Random::Get().UniformReal();
    size_t nrVariables = this->varState.nrVariables;
    size_t nrIterations = this->varState.varState.size() / nrVariables;
    // randomly select a subset of variables and find there VarDef
    this->filteredVars.clear();
    this->filteredAccesses.clear();
    std::vector<size_t> indices;
    size_t lastVarStartIndex = 0;
    for (size_t i = 0; i < nrVariables; i++) {
      if (this->varState.varMap.contains(i))
        lastVarStartIndex = i;
      if (indices.size() > 0 && randDouble() > 1 - GlobalOptions::Get().VariableTakeProba)
        continue;
      indices.push_back(i);
      const symir::VarDef *var = funBd->FindVar(this->varState.varMap[lastVarStartIndex]);
      Assert(
          var != nullptr, "var %s not found in fun %s",
          this->varState.varMap[lastVarStartIndex].c_str(), funBd->GetName().c_str()
      );
      this->filteredVars.push_back(var);
      this->filteredAccesses.push_back(UnflattenAccess(funBd, var, i - lastVarStartIndex));
    }
    Log::Get().Out() << "Using Variables (" << this->filteredVars.size() << "): ";
    for (size_t i = 0; i < this->filteredVars.size(); i++) {
      Log::Get().Out() << this->filteredVars[i]->GetName();
      if (this->filteredVars[i]->GetType() != symir::SymIR::I32) {
        Log::Get().Out() << "[";
        for (size_t j = 0; j < this->filteredAccesses[i].size() - 1; j++) {
          Log::Get().Out() << this->filteredAccesses[i][j]->GetI32Value() << ", ";
        }
        Log::Get().Out() << this->filteredAccesses[i].back()->GetI32Value() << "]";
      }
      Log::Get().Out() << "(" << indices[i] << "): [";
      for (size_t k = 0; k < nrIterations; k++) {
        Log::Get().Out() << this->varState.varState[k * nrVariables + indices[i]];
        if (k == nrIterations - 1) {
          Log::Get().Out() << "]";
        } else {
          Log::Get().Out() << ", ";
        }
      }
      if (i == this->filteredVars.size() - 1) {
        Log::Get().Out() << std::endl;
      } else {
        Log::Get().Out() << ", ";
      }
    }

    // Filter varState
    this->filteredVarState.clear();
    this->filteredVarState.reserve(nrIterations * this->filteredVars.size());
    for (size_t i = 0; i < nrIterations; i++) {
      for (size_t j = 0; j < this->filteredVars.size(); j++) {
        this->filteredVarState.push_back(this->varState.varState[i * nrVariables + indices[j]]);
      }
    }
  }

  void PrimeInterpolation::Interpolate(
      size_t nrVariables, size_t nrIterations, std::vector<int32_t> varState, int32_t target
  ) {
    Assert(
        0 <= target && target < static_cast<int32_t>(this->mod.n), "targets (%d) must be in Z_%ld",
        target, this->mod.n
    );
    Assert(nrVariables * nrIterations == varState.size(), "varState is the wrong size");
    Assert(nrVariables > 0 && nrIterations > 0, "must atleast have one variable and one monomial");

    Log::Get().OpenSection("Interpolation");

    nmod_mat_t A;
    nmod_mat_t B;
    nmod_mat_t X;
    nmod_mat_init(A, nrIterations + 1, nrIterations + 1, this->mod.n);
    nmod_mat_init(B, nrIterations + 1, 1, this->mod.n);
    nmod_mat_init(X, nrIterations + 1, 1, this->mod.n);

    // Fill B vector
    for (size_t row = 0; row < nrIterations; row++) {
      nmod_mat_set_entry(B, row, 0, target);
    }

    // Fill the last element of B with a unique new target value to avoid the const polynomial
    nmod_mat_set_entry(B, nrIterations, 0, nmod_add(static_cast<ulong>(target), 1, this->mod));


    int32_t res;
    for (size_t i = 0; i < 5;
         i++) { // retry up to 5 times incase the system is unsolvable should rarly happen
      this->RandomizePolynomial(nrVariables, nrIterations);

      Log::Get().Out() << "Polynomial (" << this->polynomial.size() << "): ";
      for (size_t i = 0; i < this->polynomial.size(); i++) {
        Log::Get().Out() << this->polynomial[i] << " ";
      }
      Log::Get().Out() << std::endl;

      // set all entries of A
      for (size_t row = 0; row < nrIterations; row++) {
        for (size_t col = 0; col < nrIterations; col++) {
          ulong term = 1;
          for (uint32_t vari = 0; vari < nrVariables; vari++) {
            int32_t var = this->ReduceMod(varState[row * nrVariables + vari]);
            ulong deg = static_cast<ulong>(this->polynomial[col * nrVariables + vari]);
            term = nmod_mul(term, nmod_pow_ui(var, deg, this->mod), this->mod);
          }
          nmod_mat_set_entry(A, row, col, term);
        }
        nmod_mat_set_entry(A, row, nrIterations, 1);
      }

      // fill the last row of the A matrix with unique new numbers to avoid the const polynomial
      std::vector<int32_t> unique_iter =
          this->FindUniqueIteration(nrVariables, nrIterations, varState);
      for (size_t col = 0; col < nrIterations; col++) {
        ulong term = 1;
        Assert(col * nrVariables < nrIterations * nrVariables, "Array access out of bounds");
        for (uint32_t i = 0; i < nrVariables; i++) {
          int32_t var = this->ReduceMod(unique_iter[i]);
          ulong deg = static_cast<ulong>(this->polynomial[col * nrVariables + i]);
          term = nmod_mul(term, nmod_pow_ui(var, deg, this->mod), this->mod);
        }
        nmod_mat_set_entry(A, nrIterations, col, term);
      }
      nmod_mat_set_entry(A, nrIterations, nrIterations, 1);

      Log::Get().Out() << "Interpolation Matrix A[" << nrIterations + 1 << ", " << nrIterations + 1
                       << "]:" << std::endl;
      for (size_t row = 0; row < nrIterations + 1; row++) {
        Log::Get().Out() << "[";
        for (size_t col = 0; col < nrIterations + 1; col++) {
          Log::Get().Out() << nmod_mat_get_entry(A, row, col);
          if (col != nrIterations)
            Log::Get().Out() << ", ";
        }
        Log::Get().Out() << "]" << std::endl;
      }
      Log::Get().Out() << "Rank(A) = " << nmod_mat_rank(A) << std::endl;

      // solve the system
      res = nmod_mat_can_solve(X, A, B);
      if (res == 1)
        break;
      Log::Get().Out() << "System Unsolvable" << std::endl;
    }
    Assert(res == 1, "Unable to solve AX = B");

    nmod_mat_clear(A);
    nmod_mat_clear(B);

    // copy to the output vector
    this->coeffs.clear();
    this->coeffs.resize(nrIterations + 1);
    for (size_t row = 0; row < nrIterations + 1; row++) {
      this->coeffs[row] = nmod_mat_get_entry(X, row, 0);
    }
    nmod_mat_clear(X);

    Log::Get().Out() << std::endl << "Coeffs (" << this->coeffs.size() << "): ";
    for (size_t i = 0; i < this->coeffs.size(); i++) {
      Log::Get().Out() << this->coeffs[i] << " ";
    }
    Log::Get().Out() << std::endl;

    Log::Get().CloseSection();
  }

  void PrimeInterpolation::Interpolate(
      size_t nrVariables, size_t nrIterations, std::vector<int32_t> varState,
      std::vector<int32_t> targets
  ) {
    Assert(targets.size() == nrIterations, "Must have a target value for each iteration");
    Assert(nrVariables > 0 && nrIterations > 0, "must atleast have one variable and one monomial");

    bool has_unique = false;
    for (const int32_t target: targets) {
      Assert(
          0 <= target && target < static_cast<int32_t>(this->mod.n), "targets must be in Z_%d",
          static_cast<int32_t>(this->mod.n)
      );
      has_unique |= targets[0] != target;
    }
    Assert(has_unique, "target must have atleast on unique element");

    this->RandomizePolynomial(nrVariables, nrIterations - 1);

    nmod_mat_t A;
    nmod_mat_t B;
    nmod_mat_t X;
    nmod_mat_init(A, nrIterations, nrIterations, this->mod.n);
    nmod_mat_init(B, nrIterations, 1, this->mod.n);
    nmod_mat_init(X, nrIterations, 1, this->mod.n);

    // set all entries of A
    for (size_t row = 0; row < nrIterations; row++) {
      for (size_t col = 0; col < nrIterations - 1; col++) {
        ulong term = 1;
        for (size_t vari = 0; vari < nrVariables; vari++) {
          int32_t var = this->ReduceMod(varState[row * nrVariables + vari]);
          ulong deg = static_cast<int32_t>(this->polynomial[col * nrVariables + vari]);
          term = nmod_mul(term, nmod_pow_ui(var, deg, this->mod), this->mod);
        }
        nmod_mat_set_entry(A, row, col, term);
      }
      nmod_mat_set_entry(A, row, nrIterations - 1, 1);
    }

    // build B from m and d
    for (size_t row = 0; row < nrIterations; row++) {
      nmod_mat_set_entry(B, row, 0, targets[row]);
    }

    // solve the system
    int32_t res = nmod_mat_can_solve(X, A, B);
    Assert(res == 1, "Unable to solve AX = B");

    nmod_mat_clear(A);
    nmod_mat_clear(B);

    // build B from m and d
    this->coeffs.clear();
    this->coeffs.resize(nrIterations);
    for (size_t row = 0; row < nrIterations; row++) {
      this->coeffs[row] = nmod_mat_get_entry(X, row, 0);
    }
    nmod_mat_clear(X);
  }

  void PrimeInterpolation::AssertCorrectness(
      size_t nrVariables, size_t nrIterations, std::vector<int32_t> varState, int32_t target
  ) {
    Assert(
        0 <= target && target < static_cast<int32_t>(this->mod.n), "targets (%d) must be in Z_%ld",
        target, this->mod.n
    );
    Assert(nrVariables * nrIterations == varState.size(), "varState is the wrong size");
    Assert(nrVariables > 0 && nrIterations > 0, "must atleast have one variable and one monomial");

    Assert(this->polynomial.size() == nrVariables * nrIterations, "Interpolation not solved");
    Assert(
        this->coeffs.size() == nrIterations + 1, "Interpolation not solved or does not match input"
    );

    for (size_t i = 0; i < nrIterations; i++) {
      int32_t result = 0;
      for (size_t m = 0; m < nrIterations; m++) {
        Assert(
            0 <= this->coeffs[m] && this->coeffs[m] < static_cast<int32_t>(this->mod.n),
            "Coef (%d) not in Z_%ld", this->coeffs[m], this->mod.n
        );
        int32_t term = this->coeffs[m];
        for (size_t j = 0; j < nrVariables; j++) {
          ulong var = this->ReduceMod(varState[i * nrVariables + j]);
          Assert(
              0 <= this->polynomial[m * nrVariables + j] &&
                  this->polynomial[m * nrVariables + j] < static_cast<int32_t>(this->mod.n),
              "Coef (%d) not in Z_%ld", this->polynomial[m * nrVariables + j], this->mod.n
          );
          ulong deg = static_cast<ulong>(this->polynomial[m * nrVariables + j]);
          term = static_cast<int32_t>(
              nmod_mul(static_cast<ulong>(term), nmod_pow_ui(var, deg, this->mod), this->mod)
          );
        }
        result = static_cast<int32_t>(
            nmod_add(static_cast<ulong>(result), static_cast<ulong>(term), this->mod)
        );
      }
      Assert(
          0 <= this->coeffs[nrIterations] &&
              this->coeffs[nrIterations] < static_cast<int32_t>(this->mod.n),
          "Coef (%d) not in Z_%ld", this->coeffs[nrIterations], this->mod.n
      );
      result = static_cast<int32_t>(nmod_add(
          static_cast<ulong>(result), static_cast<ulong>(this->coeffs[nrIterations]), this->mod
      ));
      Assert(result == target, "interpolation is incorrect!");
    }
  }

  void PrimeInterpolation::AssertCorrectness(
      size_t nrVariables, size_t nrIterations, std::vector<int32_t> varState,
      std::vector<int32_t> targets
  ) {
    bool has_unique = false;
    for (const int32_t target: targets) {
      Assert(
          0 <= target && target < static_cast<int32_t>(this->mod.n), "targets must be in Z_%d",
          static_cast<int32_t>(this->mod.n)
      );
      has_unique |= targets[0] != target;
    }
    Assert(has_unique, "target must have atleast on unique element");
    Assert(nrVariables * nrIterations == varState.size(), "varState is the wrong size");
    Assert(nrVariables > 0 && nrIterations > 0, "must atleast have one variable and one monomial");

    Assert(this->polynomial.size() == nrVariables * (nrIterations - 1), "Interpolation not solved");
    Assert(this->coeffs.size() == nrIterations, "Interpolation not solved or does not match input");

    for (size_t i = 0; i < nrIterations; i++) {
      int32_t result = 0;
      for (size_t m = 0; m < nrIterations - 1; m++) {
        Assert(
            0 <= this->coeffs[m] && this->coeffs[m] < static_cast<int32_t>(this->mod.n),
            "Coef (%d) not in Z_%ld", this->coeffs[m], this->mod.n
        );
        int32_t term = this->coeffs[m];
        for (size_t j = 0; j < nrVariables; j++) {
          ulong var = this->ReduceMod(varState[i * nrVariables + j]);
          Assert(
              0 <= this->polynomial[m * nrVariables + j] &&
                  this->polynomial[m * nrVariables + j] < static_cast<int32_t>(this->mod.n),
              "Coef (%d) not in Z_%ld", this->polynomial[m * nrVariables + j], this->mod.n
          );
          ulong deg = static_cast<ulong>(this->polynomial[m * nrVariables + j]);
          term = static_cast<int32_t>(
              nmod_mul(static_cast<ulong>(term), nmod_pow_ui(var, deg, this->mod), this->mod)
          );
        }
        result = static_cast<int32_t>(
            nmod_add(static_cast<ulong>(result), static_cast<ulong>(term), this->mod)
        );
      }
      Assert(
          0 <= this->coeffs[nrIterations - 1] &&
              this->coeffs[nrIterations - 1] < static_cast<int32_t>(this->mod.n),
          "Coef (%d) not in Z_%ld", this->coeffs[nrIterations - 1], this->mod.n
      );
      result = static_cast<int32_t>(nmod_add(
          static_cast<ulong>(result), static_cast<ulong>(this->coeffs[nrIterations - 1]), this->mod
      ));
      Assert(result == targets[i], "interpolation is incorrect!");
    }
  }

  std::vector<int32_t> PrimeInterpolation::FindUniqueIteration(
      size_t nrVariables, size_t nrIterations, std::vector<int32_t> varState
  ) {
    std::vector<int32_t> unique;
    std::vector<int32_t> currVarState;
    unique.reserve(nrVariables);
    currVarState.resize(nrIterations);
    for (size_t i = 0; i < nrVariables; i++) {
      // Copy and sort to make it easy to find new unique value
      for (size_t j = 0; j < nrIterations; j++) {
        int32_t var = varState[j * nrVariables + i];
        currVarState[j] = this->ReduceMod(var);
      }
      std::sort(currVarState.begin(), currVarState.end());

      // To actually find a unique element we look for the largest difference between to successive
      // elements. This is to minimize the chance the AX = B is unsolvable
      int32_t last = 0;
      int32_t largest_range = 0;
      int32_t candidate = -1;
      for (size_t k = 0; k < nrIterations; k++) {
        // if the next value is not the successor of - or equal to the last one there must be a
        // unique value inbetween
        if (currVarState[k] != static_cast<int32_t>((last + 1) % mod.n) &&
            currVarState[k] - 1 - (last + 1) >= largest_range) {
          largest_range = currVarState[k] - 1 - (last + 1);
          candidate = Random::Get().Uniform(last + 1, currVarState[k] - 1)();
        }
        last = currVarState[k];
      }
      if (static_cast<int32_t>(mod.n - 1) - currVarState.back() >= largest_range) {
        largest_range = static_cast<int32_t>(mod.n - 1) - currVarState.back();
        candidate =
            Random::Get().Uniform(currVarState.back() + 1, static_cast<int32_t>(mod.n - 1))();
      }
      if (largest_range > 0)
        unique.push_back(candidate);
      else
        Panic("unable for find a unique element");
    }
    Assert(unique.size() == nrVariables, "unique array not of the right size");
    return unique;
  }

  void PrimeInterpolation::RandomizePolynomial(const size_t nrVariables, const size_t nrMonomials) {
    Assert(nrVariables > 0 && nrMonomials > 0, "must atleast have one variable and one monomial");
    this->polynomial.clear();
    this->polynomial.resize(nrVariables * nrMonomials);
    for (size_t d = nrMonomials; d > 0; d--) {
      Assert((nrMonomials - d) * nrVariables < nrMonomials * nrVariables, "Array out of bounds");
      int32_t *monomial = &this->polynomial[(nrMonomials - d) * nrVariables];
      int32_t upper = d;
      for (size_t j = 0; j < nrVariables - 1; j++) {
        monomial[j] = abs(Random::Get().Binomial(upper * 2, 0.5)() - upper);
        upper -= monomial[j];
        if (upper == 0)
          break;
      }
      monomial[nrVariables - 1] = upper;
      this->Shuffel(nrVariables, monomial);
    }
  }

  template class StmtReplacer<symir::Expr>;
  template class StmtReplacer<symir::Term>;
  template class StmtReplacer<symir::Cond>;

  template<typename Node>
  void StmtReplacer<Node>::Visit(const Node &n) {
    StmtCopier::Visit(n);
  }

  template<typename Node>
  void StmtReplacer<Node>::ReplaceStmt(
      const symir::Stmt *s, std::function<bool(const Node *)> matchFunction,
      std::function<ExprID(symir::FunctBuilder *, symir::BlockBuilder *, const Node &, void **)>
          replaceFunction,
      double randThreshold
  ) {
    this->matchFunction = matchFunction;
    this->replaceFunction = replaceFunction;
    this->randThreshold = randThreshold;
    this->randUniform = Random::Get().UniformReal();
    this->hasReplaced = false;
    bool wasTarget =
        s->GetIRId() == symir::SymIR::SIR_TGT_GOTO || s->GetIRId() == symir::SymIR::SIR_TGT_BRA;
    size_t idx;
    for (idx = 0; idx < this->blockBd->GetNumberOfCommitedStmt(); idx++) {
      if (this->blockBd->GetCommitedStmt(idx) == s)
        break;
    }

    s->Accept(*this);
    // update stmt ptr since it has been replaced
    s = this->blockBd->GetCommitedStmtOrTarget(idx);

    if (!this->hasReplaced) {
      // if by change (e.g. randTheshold) we have not replaced anything we run it again with a
      // threshold of 1 to guarentee a replacement
      if (!wasTarget)
        popStmt();
      this->randThreshold = 1;
      s->Accept(*this);
    }

    Assert(
        this->hasReplaced,
        "StmtReplacer should only be called on stmt that are guaranteed to be able to be replaced"
    );

    if (!wasTarget) {
      // If s is not a target we want to Replace s with our new Stmt manually
      this->blockBd->ReplaceCommitStmt({popStmt()}, idx);
    }

    return;
  }

  template<typename Node>
  void StmtReplacer<Node>::Visit(const symir::Branch &b) {
    b.GetCond()->Accept(*this);
    auto condId = popCond();
    std::string tt = b.GetTrueTarget();
    std::string ft = b.GetFalseTarget();
    this->blockBd->RemoveTarget();
    this->blockBd->SymBranch(tt, ft, condId);
  }

  template<>
  void StmtReplacer<symir::Expr>::Visit(const symir::Expr &e) {
    if (this->Match(e)) {
      pushExpr(this->Replace(e));
    } else {
      StmtCopier::Visit(e);
    }
  }

  template<>
  void StmtReplacer<symir::Term>::Visit(const symir::Term &t) {
    if (this->Match(t)) {
      pushTerm(this->Replace(t));
    } else {
      StmtCopier::Visit(t);
    }
  }

  template<>
  void StmtReplacer<symir::Cond>::Visit(const symir::Cond &c) {
    if (this->Match(c)) {
      pushCond(this->Replace(c));
    } else {
      StmtCopier::Visit(c);
    }
  }

  std::vector<symir::Coef *> CopyAccess(symir::FunctBuilder *funBd, const symir::VarUse *use) {
    std::vector<symir::Coef *> access;
    access.reserve(use->GetAccess().size());
    for (const auto &coef: use->GetAccess()) {
      if (auto c = funBd->FindSymbol(coef->GetName()); c != nullptr) {
        Assert(
            typeid(*coef) == typeid(symir::Coef),
            "Symbol \"%s\" is already defined and is not a coefficient", coef->GetName().c_str()
        );
        access.push_back(dynamic_cast<symir::Coef *>(coef));
      } else {
        Panic("coeff not found in provided function builder");
      }
    }
    return access;
  }

  symir::BlockBuilder::CondID
  TriviallyCondFor(bool condTarget, symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd) {
    return blockBd->SymCond(
        symir::Cond::OP_EQZ,
        blockBd->SymAddExpr({blockBd->SymCstTerm(funBd->SymI32Const(condTarget ? 0 : 1), nullptr)})
    );
  }

  symir::BlockBuilder::StmtID TrivialAssignment(
      symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd, const symir::VarDef *var,
      std::vector<symir::Coef *> access
  ) {
    return blockBd->SymAssStmt(
        var,
        blockBd->SymAddExpr({blockBd->SymCstTerm(
            funBd->SymI32Const(Random::Get().Uniform(INT_MIN, INT_MAX)()), nullptr
        )}),
        access
    );
  }

  symir::BlockBuilder::TermID VariableTerm(
      symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd, const symir::VarDef *var,
      std::vector<symir::Coef *> access
  ) {
    switch (Random::Get().Uniform(0, 4)()) {
      case 0: { // symir::Term::OP_ADD
        return blockBd->SymAddTerm(funBd->SymI32Const(0), var, access);
      } break;
      case 1: { // symir::Term::OP_MUL:
        return blockBd->SymMulTerm(funBd->SymI32Const(1), var, access);
      } break;
      case 2: { // symir::Term::OP_AND:
        return blockBd->SymAndTerm(funBd->SymI32Const(-1), var, access);
      } break;
      case 3: { // symir::Term::OP_OR:
        return blockBd->SymOrTerm(funBd->SymI32Const(0), var, access);
      } break;
      case 4: { // symir::Term::OP_SHR:
        return blockBd->SymShrTerm(funBd->SymI32Const(0), var, access);
      } break;
      default:
        Panic("Not reachable");
    }
  }

  symir::Expr::Op RandomExprOp() {
    static auto r = Random::Get().Uniform(0, symir::Expr::Op::NUM_OPS - 1);
    return static_cast<symir::Expr::Op>(r());
  }

  symir::BlockBuilder *SsplitBlockAt(
      symir::FunctBuilder *funBd, symir::BlockBuilder *blockBd, ::std::string secondLabel,
      size_t splitIdx
  ) {
    Assert(
        splitIdx <= blockBd->GetNumberOfCommitedStmt(), "splitIdx out of bounds for block %s",
        blockBd->GetLabel().c_str()
    );

    symir::BlockBuilder *secondBlockBd = funBd->OpenBlock(secondLabel);
    symir::StmtCopier secondCopier = symir::StmtCopier(funBd, secondBlockBd);

    for (size_t i = splitIdx + 1; i < blockBd->GetNumberOfCommitedStmt(); i++) {
      secondBlockBd->CommitStmt(secondCopier.CopyStmt(blockBd->GetCommitedStmt(i)));
    }

    symir::Target *target = blockBd->GetTarget();
    if (target != nullptr) {
      if (target->GetIRId() == symir::SymIR::SIR_TGT_GOTO) {
        secondBlockBd->SymGoto(static_cast<symir::Goto *>(target)->GetTarget());
      } else if (target->GetIRId() == symir::SymIR::SIR_TGT_BRA) {
        symir::Branch *branch = static_cast<symir::Branch *>(target);
        secondBlockBd->SymBranch(
            branch->GetTrueTarget(), branch->GetFalseTarget(),
            secondCopier.CopyCond(branch->GetCond())
        );
      }
      blockBd->RemoveTarget();
    }

    if (splitIdx < blockBd->GetNumberOfCommitedStmt() - 1 &&
        blockBd->GetNumberOfCommitedStmt() != 0) {
      blockBd->RemoveCommittedStmts(splitIdx + 1, blockBd->GetNumberOfCommitedStmt());
    }

    return secondBlockBd;
  }

  // TODO: This could be optimized if perf. becomes an issue
  void InsertBlockBd(
      std::vector<symir::BlockBuilder *> &currBlockBds,
      std::vector<symir::BlockBuilder *> newBlocks, size_t index
  ) {
    int currIndex = index;
    for (size_t i = 0; i < newBlocks.size(); i++) {
      currBlockBds.insert(currBlockBds.begin() + currIndex++, std::move(newBlocks[i]));
    }
  }

  static std::map<std::pair<std::string, std::string>, size_t> labelNameCount;

  std::string NameLabel(std::string functName, std::string prefix) {
    std::pair namePair = std::make_pair(functName, prefix);
    if (!labelNameCount.contains(namePair))
      labelNameCount[namePair] = 0;
    return prefix + "_" + std::to_string(labelNameCount[namePair]++);
  }

  void ClearNameLabel() { labelNameCount.clear(); }

  static std::map<std::tuple<std::string, std::string, std::string, size_t>, size_t> varNameCount;

  std::string NameVariable(std::string functName, std::string domBlockName, std::string prefix) {
    std::tuple nametuple = std::make_tuple(functName, domBlockName, prefix, 1);
    if (!varNameCount.contains(nametuple))
      varNameCount[nametuple] = 0;
    return prefix + "_" + std::to_string(varNameCount[nametuple]++);
  }

  std::string
  NameVariable(std::string functName, std::string domBlockName, std::string prefix, size_t size) {
    std::tuple nametuple = std::make_tuple(functName, domBlockName, prefix, size);
    if (!varNameCount.contains(nametuple))
      varNameCount[nametuple] = 0;
    return prefix + "_" + std::to_string(size) + "_" + std::to_string(varNameCount[nametuple]++);
  }

  void ClearNameVariable() { varNameCount.clear(); }

  const symir::VarDef *GetVariable(
      symir::FunctBuilder *funBd, const std::string domBlockName, const std::string prefix
  ) {
    std::string name = NameVariable(funBd->GetName(), domBlockName, prefix);
    const symir::VarDef *var = funBd->FindLocal(name);
    if (var == nullptr)
      var = funBd->SymScaLocal(name, nullptr);
    return var;
  }

  const symir::VarDef *GetVariable(
      symir::FunctBuilder *funBd, std::string domBlockName, std::string prefix, size_t size
  ) {
    std::string name = NameVariable(funBd->GetName(), domBlockName, prefix, size);
    const symir::VarDef *var = funBd->FindLocal(name);
    if (var == nullptr)
      var = funBd->SymVecLocal(name, {(int) size}, {});
    return var;
  }

  bool NoAddOverflow(int32_t a, int32_t b) {
    int64_t longDiff = (static_cast<int64_t>(a) + static_cast<int64_t>(b));
    return (static_cast<int64_t>(INT_MIN) <= longDiff) &&
           (longDiff <= static_cast<int64_t>(INT_MAX));
  }

  bool NoSubOverflow(int32_t a, int32_t b) {
    int64_t longDiff = (static_cast<int64_t>(a) - static_cast<int64_t>(b));
    return (static_cast<int64_t>(INT_MIN) <= longDiff) &&
           (longDiff <= static_cast<int64_t>(INT_MAX));
  }

  using namespace patternmatch;

  bool MatchSubExprInAnyStmt(const symir::Stmt *stmt, const Pattern<const symir::Expr *> &E) {
    return patternmatch::match(
        stmt,
        m_Or<const symir::Stmt *>(
            m_Branch(m_Cond(E), m_WildCard<const std::string>(), m_WildCard<const std::string>()),
            m_AssStmt(m_WildCard<const symir::VarUse *>(), E)
        )
    );
  }
} // namespace transformations::utils
