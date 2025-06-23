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

#include "lib/ubfexec.hpp"

#include "global.hpp"
#include "lib/logger.hpp"
#include "lib/naming.hpp"
#include "lib/random.hpp"
#include "lib/samputils.hpp"

int UBFreeExec::Solve(
    int numMap, bool withInterestInit, bool withRandomInit, bool withInterestCoefs
) {
  Assert(numMap > 0, "Number of mappings must be greater than 0");

  Log::Get().OpenSection("UBFreeExec: Solve");

  Log::Get().Out() << "Execution Path: ";
  for (int i = 0; i < static_cast<int>(execution.size()); i++) {
    Log::Get().Out() << execution[i];
    if (i != execution.size() - 1) {
      Log::Get().Out() << "->";
    }
  }
  Log::Get().Out() << std::endl;

  // Try to generate a couple of init-final mapping
  for (int i = 0; i < numMap; i++) {
    Log::Get().OpenSection("UBFreeExec: Iteration " + std::to_string(i));
    if (!solve(withInterestInit, withRandomInit, withInterestCoefs)) {
      std::cout << "WARNING: UNSAT at " << i << "-th initialization" << std::endl;
      return i; // There's no need to report an error to our caller
    }
    Log::Get().CloseSection();
  }

  Log::Get().CloseSection();

  return numMap;
}

bool UBFreeExec::solve(bool withInterestInit, bool withRandomInit, bool withInterestCoefs = true) {
  // clang-format off
  // Our constraint generation follow some rules:
  // 1. When the initializations vector is empty, we need to generate the constraints from scratch.
  // 2. When the initializations vector has only one element, we need to re-generate the constraints with using some symbols that we already resolved. We also need to ensure the new initialization are sufficiently different from the previous one.
  // 3. Otherwise, we only need to ensure that the new initializations are sufficiently different from the previous ones
  // clang-format on

  // Generate or re-generate the constraints for the function execution
  if (initializations.size() <= 1) {
    Log::Get().Out() << "Generating or re-generating UB-free constraints" << std::endl;
    solver.reset();
    versions.clear();

    // Let each parameter (coefficient or constant) interesting initially
    if (withInterestInit) {
      solver.add(makeInitInteresting());
    }
    if (withRandomInit) {
      solver.add(makeInitWithRandomValue());
    }

    Log::Get().Out() << "Initial constraints: " << std::endl
                     << "```" << std::endl
                     << solver.to_smt2() << "```" << std::endl;

    // Generate UB constraint for each statement in the executed basic block
    for (int i = 0; i < execution.size() - 1; i++) {
      // Generate constraints for the current basic block so that
      // no UB occurs and it can reach the next basic block
      generateConstraintsForBasicBlock(execution[i], execution[i + 1], withInterestCoefs);
    }
    generateConstraintsForBasicBlock(execution[execution.size() - 1], -1, withInterestCoefs);
  }

  // Ensure that the initialisation is sufficiently different from the previous one
  if (!initializations.empty()) {
    solver.add(differentiateInitFrom(initializations.back()));
  }

  // Dump the SMT queries for debugging
  Log::Get().Out() << "Final constraints: " << std::endl
                   << "```" << std::endl
                   << solver.to_smt2() << "```" << std::endl;

  // Solve the generated constraints and see if we succeeded
  if (solver.check() == z3::unsat) {
    Log::Get().Out() << "UNSAT" << std::endl;
    return false;
  }

  Log::Get().Out() << "SAT" << std::endl;

  z3::model model = solver.get_model();
  Log::Get().Out() << "Execution model: ```" << std::endl
                   << model << std::endl
                   << "```" << std::endl;

  // Extract values for the resolved symbols to facilitate subsequent solving
  extractSymbolsFromModel(model);
  // Insert values for unresolved symbols in unexecuted blocks
  insertUBsIntoUnexecutedBbls();
  // Extract the initialisation and finalisation values from the model
  extractInitFromModel(model);
  extractFinalFromModel(model);

  return true;
}

void UBFreeExec::generateConstraintsForBasicBlock(int u, int v, bool withInterestCoefs) {
  auto &bblU = fun.GetBbl(u);

  // UB checks for the assignment statements and conditional. Each term is either a
  // multiplication of a coefficient and a variable, or a constant coefficient.

  // Ensure all terms in each variable definition are free of undefined behaviours
  {
    int stmtIndex = 0;
    for (const int varIndex: bblU.GetDefs()) {
      z3::expr sum = generateConstraintsForTermSum(
          u, stmtIndex, false, bblU.GetDeps(varIndex), withInterestCoefs
      );
      // An assignment makes a new SSA variable with the same name but updated version
      z3::expr newVar = createVarExpr(varIndex, ++versions[varIndex]);
      solver.add(newVar == sum);
      stmtIndex++;
    }
  }

  if (bblU.GetSuccessors().size() <= 1) {
    return; // We don't need any further constraints for unconditional jumps
  }

  Assert(v != -1, "The target successor of %d (with 2 successors) cannot be -1", u);
  Assert(
      // The target v must be either the exit block (because we have pass counters)
      // or one of our successors already defined in our CFG.
      (v == fun.NumBbls() - 1 && passCounterBbl == u) ||
          std::ranges::find(bblU.GetSuccessors(), v) != bblU.GetSuccessors().end(),
      "The target successor %d of basic block %d is not a valid successor", v, u
  );

  // Ensure all terms in the conditional are free of undefined behaviours
  const z3::expr sum =
      generateConstraintsForTermSum(u, 0, true, bblU.GetCondDeps(), withInterestCoefs);
  // The jump: We always jump to the first successor for sum >= 0
  if (v == bblU.GetSuccessors()[0]) {
    solver.add(sum >= 0);
  } else {
    solver.add(sum < 0);
  }
}

z3::expr UBFreeExec::generateConstraintsForTermSum(
    // clang-format off
    int blkIndex, int stmtIndex, bool isInCond,
    const std::vector<int> &dependencies,
    bool withInterestCoefs
    // clang-format on
) {
  const auto nameCoef = isInCond ? NameCondCoef : NameCoef;
  const auto nameConst = isInCond ? NameCondConst : NameConst;

  // Define integer variables and coefficients
  std::vector<z3::expr> vars;  // [vi] for variables
  std::vector<z3::expr> coefs; // [ci] for coefficients
  std::vector<z3::expr> terms; // [ci * ai] or [ci] for the final addition

  // Taking care of coefficients and the multiplication
  for (int i = 0; i < dependencies.size(); i++) {
    vars.push_back(createVarExpr(dependencies[i]));
    coefs.push_back(createCoefExpr(nameCoef(blkIndex, stmtIndex, i)));
    terms.push_back(coefs.back() * vars.back()); // a_i * var_i
  }
  solver.add(boundVariables(vars));
  solver.add(boundCoefficients(coefs));

  // Taking care of the constant coefficient
  coefs.push_back(createCoefExpr(nameConst(blkIndex, stmtIndex)));
  terms.push_back(coefs.back());
  solver.add(
      coefs.back() >= GlobalOptions::Get().LowerBound &&
      coefs.back() <= GlobalOptions::Get().UpperBound
  );
  solver.add(boundTerms(terms));

  // Ensure that the coefficients are interesting
  if (withInterestCoefs) {
    solver.add(makeCoefsInteresting(coefs));
  }

  // Taking care of the terms and their addition
  z3::expr sum = terms[0];
  for (int i = 1; i < terms.size(); i++) {
    // Addition is left associative, so in an addition of k terms, the subexpressions with the
    // first 1, 2, ..., k terms added should all be between LOWER_BOUND and UPPER_BOUND as well
    sum = sum + terms[i];
    if (GlobalOptions::Get().EnableSafetyChecks) {
      solver.add(sum <= GlobalOptions::Get().UpperBound && sum >= GlobalOptions::Get().LowerBound);
    }
  }

  // Return the sum of the terms
  return sum;
}

void UBFreeExec::fixUnterminatedExecution() {
  const int endBbl = fun.NumBbls() - 1;
  const int stopBlk = execution.back();
  if (stopBlk == endBbl) {
    return;
  }
  // This means that the stop node is not the end node, probably because we're
  // going in a cycle, so we need to add a pass counter to the stop node (as
  // if we're artificially creating an edge to the end node).
  // Modify the basic block at the end of the sample walk to have pass counter
  // equal to the number of times that basic block has been visited.
  int passCounter =
      static_cast<int>(std::ranges::count_if(execution, [=](auto n) { return n == stopBlk; }));
  passCounterBbl = stopBlk;
  DefineSym(NamePassCounter(), passCounter);
  execution.push_back(endBbl);
  Log::Get().Out() << "Sample walk has been modified to end at the last basic block." << std::endl;
}

z3::expr UBFreeExec::makeInitInteresting() {
  std::vector<z3::expr> params;
  for (int i = 0; i < fun.NumParams(); i++) {
    params.push_back(createVarExpr(i, 0));
  }
  return AtMostKZeroes(ctx, params, fun.NumParams() / 2);
}

z3::expr UBFreeExec::makeInitWithRandomValue() {
  auto rand = Random::Get().Uniform(
      GlobalOptions::Get().LowerInitBound, GlobalOptions::Get().UpperInitBound
  );
  std::vector<z3::expr> params;
  z3::expr constraints = ctx.bool_val(true);
  for (int i = 0; i < fun.NumParams(); i++) {
    params.push_back(createVarExpr(i, 0));
    const std::string &paramName = params.back().decl().name().str();
    z3::expr randomInit = ctx.bool_val(true);
    if (SymDefined(paramName)) {
      randomInit = (params[i] == GetSymVal(paramName));
    } else {
      int randomValue = rand();
      DefineSym(paramName, randomValue);
      randomInit = (params[i] == randomValue);
    }
    constraints = constraints && randomInit;
  }
  return constraints;
}

z3::expr UBFreeExec::differentiateInitFrom(std::vector<int> initialisation) {
  // There should be at lease NUM_VAR/2 variables which are not equal in both initialisations
  std::vector<z3::expr> constraints;
  for (int i = 0; i < fun.NumParams(); i++) {
    int oldValue = initialisation[i];
    z3::expr newValue = createVarExpr(i, 0);
    constraints.push_back(z3::ite(oldValue != newValue, ctx.int_val(1), ctx.int_val(0)));
  }
  return AtMostKZeroes(ctx, constraints, std::min(3, fun.NumParams() / 2));
}

void UBFreeExec::extractSymbolsFromModel(z3::model &model) {
  for (const auto &param: symbols) {
    const std::string &symName = param.first;
    const auto symKey = ctx.int_const(symName.c_str()).decl();
    if (SymDefined(symName)) {
      continue;
    }
    if (model.has_interp(symKey)) {
      z3::expr symConst = model.get_const_interp(symKey);
      Assert(symConst.is_numeral(), "Symbol %s is not a numeral", symName.c_str());
      int symVal;
      Assert(
          Z3_get_numeral_int(ctx, symConst, &symVal),
          "Failed to obtain the symVal of symbol %s from the model", symName.c_str()
      );
      DefineSym(symName, symVal);
      Log::Get().Out() << "Extract symbols: sym=" << symName << ", value=" << symVal << std::endl;
    } else {
      Log::Get().Out() << "Extract symbols: sym=" << symName << ", value=<unresolved>" << std::endl;
    }
  }
}

void UBFreeExec::extractInitFromModel(z3::model &model) {
  std::vector<int> init;
  for (int i = 0; i < fun.NumParams(); i++) {
    z3::expr paramKey = createVarExpr(i, 0);
    const auto paramName = paramKey.decl().name().str().c_str();
    Assert(model.has_interp(paramKey.decl()), "Parameter %s is not found in model", paramName);
    z3::expr paramConst = model.get_const_interp(paramKey.decl());
    Assert(paramConst.is_numeral(), "Parameter %s is not a numeral", paramName);
    int paramVal;
    Assert(
        Z3_get_numeral_int(ctx, paramConst, &paramVal),
        "Failed to obtain the value of parameter %s from the model", paramName
    );
    init.push_back(paramVal);
    Log::Get().Out() << "Extract initialization: param=" << paramName << ", value=" << paramVal
                     << std::endl;
  }
  initializations.push_back(init);
}

void UBFreeExec::extractFinalFromModel(z3::model &model) {
  std::vector<int> fina;
  for (int paramIndex = 0; paramIndex < fun.NumParams(); paramIndex++) {
    z3::expr paramKey = createVarExpr(paramIndex);
    const auto paramName = paramKey.decl().name().str().c_str();
    Assert(model.has_interp(paramKey.decl()), "Parameter %s is not found in model", paramName);
    z3::expr paramConst = model.get_const_interp(paramKey.decl());
    Assert(paramConst.is_numeral(), "Parameter %s is not a numeral", paramName);
    int paramVal;
    Assert(
        Z3_get_numeral_int(ctx, paramConst, &paramVal),
        "Failed to obtain the value of parameter %s from the model", paramName
    );
    fina.push_back(paramVal);
    Log::Get().Out() << "Extract finalization: param=" << paramName << ", value=" << paramVal
                     << std::endl;
  }
  finalizations.push_back(fina);
}

z3::expr UBFreeExec::createVarExpr(const int varIndex, int version) {
  if (version == -1) {
    version = versions[varIndex];
  }
  return ctx.int_const(NameVar(varIndex, version).c_str());
}

z3::expr UBFreeExec::boundVariables(const std::vector<z3::expr> &vars) {
  z3::expr constraints = ctx.bool_val(true);
  for (const auto &var: vars) {
    constraints = constraints && (var >= GlobalOptions::Get().LowerBound &&
                                  var <= GlobalOptions::Get().UpperBound);
  }
  return constraints;
}

z3::expr UBFreeExec::createCoefExpr(const std::string &name) {
  // If we've already extracted the value from the model, we use that value,
  // otherwise it's an uninterpreted constant which the solver needs to figure
  // out the value for
  if (SymDefined(name)) {
    return ctx.int_val(GetSymVal(name));
  } else {
    InitSym(name);
    return ctx.int_const(name.c_str());
  }
}

z3::expr UBFreeExec::makeCoefsInteresting(const std::vector<z3::expr> &coefs) {
  return AtMostKZeroes(ctx, coefs, static_cast<int>(coefs.size()) / 2);
}

z3::expr UBFreeExec::boundCoefficients(const std::vector<z3::expr> &coefs) {
  z3::expr constraints = ctx.bool_val(true);
  for (const auto &coef: coefs) {
    if (coef.is_const()) {
      constraints = constraints && (coef >= GlobalOptions::Get().LowerCoefBound &&
                                    coef <= GlobalOptions::Get().UpperCoefBound);
    }
  }
  return constraints;
}

z3::expr UBFreeExec::boundTerms(const std::vector<z3::expr> &terms) {
  z3::expr constraints = ctx.bool_val(true);
  for (const auto &term: terms) {
    constraints = constraints && (term <= GlobalOptions::Get().UpperBound &&
                                  term >= GlobalOptions::Get().LowerBound);
  }
  return constraints;
}

void UBFreeExec::insertUBsIntoUnexecutedBbls() {
  // TODO: insert UBs into unexecuted basic blocks
  // for (int i = 0; i < fun.NumBbls(); i++) {
  //   if (std::ranges::find(execution, i) == execution.end()) {
  //     insertUBsIntoBbl(i);
  //   }
  // }

  // For now, we just define all symbols in the function with a random value
  const auto rand = Random::Get().Uniform(
      GlobalOptions::Get().LowerCoefBound, GlobalOptions::Get().UpperCoefBound
  );
  for (const auto &sym: fun.GetSymbols()) {
    if (SymDefined(sym)) {
      continue; // If the symbol is already defined, we don't need to smash it
    }
    const int val = rand();
    DefineSym(sym, val);
    Log::Get().Out() << "Define symbols: sym=" << sym << ", val=" << val
                     << " (for unexecuted basic blocks)" << std::endl;
  }
}
