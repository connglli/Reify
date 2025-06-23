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

#include "lib/block.hpp"

#include <sstream>

#include "global.hpp"
#include "lib/chksum.hpp"
#include "lib/logger.hpp"
#include "lib/naming.hpp"
#include "lib/random.hpp"
#include "lib/samputils.hpp"

void BlkGen::Generate() {
  Log::Get().OpenSection(std::string("BlkGen: Block #") + std::to_string(bblNo));

  const auto rand = Random::Get().Uniform(0, fun.NumParams() - 1);

  // We define a variable for each statement using other variables
  const auto numAssPerBbl = GlobalOptions::Get().NumAssignsPerBBL;
  assignments = SampleKDistinct(fun.NumParams(), numAssPerBbl);
  for (int stmtIndex = 0; stmtIndex < numAssPerBbl; stmtIndex++) {
    int varIndex = assignments[stmtIndex];
    Log::Get().Out() << varIndex << " <- ";

    // Sample the variables which will be used in the RHS of the assignment statement
    const auto numVarsPerAss = GlobalOptions::Get().NumVarsPerAssign;
    defUsedVars[varIndex] = SampleKDistinct(fun.NumParams(), numVarsPerAss);

    // Check every element present in vector is less than fun.NumParams()
    for (int i = 0; i < numVarsPerAss; i++) {
      const int var = defUsedVars[varIndex][i];
      Log::Get().Out() << var;
      if (i != numVarsPerAss - 1) {
        Log::Get().Out() << ", ";
      }
      Assert(
          var < fun.NumParams(), "Variable index (=%d) out of bounds (=%d)", var, fun.NumParams()
      );
    }
    Log::Get().Out() << std::endl;

    // Assign a coefficient for each variable that contributes to the current one
    for (int i = 0; i < numVarsPerAss; i++) {
      // Statement index is the index of the assignment statement in a
      // basic block, and i is the index of the variable in the RHS of
      // the assignment statement
      fun.DefSymbol(NameCoef(bblNo, stmtIndex, i));
    }

    // We also assign a constant to the current variable being defined
    fun.DefSymbol(NameConst(bblNo, stmtIndex));
  }

  // Our conditional is controlled by all variables defined in this basic block.
  // In case we need more variables beyond the number of variables we already defined,
  // let's refer to some variables defined in other basic blocks.
  condUsedVars = assignments;
  for (int i = 0; i < GlobalOptions::Get().NumVarsInCond - numAssPerBbl; i++) {
    int randomVariable = rand();
    while (std::ranges::find(condUsedVars, randomVariable) != condUsedVars.end()) {
      randomVariable = rand();
    }
    condUsedVars.push_back(randomVariable);
  }

  // Define a specific target that our conditional controls
  if (bblSkt.GetSuccessors().size() > 1) {
    // Assign a coefficient to each variable contributing to the conditional
    for (int i = 0; i < GlobalOptions::Get().NumVarsInCond; i++) {
      // 0 is the statement index here (even I'm not sure why i chose
      // this as a parameter but i don't want to break the code so
      // let's just not touch this), and i is the index of the
      // variable in the actual conditional.
      // (... perhaps because it is the first conditional statement)
      fun.DefSymbol(NameCondCoef(bblNo, 0, i));
    }
    // Target block is a parameter here if we ever wanted to support more that
    // 2 potential targets for the same basic block. We will always branch to
    // the very first successor in the conditional.
    fun.DefSymbol(NameCondConst(bblNo, 0));
  }

  Log::Get().CloseSection();
}

std::string BlkGen::GenerateCode(const UBFreeExec &exec) const {
  std::ostringstream code;

  // Generate the label for the basic block
  if (bblSkt.GetType() == BblSketch::BLOCK_LOOP_HEAD) {
    code << "    do {" << std::endl;
  }
  code << NameLabel(bblNo) << ":" << std::endl;

  // Check if we need a pass counter
  if (exec.GetPassCounterBbl() == bblNo) {
    code << NamePassCounter() << "++;" << std::endl;
  }

  // Generate the statements one by one
  auto rand =
      Random::Get().Uniform(GlobalOptions::Get().LowerBound, GlobalOptions::Get().UpperBound);
  for (int stmtIndex = 0; stmtIndex < static_cast<int>(assignments.size()); stmtIndex++) {
    int varIndex = assignments[stmtIndex];
    const auto &dependencies = GetDeps(varIndex);

    code << "    " << NameVar(varIndex) << " = ";
    for (int i = 0; i < static_cast<int>(dependencies.size()); ++i) {
      // Look up the coefficient in the parameters map
      std::string coefSym = NameCoef(bblNo, stmtIndex, i);
      Assert(
          exec.SymDefined(coefSym), "The symbol for coefficient %s is not yet resolved",
          coefSym.c_str()
      );
      // Multiply the dependency by the coefficient
      code << exec.GetSymVal(coefSym) << " * " << NameVar(dependencies[i]);
      if (i < dependencies.size() - 1) {
        code << " + ";
      }
    }

    std::string constSym = NameConst(bblNo, stmtIndex);
    Assert(
        exec.SymDefined(constSym), "The symbol for constant %s is not yet resolved",
        constSym.c_str()
    );
    code << " + " << exec.GetSymVal(constSym) << ";" << std::endl;
  }

  // Handle pass counters
  if (exec.GetPassCounterBbl() == bblNo) {
    code << "    if(" << NamePassCounter() << " >= " << exec.GetPassCounter() << ")" << std::endl;
    code << "    {" << std::endl;
    code << "        goto " << NameLabel(fun.NumBbls() - 1) << ";" << std::endl;
    code << "    }" << std::endl;
  }

  // Handle conditional and unconditional jumps
  if (bblSkt.GetSuccessors().size() > 1) {
    code << generateCondCode(exec, bblSkt.GetSuccessors()[0]);
    code << generateUncondCode(bblSkt.GetSuccessors()[1]);
  } else if (bblSkt.GetSuccessors().size() == 1) {
    code << generateUncondCode(bblSkt.GetSuccessors()[0]);
  } else {
    code << "    return " << StatelessChecksum::GetComputeName() << "(" << fun.NumParams()
         << ", (int[" << fun.NumParams() << "]){ ";
    for (int i = 0; i < fun.NumParams(); ++i) {
      code << NameVar(i);
      if (i < fun.NumParams() - 1) {
        code << ", ";
      }
    }
    code << " });" << std::endl;
  }
  return code.str();
}

std::string BlkGen::generateCondCode(const UBFreeExec &exec, const int successor) const {
  std::ostringstream constraint;
  auto rand =
      Random::Get().Uniform(GlobalOptions::Get().LowerBound, GlobalOptions::Get().UpperBound);

  if (bblSkt.GetType() == BblSketch::BLOCK_LOOP_LATCH) {
    constraint << "    } while (";
  } else {
    constraint << "    if (";
  }

  int index = 0;
  for (auto i: condUsedVars) {
    std::string coefSym = NameCondCoef(bblNo, 0, index);
    Assert(
        exec.SymDefined(coefSym), "The symbol for coefficient %s is not yet resolved",
        coefSym.c_str()
    );
    constraint << exec.GetSymVal(coefSym) << " * " << NameVar(i);
    constraint << " + ";
    ++index;
  }

  std::string constSym = NameCondConst(bblNo, 0);
  Assert(
      exec.SymDefined(constSym), "The symbol for constant %s is not yet resolved", constSym.c_str()
  );
  constraint << exec.GetSymVal(constSym);
  if (bblSkt.GetType() == BblSketch::BLOCK_LOOP_LATCH) {
    constraint << " >= 0);" << std::endl; // We latch to the loop header
  } else {
    constraint << " >= 0) goto " << NameLabel(successor) << ";" << std::endl;
  }

  // Output the constraint
  return constraint.str();
}

std::string BlkGen::generateUncondCode(const int successor) const {
  std::ostringstream code;
  code << "    goto " << NameLabel(successor) << ";" << std::endl;
  return code.str();
}
