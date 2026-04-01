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

#include "lib/symexec.hpp"

#include <cstring>
#include <optional>
#include <ranges>
#include "global.hpp"
#include "lib/logger.hpp"
#include "lib/random.hpp"
#include "lib/samputils.hpp"
#include "lib/strutils.hpp"
#include "lib/ubinject.hpp"
#include "lib/varstate.hpp"

#include <chrono>

const int SymExec::PassCounterBblId =
    2147483647; // A large number to avoid conflicts with real basic block IDs
const std::string SymExec::PassCounterBblLabel = NameLabel(SymExec::PassCounterBblId);

SymExec::SymExec(const FunPlus &fun, const std::vector<int> &execution) {
  this->owner = &fun;
  // Detect unterminated execution and fix it to terminate
  const int exitBbl = fun.GetExitBblId();
  const int stopBbl = execution.back();
  if (stopBbl == exitBbl) {
    this->fun = fun.GetFun();
    this->execution = execution;
  } else {
    Log::Get().OpenSection("SymExec: Fixing Unterminated Execution");
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
    auto passCounter = funBd->SymScaLocal("passCounterLocal", funBd->SymCoef("zero", "0"));
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

SymExec::~SymExec() {
  if (fun != nullptr && fun != owner->GetFun()) {
    delete fun; // The function is generated by us, so we need to delete it
  }
}

int SymExec::Solve(
    int numMap, bool withInterestInit, bool withRandomInit, bool withInterestCoefs, bool debug
) {
  Assert(numMap > 0, "Number of mappings must be greater than 0");

  Log::Get().OpenSection("SymExec: Solve");

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
    Log::Get().OpenSection("SymExec: Iteration " + std::to_string(i));
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

bool SymExec::solve(
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
    if (debug) {
      // Dump the optimized SMT queries for debugging
      Log::Get().Out() << "Optimized constraints: " << std::endl;
      Log::Get().Out() << "```" << std::endl;
      ubSan->PrintConstraints();
      Log::Get().Out() << "```" << std::endl;
    }
  }

  // Solve the generated constraints and see if we succeeded
  // Reuse one solver instance across initialization attempts to avoid
  // re-asserting the same base constraints.
  if (!solver || inits.size() <= 1) {
    bitwuzla::Options opts;
    opts.set(bitwuzla::Option::PRODUCE_MODELS, true);
    opts.set(bitwuzla::Option::NTHREADS, GlobalOptions::Get().BitwuzlaNumThreads);
    solver = std::make_unique<bitwuzla::Bitwuzla>(ubSan->GetTermManager(), opts);
    numAssertedConstraints = 0;
  }

  const auto &constraints = ubSan->GetConstraints();

  if (debug) {
    const auto stats = ubSan->GetStats();
    Log::Get().Out() << "Constraint stats: asserted=" << stats.assertedConstraints
                     << " addCalls=" << stats.addCalls << " skippedTrue=" << stats.skippedTrue
                     << " deduped=" << stats.deduped
                     << " ensureInRangeCalls=" << stats.ensureInRangeCalls
                     << " boundedAlready=" << stats.boundedAlready
                     << " uniqueConstraintIds=" << stats.uniqueConstraintIds
                     << " uniqueBoundedIds=" << stats.uniqueBoundedIds
                     << " (solverAsserted=" << numAssertedConstraints << ")" << std::endl;
  }

  for (; numAssertedConstraints < constraints.size(); ++numAssertedConstraints) {
    solver->assert_formula(constraints[numAssertedConstraints]);
  }

  const auto t0 = std::chrono::steady_clock::now();
  auto result = solver->check_sat();
  const auto t1 = std::chrono::steady_clock::now();
  if (debug) {
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    Log::Get().Out() << "Bitwuzla check_sat time: " << ms << "ms" << std::endl;

    // One-line dump of Bitwuzla statistics.
    const auto s = solver->statistics();
    Log::Get().Out() << "Bitwuzla statistics:";
    for (const auto &kv: s) {
      Log::Get().Out() << " " << kv.first << "=" << kv.second;
    }
    Log::Get().Out() << std::endl;
  }
  if (result == bitwuzla::Result::UNKNOWN) {
    Log::Get().Out() << "UNKNOWN" << std::endl;
    return false;
  }
  if (result == bitwuzla::Result::UNSAT) {
    Log::Get().Out() << "UNSAT" << std::endl;
    return false;
  }

  Log::Get().Out() << "SAT" << std::endl;

  // Extract values for the resolved symbols to facilitate subsequent solving
  extractSymbolsFromModel();

  // Extract the state of each variable along the exec path from the model
  this->varStateExtractor.push_back(VariableStateExtractor());
  varStateExtractor.back().extract(this);

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
  auto randDefaultArgs =
      Random::Get().Uniform(GlobalOptions::Get().LowerBound, GlobalOptions::Get().UpperBound);
  std::vector<int> defaultArgs;
  for (size_t i = 0; i < fun->GetParams().size(); i++) {
    defaultArgs.push_back(randDefaultArgs());
  }
  inits.push_back(extractParamsFromModel(0, defaultArgs));
  finas.push_back(extractParamsFromModel(-1, defaultArgs));

  return true;
}

std::optional<int32_t> SymExec::extractTermFromModel(bitwuzla::Term t) {
  bitwuzla::Term symValue = solver->get_value(t);
  std::string binaryStr = symValue.value<std::string>(2);
  if (binaryStr.empty()) return {};
  // Convert binary string to signed integer (32-bit)
  int32_t symVal = 0;
  // Parse as unsigned first, then reinterpret as signed
  uint64_t unsigned_val = std::stoull(binaryStr, nullptr, 2);
  uint32_t u32 = static_cast<uint32_t>(unsigned_val);
  std::memcpy(&symVal, &u32, sizeof(int32_t));
  return symVal;
}

void SymExec::extractSymbolsFromModel() {
  for (auto symbol: fun->GetSymbols()) {
    if (symbol->IsSolved()) {
      continue;
    }

    const auto symName = symbol->GetName();
    if (!ubSan->IsCoefManaged(*dynamic_cast<symir::Coef *>(symbol))) {
      Log::Get().Out() << "Extract symbols: sym=" << symName << ", value=<unresolved>" << std::endl;
      continue;
    }

    // Currently, we only support coefficients
    const auto symKey = ubSan->CreateCoefExpr(*dynamic_cast<const symir::Coef *>(symbol));
    std::optional<int32_t> symValOpt = extractTermFromModel(symKey);
    Assert(symValOpt.has_value(), "The symbol value of symbol %s is empty", symName.c_str());
    int32_t symVal = symValOpt.value();
    
    symbol->SetValue(std::to_string(symVal));
    Log::Get().Out() << "Extract symbols: sym=" << symName << ", value=" << symVal << std::endl;
  }
}

std::vector<ArgPlus<int>>
SymExec::extractParamsFromModel(int version, const std::vector<int> &defaults) {
  Assert(version == 0 || version == -1, "Version must be 0 (init) or -1 (fina)");
  const auto verTag = version == 0 ? "initialization" : "finalization";
  std::vector<ArgPlus<int>> args;

  std::function<
      void(const symir::VarDef *, symir::SymIR::Type, const std::string &, const std::vector<int> &, size_t, std::string, int, bool, ArgPlus<int> &, std::vector<bitwuzla::Term> &)>
      expand = [&](const symir::VarDef *var, symir::SymIR::Type type, const std::string &sName,
                   const std::vector<int> &shape, size_t shapeIdx, std::string termSuffix,
                   int flatIdx, bool hasArray, ArgPlus<int> &arg,
                   std::vector<bitwuzla::Term> &keys) {
        if (shapeIdx < shape.size()) {
          const int dimLen = shape[shapeIdx];
          for (int i = 0; i < dimLen; ++i) {
            const int nextFlat = flatIdx * dimLen + i;
            expand(var, type, sName, shape, shapeIdx + 1, termSuffix, nextFlat, true, arg[i], keys);
          }
          return;
        }

        if (hasArray) {
          termSuffix += "_el" + std::to_string(flatIdx);
        }

        if (type == symir::SymIR::Type::STRUCT) {
          const auto *sDef = fun->GetStruct(sName);
          for (int i = 0; i < (int) sDef->GetFields().size(); ++i) {
            const auto &field = sDef->GetField(i);
            std::string nextTerm = termSuffix + "_" + field.name;
            expand(
                var, field.baseType, field.structName, field.shape, 0, nextTerm, 0, false, arg[i],
                keys
            );
          }
        } else {
          // Leaf
          keys.push_back(ubSan->CreateVersionedExpr(var, termSuffix, version));
        }
      };

  std::function<
      ArgPlus<int>(symir::SymIR::Type, const std::string &, const std::vector<int> &, size_t)>
      createSchema = [&](symir::SymIR::Type type, const std::string &sName,
                         const std::vector<int> &shape, size_t shapeIdx) -> ArgPlus<int> {
    if (shapeIdx < shape.size()) {
      auto res = ArgPlus<int>(ArrayDim, shape[shapeIdx]);
      for (int i = 0; i < shape[shapeIdx]; ++i) {
        res[i] = createSchema(type, sName, shape, shapeIdx + 1);
      }
      return res;
    } else if (type == symir::SymIR::Type::STRUCT) {
      const auto *sDef = fun->GetStruct(sName);
      std::vector<std::string> fNames;
      for (const auto &f: sDef->GetFields())
        fNames.push_back(f.name);
      auto res = ArgPlus<int>(sName, fNames);
      for (int i = 0; i < (int) sDef->GetFields().size(); ++i) {
        const auto &f = sDef->GetField(i);
        res[i] = createSchema(f.baseType, f.structName, f.shape, 0);
      }
      return res;
    } else {
      return ArgPlus<int>(0);
    }
  };

  int index = -1;
  for (auto param: fun->GetParams()) {
    index += 1;
    const auto paramName = param->GetName().c_str();
    args.push_back(createSchema(
        param->GetBaseType(),
        (param->GetBaseType() == symir::SymIR::Type::STRUCT ? param->GetStructName() : ""),
        param->GetVecShape(), 0
    ));

    std::vector<bitwuzla::Term> paramKeys;
    expand(
        param, param->GetBaseType(),
        (param->GetBaseType() == symir::SymIR::Type::STRUCT ? param->GetStructName() : ""),
        param->GetVecShape(), 0, "", 0, false, args.back(), paramKeys
    );

    for (int i = 0; i < static_cast<int>(paramKeys.size()); i++) {
      bitwuzla::Term &paramKey = paramKeys[i];
      bitwuzla::Term paramValue = solver->get_value(paramKey);
      std::string binaryStr = paramValue.value<std::string>(2);
      // Convert binary string to signed integer (32-bit)
      int32_t paramVal = defaults[index];
      if (!binaryStr.empty()) {
        // Parse as unsigned first, then reinterpret as signed
        uint64_t unsigned_val = std::stoull(binaryStr, nullptr, 2);
        uint32_t u32 = static_cast<uint32_t>(unsigned_val);
        std::memcpy(&paramVal, &u32, sizeof(int32_t));
      }
      args.back().SetValue(i, paramVal);
      Log::Get().Out() << "Extract " << verTag << ": param=" << paramName << ", value=" << paramVal
                       << std::endl;
    }
  }
  return args;
}

void SymExec::insertUBsIntoUnexecutedBbls() {
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

void SymExec::insertRandomValueIntoUnsolvedSymbols() {
  // For now, we just define all symbols in the function with a random value
  const auto rand =
      Random::Get().Uniform(GlobalOptions::Get().LowerBound, GlobalOptions::Get().UpperBound);
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

std::string SymExec::getVarStateJson() {
  return varstate::allToJsonFile(this->varStateExtractor);
}
