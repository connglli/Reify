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

#include <ranges>
#include "global.hpp"
#include "lib/logger.hpp"
#include "lib/random.hpp"
#include "lib/samputils.hpp"
#include "lib/strutils.hpp"
#include "lib/ubinject.hpp"

const int UBFreeExec::PassCounterBblId =
    2147483647; // A large number to avoid conflicts with real basic block IDs
const std::string UBFreeExec::PassCounterBblLabel = NameLabel(UBFreeExec::PassCounterBblId);

UBFreeExec::UBFreeExec(const FunPlus &fun, const std::vector<int> &execution) {
  this->owner = &fun;
  // Detect unterminated execution and fix it to terminate
  const int exitBbl = fun.GetExitBblId();
  const int stopBbl = execution.back();
  if (stopBbl == exitBbl) {
    this->fun = fun.GetFun();
    this->execution = execution;
  } else {
    Log::Get().OpenSection("UBFreeExec: Fixing Unterminated Execution");
    // Whether we need a pass counter for the execution.
    // This help ensure we reach the end of a function for unterminated executions.
    // This means that the stop node is not the end node, probably because we're
    // going in a cycle, so we need to add a pass counter to the stop node (as
    // if we're artificially creating an edge to the end node).
    // Modify the basic block at the end of the sample walk to have pass counter
    // equal to the number of times that basic block has been visited.
    int passCounterVal =
        static_cast<int>(std::ranges::count_if(execution, [=](auto n) { return n == stopBbl; }));

    Log::Get().Out() << "Adding pass counter to the stop block " << NameLabel(stopBbl) << std::endl;
    Log::Get().Out() << "The pass counter's block is " << PassCounterBblLabel << std::endl;
    Log::Get().Out() << "The pass counter's value is " << passCounterVal << std::endl;

    // Cooy the function into a builder to modify it
    auto funBd = symir::FunctCopier(fun.GetFun()).CopyAsBuilder();
    const auto *stopBlock = funBd->FindBlock(NameLabel(stopBbl));
    Assert(stopBlock != nullptr, "Stop block %d does not exist in the function", stopBbl);
    const auto *exitBlock = funBd->FindBlock(NameLabel(exitBbl));
    Assert(exitBlock != nullptr, "Exit block %d does not exist in the function", exitBbl);

    // Insert a basic block to count and check the pass counter right before the stop block
    auto passCounterBlockBd = funBd->OpenBlock(PassCounterBblLabel);
    auto passCounter = funBd->SymLocal("passCounterLocal", funBd->SymCoef("zero", "0"));
    passCounterBlockBd->SymAssign(
        passCounter, passCounterBlockBd->SymAddExpr(
                         {passCounterBlockBd->SymMulTerm(funBd->SymCoef("one1", "1"), passCounter),
                          passCounterBlockBd->SymCstTerm(funBd->SymCoef("one2", "1"), nullptr)}
                     )
    );
    // Jump to the exit block if the pass counter is greater than the value
    passCounterBlockBd->SymBranch(
        exitBlock->GetLabel(), stopBlock->GetLabel(),
        passCounterBlockBd->SymGtzCond(passCounterBlockBd->SymAddExpr(
            {passCounterBlockBd->SymMulTerm(funBd->SymCoef("one3", "1"), passCounter),
             passCounterBlockBd->SymCstTerm(
                 funBd->SymCoef("passCounterValue", std::to_string(-passCounterVal + 1)), nullptr
             )}
        ))
    );
    funBd->CloseBlockAt(passCounterBlockBd, stopBlock);

    // Updates the targets of the existing basic blocks in the function
    // If they point to the stop block, redirect them to the pass counter block
    for (auto *blk: funBd->GetBlocks()) {
      if (blk->GetLabel() == PassCounterBblLabel) {
        continue; // Skip the pass counter block itself
      }
      const auto *target = blk->GetTarget();
      if (target != nullptr && target->HasTarget(stopBlock->GetLabel())) {
        // FIXME: This is a workaround for the fact that the target is const.
        (const_cast<symir::Target *>(target))
            ->ReplaceTarget(stopBlock->GetLabel(), PassCounterBblLabel);
      }
    }

    // Build the function with the pass counter block
    this->fun = funBd->Build().release();

    Log::Get().Out() << "Modifying the sample execution to early stop." << std::endl;
    Log::Get().Out() << "Before modification: " << JoinInt(execution, "->") << std::endl;

    // Update the execution path to include the pass counter block
    this->execution = std::vector<int>();
    for (const auto b: execution) {
      if (b == stopBbl) {
        this->execution.push_back(PassCounterBblId);
      }
      this->execution.push_back(b);
    }
    this->execution.pop_back();         // Remove the last element, which is the stop block
    this->execution.push_back(exitBbl); // Add the exit block at the end to early stop

    Log::Get().Out() << "After modification: " << JoinInt(this->execution, "->") << std::endl;

    Log::Get().CloseSection();
  }
  this->executionByLabels = std::vector<std::string>(this->execution.size());
  std::ranges::transform(this->execution, this->executionByLabels.begin(), [](int idx) {
    return idx == PassCounterBblId ? PassCounterBblLabel : NameLabel(idx);
  });
  this->ubSan = std::make_unique<UBSan>(*(this->fun), this->executionByLabels);
}

UBFreeExec::~UBFreeExec() {
  if (fun != nullptr && fun != owner->GetFun()) {
    delete fun; // The function is generated by us, so we need to delete it
  }
}

int UBFreeExec::Solve(
    int numMap, bool withInterestInit, bool withRandomInit, bool withInterestCoefs, bool debug
) {
  Assert(numMap > 0, "Number of mappings must be greater than 0");

  Log::Get().OpenSection("UBFreeExec: Solve");

  Log::Get().Out() << "Execution Path: ";
  for (size_t i = 0; i < execution.size(); i++) {
    Log::Get().Out() << execution[i];
    if (i != execution.size() - 1) {
      Log::Get().Out() << "->";
    }
  }
  Log::Get().Out() << std::endl;

  // Try to generate a couple of init-final mapping
  for (int i = 0; i < numMap; i++) {
    Log::Get().OpenSection("UBFreeExec: Iteration " + std::to_string(i));
    if (!solve(withInterestInit, withRandomInit, withInterestCoefs, debug)) {
      std::cout << "WARNING: UNSAT at " << i << "-th initialization" << std::endl;
      Log::Get().CloseSection();
      Log::Get().CloseSection();
      return i; // There's no need to report an error to our caller
    }
    Log::Get().CloseSection();
  }

  Log::Get().CloseSection();

  return numMap;
}

bool UBFreeExec::solve(
    bool withInterestInit, bool withRandomInit, bool withInterestCoefs, bool debug
) {
  // clang-format off
  // Our constraint generation follow some rules:
  // 1. When the initializations vector is empty, we need to generate the constraints from scratch.
  // 2. When the initializations vector has only one element, we need to re-generate the constraints with using some symbols that we already resolved. We also need to ensure the new initialization are sufficiently different from the previous one.
  // 3. Otherwise, we only need to ensure that the new initializations are sufficiently different from the previous ones
  // clang-format on

  // Generate or re-generate the constraints for the function execution
  if (inits.size() <= 1) {
    Log::Get().Out() << "Generating or re-generating UB-free constraints" << std::endl;
    ubSan->Reset();

    // Let each parameter (coefficient or constant) interesting initially
    if (withInterestInit) {
      ubSan->MakeInitInteresting();
    }
    if (withRandomInit) {
      ubSan->MakeInitWithRandomValue();
    }
    if (withInterestCoefs) {
      ubSan->EnableInterestCoefs(true);
    }

    if (debug) {
      Log::Get().Out() << "Initial constraints: " << std::endl;
      Log::Get().Out() << "```" << std::endl;
      ubSan->PrintConstraints();
      Log::Get().Out() << "```" << std::endl;
    }

    ubSan->Collect();
  }

  // We are solving the input parameters; let's sample for a different solution
  if (!inits.empty()) {
    // Ensure that the initialisation is sufficiently different from the previous one
    ubSan->MakeInitDifferentFrom(inits.back());
  }

  if (debug) {
    Log::Get().Out() << "Final constraints: " << std::endl;
    Log::Get().Out() << "```" << std::endl;
    ubSan->PrintConstraints();
    Log::Get().Out() << "```" << std::endl;
  }

  // We are solving the symbols for the first time; let's try simplifying the constraints
  if (inits.empty()) {
    ubSan->Optimize();

    if (debug) {
      // Dump the optimized SMT queries for debugging
      Log::Get().Out() << "Optimized constraints: " << std::endl;
      Log::Get().Out() << "```" << std::endl;
      ubSan->PrintConstraints();
      Log::Get().Out() << "```" << std::endl;
    }
  }

  // Solve the generated constraints and see if we succeeded
  z3::solver solver(ubSan->GetContext());
  solver.add(ubSan->GetConstraints());
  if (solver.check() == z3::unknown) {
    Log::Get().Out() << "UNKNOWN" << std::endl;
    return false;
  }
  if (solver.check() == z3::unsat) {
    Log::Get().Out() << "UNSAT" << std::endl;
    return false;
  }

  Log::Get().Out() << "SAT" << std::endl;

  z3::model model = solver.get_model();
  Log::Get().Out() << "Execution model: " << std::endl;
  Log::Get().Out() << "```" << std::endl;
  Log::Get().Out() << model << std::endl;
  Log::Get().Out() << "```" << std::endl;

  // Extract values for the resolved symbols to facilitate subsequent solving
  extractSymbolsFromModel(model);
  // Insert values for unresolved symbols in unexecuted blocks
  // We only do this for the first initialization, as afterward, all symbols should be resolved
  if (inits.empty()) {
    if (GlobalOptions::Get().EnableUBInUnexecutedBbls) {
      insertUBsIntoUnexecutedBbls();
    }
    insertRandomValueIntoUnsolvedSymbols();
  } else {
    for (const auto *s: fun->GetSymbols()) {
      Assert(s->IsSolved(), "The symbol %s is not yet solved", s->GetName().c_str());
    }
  }
  // Extract the initialisation and finalisation values from the model
  inits.push_back(extractParamsFromModel(model, 0));
  finas.push_back(extractParamsFromModel(model, -1));

  return true;
}

void UBFreeExec::extractSymbolsFromModel(z3::model &model) {
  for (auto symbol: fun->GetSymbols()) {
    if (symbol->IsSolved()) {
      continue;
    }
    const auto symName = symbol->GetName().c_str();
    // Currently, we only support coefficients
    const auto symKey = ubSan->CreateCoefExpr(*dynamic_cast<const symir::Coef *>(symbol)).decl();
    if (model.has_interp(symKey)) {
      z3::expr symConst = model.get_const_interp(symKey);
      Assert(symConst.is_numeral(), "Symbol %s is not a numeral", symName);
      int symVal;
      Assert(
          symConst.is_numeral_i(symVal), "Failed to obtain the symVal of symbol %s from the model",
          symName
      );
      symbol->SetValue(std::to_string(symVal));
      Log::Get().Out() << "Extract symbols: sym=" << symName << ", value=" << symVal << std::endl;
    } else {
      // The symbol is never used by any basic blocks being executed
      Log::Get().Out() << "Extract symbols: sym=" << symName << ", value=<unresolved>" << std::endl;
    }
  }
}

std::vector<ArgPlus<int>> UBFreeExec::extractParamsFromModel(z3::model &model, int version) {
  Assert(version == 0 || version == -1, "Version must be 0 (init) or 1 (fina)");
  const auto verTag = version == 0 ? "initialization" : "finalization";
  std::vector<ArgPlus<int>> args;
  for (auto param: fun->GetParams()) {
    const auto paramName = param->GetName().c_str();
    std::vector<z3::expr> paramKeys;
    if (param->IsVector()) {
      args.emplace_back(param->GetVecDims());
      for (int i = 0; i < param->GetVecNumEls(); i++) {
        paramKeys.push_back(ubSan->CreateVecElExpr(param, i, version));
      }
    } else {
      args.emplace_back();
      paramKeys.push_back(ubSan->CreateScaExpr(param, version));
    }
    for (int i = 0; i < static_cast<int>(paramKeys.size()); i++) {
      z3::expr &paramKey = paramKeys[i];
      if (model.has_interp(paramKey.decl())) {
        z3::expr paramConst = model.get_const_interp(paramKey.decl());
        Assert(paramConst.is_numeral(), "Parameter %s is not a numeral", paramName);
        int paramVal;
        Assert(
            paramConst.is_numeral_i(paramVal),
            "Failed to obtain the value of parameter %s from the model", paramName
        );
        if (args.back().IsScalar()) {
          args.back().SetValue(paramVal);
        } else {
          args.back().SetValue(i, paramVal);
        }
        Log::Get().Out() << "Extract " << verTag << ": param=" << paramName
                         << ", value=" << paramVal << std::endl;
      } else {
        // The parameter is never used by any executed block. We assign it a default value.
        // Note, the assigned value should be the same as its value in finalization (as it
        // is never used means it is never evaluated/changed).
        if (args.back().IsVector()) {
          args.back().SetValue(i, GlobalOptions::Get().LowerBound);
        } else {
          args.back().SetValue(GlobalOptions::Get().LowerBound);
        }
        Log::Get().Out() << "Extract " << verTag << ": param=" << paramName << ", value=<unused>"
                         << std::endl;
      }
    }
  }
  return args;
}

void UBFreeExec::insertUBsIntoUnexecutedBbls() {
  std::vector<std::string> blocks(fun->GetBlocks().size());
  std::ranges::transform(fun->GetBlocks(), blocks.begin(), [](const auto *b) {
    return b->GetLabel();
  });
  auto rand = Random::Get().UniformReal();
  IntUBInject ubInj{};
  for (const auto &label: blocks) {
    if (std::ranges::find(executionByLabels, label) == executionByLabels.end()) {
      if (rand() < GlobalOptions::Get().UBInjectionProba) {
        auto newFun = ubInj.InjectUBs(fun, fun->FindBlock(label));
        if (newFun != nullptr) {
          // We have created a new function with UBs injected
          if (fun != owner->GetFun()) {
            delete fun; // The function is created by us
          }
          fun = newFun.release();
        }
      }
    }
  }
}

void UBFreeExec::insertRandomValueIntoUnsolvedSymbols() {
  // For now, we just define all symbols in the function with a random value
  const auto rand = Random::Get().Uniform(
      GlobalOptions::Get().LowerCoefBound, GlobalOptions::Get().UpperCoefBound
  );
  for (const auto &sym: fun->GetSymbols()) {
    if (sym->IsSolved()) {
      continue; // If the symbol is already defined, we don't need to smash it
    }
    const int val = rand();
    sym->SetValue(std::to_string(val));
    Log::Get().Out() << "Define symbols: sym=" << sym->GetName() << ", val=" << val
                     << " (for unexecuted basic blocks)" << std::endl;
  }
}
