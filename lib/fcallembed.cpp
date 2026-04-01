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

#include <flint/ulong_extras.h>
#include <flint/nmod.h>
#include <flint/nmod_mat.h>
#include <lib/random.hpp>
#include <lib/samputils.hpp>
#include <utility>

#include "global.hpp"
#include "lib/chksum.hpp"
#include "lib/fcallembed.hpp"
#include "lib/lang.hpp"
#include "lib/logger.hpp"

namespace {
  /// given a flattend intex recover the access vector needed to create a VarUse Object
  std::vector<symir::Coef *> unflattenAccess(symir::FunctBuilder *funBuilder, const symir::VarDef *var, size_t flattenedIndex) {
    if (var->GetType() == symir::SymIR::Type::I32) {
      Assert(
        flattenedIndex == 0,
        "Trying to index int32_to %s which is an (I32, %s) with %ld",
        var->GetName().c_str(),
        var->GetBaseType() == symir::SymIR::Type::I32 ? "i32" : (var->GetBaseType() == symir::SymIR::Type::ARRAY ? "ARRAY" : "STRUCT"),
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
        size_t type_size = symir::intSizeOfSymIRType(funBuilder->GetStructs(), baseType, baseType, {}, structName);
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
        for (const auto &field : sDef->GetFields()) {
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
          size_t field_size = intSizeOfSymIRType(funBuilder->GetStructs(), type, baseType, shape, structName);
          if (field_size > remainingIndex) {
            accessVals.push_back(fieldIdx);
            goto typeLoop;
          }
          remainingIndex -= field_size;
          fieldIdx += 1;
        }
      } break;
      default: Panic("Unknown or Int Type while finding Access Path");
      }
    }
    Assert(
      remainingIndex == 0,
      "Var Access has overflown with remainingIndex: %ld, on var: %s, struct: %s, flattendIndex: %ld",
      remainingIndex, var->GetName().c_str(), var->GetStructName().c_str(), flattenedIndex);

    std::vector<symir::Coef *> access;
    access.reserve(accessVals.size());
    // hacky counter to avoid "same named coefficient" error for polynomial coeffs
    static size_t unique_counter = 0;
    for (size_t i = 0; i < accessVals.size(); i++) {
      access.push_back(funBuilder->SymCoef(
        "poly_var_access_" + var->GetName() + "_" + std::to_string(flattenedIndex) + "_" + std::to_string(i) + "_" + std::to_string(unique_counter),
        std::to_string(accessVals[i])
      ));
    }
    unique_counter += 1;
    return access;
  }

// ==================== PrimeInterpolationHelper ====================

  class PrimeInterpolation {
  public:
    PrimeInterpolation(int32_t mod) {
      Assert(mod <= 46337, "To avoid overflow mod must be less then 46337");
      Assert(n_is_prime(mod), "PrimeInterpolation requires that mod is prime");
      nmod_init(&this->mod, mod);
    }
  
    void interpolate(size_t nrVariables, size_t nrIterations, std::vector<int32_t> varState, int32_t target) {
      Assert(0 <= target && target < static_cast<int32_t>(this->mod.n), "targets (%d) must be in Z_%ld", target, this->mod.n);
      Assert(nrVariables * nrIterations == varState.size(), "varState is the wrong size");
      Assert(nrVariables > 0 && nrIterations > 0, "must atleast have one variable and one monomial");
  
      Log::Get().OpenSection("Interpolation");
      this->randomizePolynomial(nrVariables, nrIterations);
  
      Log::Get().Out() << "Polynomial (" << this->polynomial.size() << "): ";
      for (size_t i = 0; i < this->polynomial.size(); i++) {
        Log::Get().Out() << this->polynomial[i] << " ";
      }
      Log::Get().Out() << std::endl;

      nmod_mat_t A;
      nmod_mat_t B;
      nmod_mat_t X;
      nmod_mat_init(A, nrIterations + 1, nrIterations + 1, this->mod.n);
      nmod_mat_init(B, nrIterations + 1, 1, this->mod.n);
      nmod_mat_init(X, nrIterations + 1, 1, this->mod.n);
  
      // set all entries of A
      for (size_t row = 0; row < nrIterations; row++) {
        for (size_t col = 0; col < nrIterations; col++) {
          ulong term = 1;
          for (uint32_t vari = 0; vari < nrVariables; vari++) {
            int32_t var = this->reduceMod(varState[row * nrVariables + vari]);
            ulong deg = static_cast<ulong>(this->polynomial[col * nrVariables + vari]);
            term = nmod_mul(term, nmod_pow_ui(var, deg, this->mod), this->mod);
          }
          nmod_mat_set_entry(A, row, col, term);
        }
        nmod_mat_set_entry(A, row, nrIterations, 1);
      }
  
      // fill the last row of the A matrix with unique new numbers to avoid the const polynomial
      std::vector<int32_t> unique_iter =
        this->findUniqueIteration(nrVariables, nrIterations, varState);
      for (size_t col = 0; col < nrIterations; col ++) {
        ulong term = 1;
        Assert(col * nrVariables < nrIterations * nrVariables, "Array access out of bounds");
        for (uint32_t i = 0; i < nrVariables; i++) {
          int32_t var = this->reduceMod(unique_iter[i]);
          ulong deg = static_cast<ulong>(this->polynomial[col * nrVariables + i]);
          term = nmod_mul(term, nmod_pow_ui(var, deg, this->mod), this->mod);
        }
        nmod_mat_set_entry(A, nrIterations, col, term);
      }
      nmod_mat_set_entry(A, nrIterations, nrIterations, 1);
  
      Log::Get().Out() << "Interpolation Matrix A["<< nrIterations + 1 << ", " << nrIterations + 1 << "]:" << std::endl;
      for (size_t row = 0; row < nrIterations + 1; row++) {
        Log::Get().Out() << "[";
        for (size_t col = 0; col < nrIterations + 1; col ++) {
          Log::Get().Out() << nmod_mat_get_entry(A, row, col);
          if (col != nrIterations) Log::Get().Out() << ", ";
        }
        Log::Get().Out() << "]" << std::endl;
      }
      Log::Get().Out() << "Rank(A) = " << nmod_mat_rank(A);

      // Fill B vector
      for (size_t row = 0; row < nrIterations; row++) {
        nmod_mat_set_entry(B, row, 0, target);
      }
      // Fill the last element of B with a unique new target value to avoid the const polynomial
      nmod_mat_set_entry(B, nrIterations, 0, nmod_add(static_cast<ulong>(target), 1, this->mod));
  
      // solve the system
      int32_t res = nmod_mat_can_solve(X, A, B);
      //TODO: I believe this can fail by change, maybe we can retry in this case (should not need many retries).
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
  
    void interpolate(size_t nrVariables, size_t nrIterations, std::vector<int32_t> varState, std::vector<int32_t> targets) {
      Assert(targets.size() == nrIterations, "Must have a target value for each iteration");
      Assert(nrVariables > 0 && nrIterations > 0, "must atleast have one variable and one monomial");

      bool has_unique = false;
      for (const int32_t target: targets) {
        Assert(0 <= target && target < static_cast<int32_t>(this->mod.n), "targets must be in Z_%d", static_cast<int32_t>(this->mod.n));
        has_unique |= targets[0] != target;
      }
      Assert(has_unique, "target must have atleast on unique element");
  
      this->randomizePolynomial(nrVariables, nrIterations - 1);
  
      nmod_mat_t A;
      nmod_mat_t B;
      nmod_mat_t X;
      nmod_mat_init(A, nrIterations, nrIterations, this->mod.n);
      nmod_mat_init(B, nrIterations, 1, this->mod.n);
      nmod_mat_init(X, nrIterations, 1, this->mod.n);
  
      // set all entries of A
      for (size_t row = 0; row < nrIterations; row++) {
        for (size_t col = 0; col < nrIterations - 1; col ++) {
          ulong term = 1;
          for (size_t vari = 0; vari < nrVariables; vari++) {
            int32_t var = this->reduceMod(varState[row * nrVariables + vari]);
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
  
    std::vector<int32_t> getPolynomial() {
      return std::vector(this->polynomial);
    }
  
    std::vector<int32_t> getCoeffs() {
      return std::vector(this->coeffs);
    }
  
    /// Triggers an assert if the last interpolation does not correctly yield 'target' when evaluated over 'varState'
    void assertCorrectness(size_t nrVariables, size_t nrIterations, std::vector<int32_t> varState, int32_t target) {
      Assert(0 <= target && target < static_cast<int32_t>(this->mod.n), "targets (%d) must be in Z_%ld", target, this->mod.n);
      Assert(nrVariables * nrIterations == varState.size(), "varState is the wrong size");
      Assert(nrVariables > 0 && nrIterations > 0, "must atleast have one variable and one monomial");

      Assert(this->polynomial.size() == nrVariables * nrIterations, "Interpolation not solved");
      Assert(this->coeffs.size() == nrIterations + 1, "Interpolation not solved or does not match input");

      for (size_t i = 0; i < nrIterations; i++) {
        int32_t result = 0;
        for (size_t m = 0; m < nrIterations; m++) {
          Assert(0 <= this->coeffs[m] && this->coeffs[m] < static_cast<int32_t>(this->mod.n), "Coef (%d) not in Z_%ld", this->coeffs[m], this->mod.n);
          int32_t term = this->coeffs[m];
          for (size_t j = 0; j < nrVariables; j++) {
            ulong var = this->reduceMod(varState[i * nrVariables + j]);
            Assert(
              0 <= this->polynomial[m * nrVariables + j] && this->polynomial[m * nrVariables + j] < static_cast<int32_t>(this->mod.n),
              "Coef (%d) not in Z_%ld", this->polynomial[m * nrVariables + j], this->mod.n
            );
            ulong deg = static_cast<ulong>(this->polynomial[m * nrVariables + j]);
            term = static_cast<int32_t>(nmod_mul(static_cast<ulong>(term), nmod_pow_ui(var, deg, this->mod), this->mod));
          }
          result = static_cast<int32_t>(nmod_add(static_cast<ulong>(result), static_cast<ulong>(term), this->mod));
        }
        Assert(
          0 <= this->coeffs[nrIterations] && this->coeffs[nrIterations] < static_cast<int32_t>(this->mod.n),
          "Coef (%d) not in Z_%ld", this->coeffs[nrIterations], this->mod.n
        );
        result = static_cast<int32_t>(nmod_add(static_cast<ulong>(result), static_cast<ulong>(this->coeffs[nrIterations]), this->mod));
        Assert(result == target, "interpolation is incorrect!");
      }
    }

    /// Triggers an assert if the last interpolation does not correctly yield 'targets' when evaluated over 'varState'
    void assertCorrectness(size_t nrVariables, size_t nrIterations, std::vector<int32_t> varState, std::vector<int32_t> targets) {
      bool has_unique = false;
      for (const int32_t target: targets) {
        Assert(0 <= target && target < static_cast<int32_t>(this->mod.n), "targets must be in Z_%d", static_cast<int32_t>(this->mod.n));
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
          Assert(0 <= this->coeffs[m] && this->coeffs[m] < static_cast<int32_t>(this->mod.n), "Coef (%d) not in Z_%ld", this->coeffs[m], this->mod.n);
          int32_t term = this->coeffs[m];
          for (size_t j = 0; j < nrVariables; j++) {
            ulong var = this->reduceMod(varState[i * nrVariables + j]);
            Assert(
              0 <= this->polynomial[m * nrVariables + j] && this->polynomial[m * nrVariables + j] < static_cast<int32_t>(this->mod.n),
              "Coef (%d) not in Z_%ld", this->polynomial[m * nrVariables + j], this->mod.n
            );
            ulong deg = static_cast<ulong>(this->polynomial[m * nrVariables + j]);
            term = static_cast<int32_t>(nmod_mul(static_cast<ulong>(term), nmod_pow_ui(var, deg, this->mod), this->mod));
          }
          result = static_cast<int32_t>(nmod_add(static_cast<ulong>(result), static_cast<ulong>(term), this->mod));
        }
        Assert(
          0 <= this->coeffs[nrIterations - 1] && this->coeffs[nrIterations - 1] < static_cast<int32_t>(this->mod.n),
          "Coef (%d) not in Z_%ld", this->coeffs[nrIterations - 1], this->mod.n
        );
        result = static_cast<int32_t>(nmod_add(static_cast<ulong>(result), static_cast<ulong>(this->coeffs[nrIterations - 1]), this->mod));
        Assert(result == targets[i], "interpolation is incorrect!");
      }
    }

  private:
  
    /// Find a new unused iteration according to varState
    std::vector<int32_t> findUniqueIteration(size_t nrVariables, size_t nrIterations, std::vector<int32_t> varState) {
      std::vector<int32_t> unique;
      std::vector<int32_t> currVarState;
      unique.reserve(nrVariables);
      currVarState.resize(nrIterations);
      for (size_t i = 0; i < nrVariables; i++) {
        // Copy and sort to make it easy to find new unique value
        for (size_t j = 0; j < nrIterations; j++) {
          int32_t var = varState[j * nrVariables + i];
          currVarState[j] = this->reduceMod(var);
        }
        std::sort(currVarState.begin(), currVarState.end());
  
        // To actually find a unique element we look for the largest difference between to successive elements.
        // This is to minimize the chance the AX = B is unsolvable
        int32_t last = 0;
        int32_t largest_range = 0;
        int32_t candidate = -1;
        for (size_t k = 0; k < nrIterations; k++) {
          // if the next value is not the successor of - or equal to the last one there must be a unique value inbetween
          if (currVarState[k] != static_cast<int32_t>((last + 1) % mod.n)
              && currVarState[k] - 1 - (last + 1) >= largest_range) {
            largest_range = currVarState[k] - 1 - (last + 1);
            candidate = Random::Get().Uniform(last + 1, currVarState[k] - 1)();
          }
          last = currVarState[k];
        }
        if (static_cast<int32_t>(mod.n - 1) - currVarState.back() >= largest_range) {
          largest_range = static_cast<int32_t>(mod.n - 1) - currVarState.back();
          candidate = Random::Get().Uniform(currVarState.back() + 1, static_cast<int32_t>(mod.n - 1))();
        }
        if (largest_range > 0) unique.push_back(candidate);
        else Panic("unable for find a unique element");
      }
      Assert(unique.size() == nrVariables, "unique array not of the right size");
      return unique;
    }
    
    /// see https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle
    void shuffel(const size_t n, int32_t *arr) {
      for (size_t i = n - 1; i >= 1; i--) {
        size_t r = rand() % (i + 1);
        int32_t temp = arr[r];
        arr[r] = arr[i];
        arr[i] = temp;
      }
    }
    
    // This is a quite biased in its selection. TODO: Are the more uniform / fair algorithms for finding a random polynomial fast?
    // Monomial ordering: https://people.math.sc.edu/Burkardt/c_src/monomial/monomial.html
    /// Samples a new random polynomial
    void randomizePolynomial(
      const size_t nrVariables,
      const size_t nrMonomials
    ) {
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
          if (upper == 0) break;
        }
        monomial[nrVariables - 1] = upper;
        this->shuffel(nrVariables, monomial);
      }
    }

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
} // namespace

// ==================== FCallStrategy Base Implementations ====================
void FCallStrategy::initialize(
  const symir::Funct *guest,
  const std::vector<ArgPlus<int32_t>> *init,
  const std::vector<ArgPlus<int32_t>> *fina
) {
  this->guest = guest;
  this->init = init;
  this->fina = fina;
}

void FCallStrategy::setTarget(const int32_t target) {
  this->emplaceTargetValue = target;
}

void FCallStrategy::setMaxNrBlocks(size_t nrBlocks) {
  this->argUsedMatrix.clear();
  this->argUsedMatrix.resize(nrBlocks);
  this->nrStmts = 1;
  this->nrBlocks = nrBlocks;
}


std::string FCallStrategy::wrapChecksum(int32_t checksum, std::string call) const {
  std::ostringstream res;
  res << StatelessChecksum::GetCheckChksumName()
      <<"(" 
      << checksum 
      << ", "
      << call
      << ")";
  return res.str();
}

const symir::VarDef *FCallStrategy::getUnusedAssignVar(symir::FunctBuilder *funBd, size_t blockIndex, size_t stmtIndex) {
  Assert(blockIndex < this->nrBlocks, "blockIndex must be less then the set max number of blocks");

  if (stmtIndex + 1 > nrStmts) {
    this->argUsedMatrix.resize(this->nrBlocks * (stmtIndex + 1));
    nrStmts = stmtIndex + 1;
  }

  // How many argument variables are already in use at this stmt
  size_t argUsed = this->argUsedMatrix[stmtIndex * this->nrBlocks + blockIndex];

  // How many argument variables exist in the current function
  auto locals = funBd->GetLocals();
  size_t argAvailable = 0;
  for (size_t i = 0; i < locals.size(); i++) {
    if (locals[i]->GetIRId() == symir::SymIR::SIR_LOCAL_UNINIT) {
      argAvailable += 1;
    }
  }

  const symir::VarDef *loc;
  if (argUsed >= argAvailable) {
    loc = funBd->SymUnInitLocal("arg_" + std::to_string(argUsed));
  } else {
    loc = funBd->FindVar("arg_" + std::to_string(argUsed));
  }
  Assert(loc != nullptr, "creation or search for local has failed");
  Log::Get().Out() << "AssignVariable: " << loc->GetName() << std::endl;;

  this->argUsedMatrix[stmtIndex * this->nrBlocks + blockIndex] += 1;
  return loc;
}

void FCallStrategy::appendVarState(VariableStateQuery *varStateQuery, size_t blockIndex, size_t stmtIndex) {
  // get the current variable state for all variables in scope
  auto varStatePair = varStateQuery->query(blockIndex, stmtIndex);
  size_t currVarStateSize;
  if (this->nrVariables == 0) {
    this->nrVariables = varStatePair.first;
    this->varState = varStatePair.second;
    currVarStateSize = this->varState.size();
  } else {
    Assert(this->nrVariables == varStatePair.first, "Different Queries have different nrVariables");
    this->varState.reserve(this->varState.size() + varStatePair.second.size());
    currVarStateSize = varStatePair.second.size();
    for (size_t i = 0; i < currVarStateSize; i++) {
      this->varState.push_back(varStatePair.second[i]);
    }
  }
  Assert(this->varState.size() % this->nrVariables == 0, "nrVariables * nr_iteration == varState.size must hold");
  this->nrIterations += currVarStateSize / this->nrVariables;
  
  Log::Get().Out() << "Variables State: (nrVariables: " << this->nrVariables << "), (nrIterations: " << this->nrIterations << ")" << std::endl;
  for (size_t i = 0; i < this->nrIterations; i++) {
    Log::Get().Out() << "Iteration " << i << ": ";
    for (size_t j = 0; j < this->nrVariables; j++) {
      Log::Get().Out() << this->varState[i * nrVariables + j];
      if (j == this->nrVariables - 1) {
        Log::Get().Out() << std::endl;
      } else {
        Log::Get().Out() << ", ";
      }
    }
  }
  Log::Get().Out() << std::endl;
}

void FCallStrategy::randomlyFilterVarState(symir::FunctBuilder *funBd) {
  auto randDouble = Random::Get().UniformReal();
  // randomly select a subset of variables and find there VarDef
  this->filteredVars.clear();
  this->filteredAccesses.clear();
  std::vector<size_t> indices;
  size_t lastVarStartIndex = 0;
  for (size_t i = 0; i < nrVariables; i++) {
    if (this->varMap.contains(i)) lastVarStartIndex = i;
    if (indices.size() > 0 && randDouble() > 1 - GlobalOptions::Get().VariableTakeProba) continue;
    indices.push_back(i);
    const symir::VarDef *var = funBd->FindVar(this->varMap[lastVarStartIndex]);
    this->filteredVars.push_back(var);
    this->filteredAccesses.push_back(unflattenAccess(funBd, var, i - lastVarStartIndex));
  }
  this->filteredNrVariables = this->filteredVars.size();

  Log::Get().Out() << "Using Variables (" << this->filteredNrVariables << "): ";
  for (size_t i = 0; i < this->filteredNrVariables; i++) {
    Log::Get().Out() << this->filteredVars[i]->GetName();
    if (this->filteredVars[i]->GetType() != symir::SymIR::I32) {
      Log::Get().Out() << "[";
      for (size_t j = 0; j < this->filteredAccesses[i].size() - 1; j++) {
        Log::Get().Out() << this->filteredAccesses[i][j]->GetI32Value() << ", ";
      }
      Log::Get().Out() << this->filteredAccesses[i].back()->GetI32Value() << "]";
    }
    Log::Get().Out() << "(" << indices[i] << "): [";
    for (size_t k = 0; k < this->nrIterations; k++) {
      Log::Get().Out() << this->varState[k * nrVariables + indices[i]];
      if (k == this->nrIterations - 1) {
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
  this->filteredVarState.reserve(this->nrIterations * this->filteredNrVariables);
  for (size_t i = 0; i < this->nrIterations; i++) {
    for (size_t j = 0; j < this->filteredNrVariables; j++) {
      this->filteredVarState.push_back(this->varState[i * this->nrVariables + indices[j]]);
    }
  }
}

void smartlyFilterVarState(const symir::FunctBuilder *funBd) { Panic("TODO: Not yet implemented"); }

// ==================== FCallEmbedder Base Implementations ====================
FCallEmbedder::FCallEmbedder(symir::Funct *const host): host(host) {
  Assert(
    host != nullptr,
    "The host function passed to the constructur is a nullptr"
  );
  for (auto &sym: host->GetSymbols()) {
    // Check to ensure all symbols are Coef
    Assert(
      sym->IsSolved(),
      "The symbol \"%s\" in the host function is not solved, cannot replace it",
      sym->GetName().c_str()
    );
    this->symbols[dynamic_cast<symir::Coef *>(sym)] = false;
  }
  this->createBuilder();
}

bool FCallEmbedder::embedGuest(
  symir::Funct *guest,
  const std::vector<ArgPlus<int32_t>> *init,
  const std::vector<ArgPlus<int32_t>> *fina
) {
  Assert(this->callGenStrategy != nullptr, "No embedding strategy choosen");
  Assert(this->varStateQueries != nullptr, "varStateQuery not initialized");
  Assert(guest != nullptr, "No valid guest to embed");
  Assert(init != nullptr, "No valid init to embed");
  Assert(fina != nullptr, "No valid fina to embed");


  this->callGenStrategy->initialize(guest, init, fina);
  this->succeeded = false;
  this->host->Accept(*this);


  return this->succeeded;
}

bool FCallEmbedder::wasMutated(symir::Coef *c) {
  auto it = symbols.find(c);
  Assert(
      it != symbols.end(),
      "The coefficient with name \"%s\" is not found in the host function's symbols",
      it->first->GetName().c_str()
  );
  return it->second;
}
void FCallEmbedder::markMutated(symir::Coef *c) {
  auto it = symbols.find(c);
  Assert(
      it != symbols.end(),
      "The coefficient with name \"%s\" is not found in the host function's symbols",
      it->first->GetName().c_str()
  );
  it->second = true;
}

// ==================== LiteralFCallStrategy Implementations ====================
void LiteralFCallStrategy::generatePreamble(
  std::vector<VariableStateQuery> *varStateQueries,
  symir::FunctBuilder *funBd,
  size_t blockIndex,
  size_t stmtIndex
) { /* Do Nothing */ }

void LiteralFCallStrategy::generatePostamble(
  std::vector<VariableStateQuery> *varStateQueries,
  symir::FunctBuilder *funBd,
  size_t blockIndex,
  size_t stmtIndex
) { /* Do Nothing */ }

std::string LiteralFCallStrategy::generateCall() {
  Assert(this->guest, "guest is not initialized");
  Assert(this->init, "init is not initialized");
  Assert(this->fina, "fina is not initialized");

  // Build call
  std::ostringstream fcall;
  fcall << this->guest->GetName() 
        << "(";
  const auto &params = this->guest->GetParams();
  for (int32_t i = 0; i < static_cast<int32_t>(init->size()); ++i) {
    const auto &p = params[i];
    const auto &arg = (*init)[i];
    fcall << arg.GetTypeCastStr(p) 
          << arg.ToCxStr();
    if (i < static_cast<int32_t>(init->size()) - 1) {
      fcall << ", ";
    }
  }
  fcall << ")";

  // Handle checksum
  int32_t checksum = StatelessChecksum::Compute(*this->fina);
  std::string chk_call = this->wrapChecksum(checksum, fcall.str());

  // Correct call result such that it matches the emplacedTargetValue
  // To avoid UBs, we'd use an upper type to save the result: long long here
  long long diff = static_cast<long long>(this->emplaceTargetValue)
                 - static_cast<long long>(checksum);
  if (
      diff >= static_cast<long long>(INT32_MIN) 
      && diff <= static_cast<long long>(INT32_MAX)
    ) {
    return "(" + chk_call + " + " + std::to_string(diff) + ")";
  } else {
    return "(int) ((long long)" + chk_call + " + " + std::to_string(diff) + "L)";
  }
}

// ==================== PrimeInterpFCallStrategy Implementations ====================
void PrimeInterpFCallStrategy::generatePreamble(
  std::vector<VariableStateQuery> *varStateQueries,
  symir::FunctBuilder *funBd,
  size_t blockIndex,
  size_t stmtIndex
) {
  Assert(this->guest, "guest is not initialized");
  Assert(this->init, "init is not initialized");
  Assert(this->fina, "fina is not initialized");
  Assert(blockIndex < funBd->GetBlocks().size(), "index %ld is Out of bound for blocks", blockIndex);

  Log::Get().OpenSection("generatePreable");

  // clear temporaries
  this->argVars.clear();
  this->varState.clear();
  this->varMap.clear();
  this->filteredVarState.clear();
  this->filteredVars.clear();
  this->filteredAccesses.clear();
  this->nrVariables = 0;
  this->nrIterations = 0;
  this->filteredNrVariables = 0;

  if (this->nrBlocks == 0) this->setMaxNrBlocks(funBd->GetBlocks().size());

  int32_t prime = 46337; //TODO: maybe allow users to choose 2 <= P <= 46337 via a global?
  PrimeInterpolation interpolGen = PrimeInterpolation(prime); 

  std::vector<const symir::Param *> params = funBd->GetParams();
  std::vector<const symir::Local *> locals = funBd->GetLocals();

  const symir::Block *targetBlock= funBd->GetBlocks()[blockIndex];
  Log::Get().Out() << "Targeting Block: " << targetBlock->GetLabel() << ", " << stmtIndex << "-th Statement" << std::endl;
  symir::BlockBuilder *blockBd = symir::BlockCopier(funBd, targetBlock).CopyAsBuilder();

  this->varMap = (*varStateQueries)[0].GetVarMap();
  for (size_t i = 0; i < varStateQueries->size(); i++) {
    this->appendVarState(&(*varStateQueries)[i], blockIndex, stmtIndex);
  }

  auto randDouble = Random::Get().UniformReal();
  auto randTarget = Random::Get().Uniform(0, prime - 1);
  size_t flattIndex = 0;
  for (size_t initIdx = 0; initIdx < this->init->size(); initIdx++) {
    const auto &arg = (*this->init)[initIdx];
    for (size_t argIdx = 0; argIdx < arg.getSize(); argIdx++) {
      flattIndex += 1;
      if (randDouble() <= GlobalOptions::Get().InitReplaceProba) continue;

      Log::Get().Out() << "Replacing the " << flattIndex << "-th argument" << std::endl;

      this->randomlyFilterVarState(funBd);

      // We need a target value in Z_p to achive this we choose one randomly and then figure out how to correct for it.
      // TODO: iterate through init not just the first element
      int32_t target = arg.IsScalar() ? arg.GetValue() : arg.GetValue(argIdx);
      int32_t interpolTarget;
      if (target == INT32_MIN) {
        // avoid the div by 0 case of the else stmt
        interpolTarget = 0;
      } else {
        interpolTarget = randTarget() % static_cast<int32_t>((-static_cast<int64_t>(INT32_MIN)) + target);
      }

      Log::Get().Out() << "Target: " << target << ", Interpolation Target: " << interpolTarget << std::endl;

      interpolGen.interpolate(this->filteredNrVariables, this->nrIterations, this->filteredVarState, interpolTarget);
      std::vector<int32_t> polynomial = interpolGen.getPolynomial();
      std::vector<int32_t> coeffsVals = interpolGen.getCoeffs();
      interpolGen.assertCorrectness(filteredNrVariables, nrIterations, filteredVarState, interpolTarget);

      // we need coeff object to hand to the builder
      std::vector<symir::Coef *> coeffs{};
      coeffs.reserve(coeffsVals.size());
      for (const int32_t c : coeffsVals) {
        coeffs.push_back(funBd->SymI32Const(c));
      }

      const symir::VarDef *loc = this->getUnusedAssignVar(funBd, blockIndex, stmtIndex);

      auto assignment = blockBd->SymModAssignAt(
        loc,
        blockBd->SymModExpr(coeffs, this->filteredVars, this->filteredAccesses, polynomial, prime),
        {},
        stmtIndex
      );
      Assert(assignment != nullptr, "Failed to create a ModAssignment, likely by a failed dynamic_cast");
      this->argVars[flattIndex - 1] = std::make_pair(assignment->GetVar()->GetName(), target - interpolTarget);
      
      Log::Get().Out() << std::endl;
    }
  }
  funBd->ReplaceOrCloseBlock(blockBd);
  Log::Get().CloseSection();
}

void PrimeInterpFCallStrategy::generatePostamble(
  std::vector<VariableStateQuery> *varStateQueries,
  symir::FunctBuilder *funBd,
  size_t blockIndex,
  size_t stmtIndex
) { /* Do Nothing */ }

std::string PrimeInterpFCallStrategy::generateCall() {
  Assert(this->guest, "guest is not initialized");
  Assert(this->init, "init is not initialized");
  Assert(this->fina, "fina is not initialized");

  int32_t checksum = StatelessChecksum::Compute(*this->fina);
  std::ostringstream fcall;
  fcall << this->guest->GetName() 
        << "(";

  const auto &params = this->guest->GetParams();
  size_t flattenedIndex = 0;
  for (int32_t i = 0; i < static_cast<int32_t>(init->size()); ++i) {
    const auto &p = params[i];
    const auto &arg = (*this->init)[i];

    std::map<size_t, std::pair<std::string, int32_t> *> replacers;
    for (size_t argIdx = 0; argIdx < arg.getSize(); argIdx++) {
      if (this->argVars.contains(flattenedIndex)) {
        replacers[argIdx] = &this->argVars[i];
      }
      flattenedIndex += 1;
    }
    fcall << arg.GetTypeCastStr(p) 
          << arg.ToCxStrWithReplaced(replacers);

    if (i < static_cast<int32_t>(this->init->size()) - 1) {
      fcall << ", ";
    }
  }
  fcall << ")";
  std::string chk_call = this->wrapChecksum(checksum, fcall.str());
  // To avoid UBs, we'd use an upper type to save the result: long long here
  long long diff = static_cast<long long>(this->emplaceTargetValue)
                 - static_cast<long long>(checksum);
  if (
      diff >= static_cast<long long>(INT32_MIN) 
      && diff <= static_cast<long long>(INT32_MAX)
    ) {
    return "(" + chk_call + " + " + std::to_string(diff) + ")";
  } else {
    return "(int) ((long long)" + chk_call + " + " + std::to_string(diff) + "L)";
  }
}


// ==================== RandomFCallEmbedder Implementations ====================
void RandomFCallEmbedder::createPathBlockWhitelist() {
  Assert(this->varStateQueries != nullptr, "varStateQueries must be set to create white list");
  this->blockIndicesWhitelist.clear();
  for (size_t i = 0; i < this->varStateQueries->size(); i++) {
    const auto indices = (*this->varStateQueries)[0].getPathBlocksIndices();
    this->blockIndicesWhitelist.resize(this->blockIndicesWhitelist.size() + indices.size());
    for (size_t j = 0; j < indices.size(); j++) {
      this->blockIndicesWhitelist.push_back(indices[j]);
    }
  }
}

void RandomFCallEmbedder::Visit(const symir::VarUse &v) {
  if (this->succeeded) {
    return;
  }
  if (v.IsVector()) {
    for (const auto *c: v.GetAccess()) {
      c->Accept(*this);
      if (this->succeeded) {
        return;
      }
    }
  }
}

void RandomFCallEmbedder::Visit(const symir::Coef &c) {
  if (this->succeeded) {
    return;
  }
  auto *pc = const_cast<symir::Coef *>(&c);
  if (wasMutated(pc)) {
    // If the coefficient is already replaced, we do not need to do anything
    return;
  }
  // Replace the coefficient with a call to the function
  Assert(
    pc->GetType() == symir::SymIR::I32,
    "Unsupported type %s for the coefficient with name \"%s\"",
    symir::SymIR::GetTypeName(pc->GetType()).c_str(), pc->GetName().c_str()
  );

  this->callGenStrategy->setTarget(pc->GetI32Value());

  this->callGenStrategy->generatePreamble(this->varStateQueries, this->hostBuilder.get(), this->current_block, this->current_stmt);
  this->hostBuilder->FindSymbol(pc->GetName())->SetValue(this->callGenStrategy->generateCall());
  this->callGenStrategy->generatePostamble(this->varStateQueries, this->hostBuilder.get(), this->current_block, this->current_stmt);

  markMutated(pc);
  this->succeeded = true;
 }

 void RandomFCallEmbedder::Visit(const symir::Term &t) {
  if (this->succeeded) {
    return;
  }
  t.GetCoef()->Accept(*this);
  if (this->succeeded) {
    return;
  }
  if (t.GetVar() != nullptr) {
    t.GetVar()->Accept(*this);
  }
}

void RandomFCallEmbedder::Visit(const symir::ModExpr &e) { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::Expr &e) {
  if (this->succeeded) {
    return;
  }
  for (const auto *term: e.GetTerms()) {
    if (this->succeeded) {
      return;
    }
    term->Accept(*this);
  }
}

void RandomFCallEmbedder::Visit(const symir::Cond &c) {
 if (this->succeeded) {
   return;
 }
 c.GetExpr()->Accept(*this);
}
void RandomFCallEmbedder::Visit(const symir::ModAssStmt &a) { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::AssStmt &a) {
  if (this->succeeded) {
    return;
  }
  a.GetExpr()->Accept(*this);
  if (this->succeeded) {
    return;
  }
  a.GetVar()->Accept(*this);
 }
 void RandomFCallEmbedder::Visit(const symir::RetStmt &r) { /* Do Nothing */ }
 void RandomFCallEmbedder::Visit(const symir::Branch &b) {
  if (this->succeeded) {
    return;
  }
  b.GetCond()->Accept(*this);
 }
void RandomFCallEmbedder::Visit(const symir::Goto &g)        { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::ScaParam &p)    { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::VecParam &p)    { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::StructParam &p) { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::UnInitLocal &l)    { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::ScaLocal &l)    { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::VecLocal &l)    { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::StructLocal &l) { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::StructDef &s)   { /* Do Nothing */ }

void RandomFCallEmbedder::Visit(const symir::Block &b) {
  const auto stmts = b.GetStmts();
  const auto rand = Random::Get().Uniform(0, static_cast<int32_t>(stmts.size()) - 1);
  const auto randDouble = Random::Get().UniformReal();

  for (int32_t tries = 0; tries < 10; tries++) {
    const int32_t index = rand();
    const auto stmt = stmts[index];
    // Idea: 80% of the replacements should be in the conditional statements,
    // and the rest 20% in those assignment statements.
    double threshold;
    switch (stmt->GetIRId()) {
      case symir::SymIR::SIR_TGT_BRA:
        threshold = 0.8;
        break;
      case symir::SymIR::SIR_STMT_ASS:
        threshold = 0.2;
        break;
      default:
        threshold = 0;
        break;
    }
    if (randDouble() < threshold) {
      this->current_stmt = index;
      stmt->Accept(*this);
      if (this->succeeded) {
        return;
      }
    }
  }
}

void RandomFCallEmbedder::Visit(const symir::Funct &f) {
  const auto blocks = f.GetBlocks();
  // We could remove duplicates from the path but not doing so gives a bias to blocks inside loops which is not totally undesirable.
  const auto rand = Random::Get().Uniform(0, static_cast<int32_t>(this->blockIndicesWhitelist.size()) - 1);
  // Sample a statement from the list of statements for replacement
  for (int32_t tries = 0; tries < 100; tries++) {
    const size_t index = this->blockIndicesWhitelist[rand()];
    Assert(index < blocks.size(), "whitelisted index out of bounds");
    this->current_block = index;
    blocks[index]->Accept(*this);
    if (this->succeeded) {
      return;
    }
  }
}
