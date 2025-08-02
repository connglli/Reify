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

#include "lib/function.hpp"

#include "global.hpp"
#include "lib/chksum.hpp"
#include "lib/logger.hpp"
#include "lib/lowers.hpp"
#include "lib/naming.hpp"
#include "lib/random.hpp"
#include "lib/samputils.hpp"
#include "lib/strutils.hpp"

void FunPlus::Generate() {
  Log::Get().OpenSection("FunPlus: Generate Function " + name);

  // Generate the sketch of our control flow graph
  cfg.Generate();
  for (int i = 0; i < Random::Get().Uniform(0, maxNumLoops)(); i++) {
    cfg.GenerateReduLoop(maxNumBblsPerLoop);
  }
  cfg.Print();

  // Create the function builder to build the function
  auto builder = std::make_unique<symir::FunctBuilder>(name, symir::SymIR::I32);
  for (int i = 0; i < numParams; i++) {
    builder->SymParam(NameVar(i), symir::SymIR::Type::I32);
  }

  // Map each basic block in the CFG to a basic block generator
  for (int i = 0; i < NumBbls(); i++) {
    generateBasicBlock(builder.get(), i, cfg.GetBbl(i));
  }

  // Now we build the function
  fun = builder->Build();

  Log::Get().CloseSection();
}

std::vector<int> FunPlus::SampleExec(int execStep, bool consistent) {
  return cfg.SampleExec(execStep, consistent);
}

void FunPlus::generateBasicBlock(symir::FunctBuilder *funBd, int bblId, const BblSketch &bblSkt) {
  Log::Get().OpenSection(std::string("FunPlus: Generate Block #") + std::to_string(bblId));

  auto bblBd = funBd->OpenBlock(NameLabel(bblId));
  const auto randTermOp = Random::Get().Uniform(
      GlobalOptions::Get().EnableAllOps ? 1 : symir::Term::Op::OP_MUL,
      GlobalOptions::Get().EnableAllOps ? symir::Term::Op::NUM_OPS - 1 : symir::Term::Op::OP_MUL
  );
  const auto randExprOp = Random::Get().Uniform(
      GlobalOptions::Get().EnableAllOps ? 0 : symir::Expr::Op::OP_ADD,
      GlobalOptions::Get().EnableAllOps ? symir::Expr::Op::NUM_OPS - 1 : symir::Expr::Op::OP_ADD
  );

  // We define a variable as LHS for each assignment using other variables
  const auto numAssPerBbl = GlobalOptions::Get().NumAssignsPerBBL;
  const auto assignments = SampleKDistinct(numParams, numAssPerBbl);

  for (int stmtIndex = 0; stmtIndex < numAssPerBbl; stmtIndex++) {
    int varIndex = assignments[stmtIndex];
    const auto var = funBd->FindVar(NameVar(varIndex));
    Assert(var != nullptr, "Variable %d for definition is not defined!", varIndex);
    Log::Get().Out() << "Generate assignment: " << varIndex << " <- ";

    // Sample the variables which will be used in the RHS of the assignment statement
    const auto numVarsPerAss = GlobalOptions::Get().NumVarsPerAssign;
    const auto assDeps = SampleKDistinct(numParams, numVarsPerAss);
    Log::Get().Out() << JoinInt(assDeps, ", ") << std::endl;

    // Create the terms for each assigment
    std::vector<symir::SymIRBuilder::TermID> terms;
    // Assign a coefficient for each variable that contributes to the current one
    for (int i = 0; i < numVarsPerAss; i++) {
      int depIndex = assDeps[i];
      const auto depVar = funBd->FindVar(NameVar(depIndex));
      Assert(depVar != nullptr, "Variable %d for dependency#%d is not defined!", depIndex, i);
      // Statement index is the index of the assignment statement in a
      // basic block, and i is the index of the variable in the RHS of
      // the assignment statement. We create a term with random operations
      const auto depCoef = funBd->SymCoef(NameCoef(bblId, stmtIndex, i), symir::SymIR::Type::I32);
      terms.push_back(bblBd->SymTerm(static_cast<symir::Term::Op>(randTermOp()), depCoef, depVar));
    }
    // Assign a constant to the current variable being defined
    terms.push_back(bblBd->SymCstTerm(
        funBd->SymCoef(NameConst(bblId, stmtIndex), symir::SymIR::Type::I32), nullptr
    ));

    // Create the assignment statement with a random expression
    bblBd->SymAssign(var, bblBd->SymExpr(static_cast<symir::Expr::Op>(randExprOp()), terms));
  }

  // Define a specific target that our conditional/unconditional controls
  if (bblSkt.GetSuccessors().size() > 1) {
    // Our conditional is controlled by all variables defined in this basic block.
    // In case we need more variables beyond the number of variables we already defined,
    // let's refer to some variables defined in other basic blocks.
    const auto randVar = Random::Get().Uniform(0, numParams - 1);
    auto condDeps = assignments;
    const auto numVarsInCond = GlobalOptions::Get().NumVarsInCond;
    for (int i = 0; i < numVarsInCond - numAssPerBbl; i++) {
      int dep = randVar();
      while (std::ranges::find(condDeps, dep) != condDeps.end()) {
        dep = randVar();
      }
      condDeps.push_back(dep);
    }
    Log::Get().Out() << "Generate conditional: " << JoinInt(condDeps, ", ") << std::endl;

    std::vector<symir::SymIRBuilder::TermID> condTerms;
    // Assign a coefficient to each variable contributing to the conditional
    for (int i = 0; i < numVarsInCond; i++) {
      int depIndex = condDeps[i];
      const auto depVar = funBd->FindVar(NameVar(depIndex));
      Assert(depVar != nullptr, "Variable %d for dependency#%d is not defined!", depIndex, i);
      // 0 is the statement index here (even I'm not sure why i chose
      // this as a parameter but i don't want to break the code so
      // let's just not touch this), and i is the index of the
      // variable in the actual conditional.
      // (... perhaps because it is the first conditional statement)
      const auto depCoef = funBd->SymCoef(NameCondCoef(bblId, 0, i), symir::SymIR::Type::I32);
      condTerms.push_back(
          bblBd->SymTerm(static_cast<symir::Term::Op>(randTermOp()), depCoef, depVar)
      );
    }
    condTerms.push_back(
        bblBd->SymCstTerm(funBd->SymCoef(NameCondConst(bblId, 0), symir::SymIR::Type::I32), nullptr)
    );

    // Target block is a parameter here if we ever wanted to support more that
    // 2 potential targets for the same basic block. We will always branch to
    // the very first successor in the condition of the conditional evals to true.
    const auto randCondOp = Random::Get().Uniform(
        GlobalOptions::Get().EnableAllOps ? 0 : symir::Cond::Op::OP_GTZ,
        GlobalOptions::Get().EnableAllOps ? symir::Cond::Op::NUM_OPS - 1 : symir::Cond::Op::OP_GTZ
    );
    bblBd->SymBranch(
        NameLabel(bblSkt.GetSuccessors()[0]), NameLabel(bblSkt.GetSuccessors()[1]),
        bblBd->SymCond(
            static_cast<symir::Cond::Op>(randCondOp()),
            bblBd->SymExpr(static_cast<symir::Expr::Op>(randExprOp()), condTerms)
        )
    );
  } else if (bblSkt.GetSuccessors().size() == 1) {
    // Directly jump to the target successor
    bblBd->SymGoto(NameLabel(bblSkt.GetSuccessors()[0]));
  } else {
    // Return the function if there is no successor
    bblBd->SymReturn();
  }

  // Now we build our basic block
  funBd->CloseBlock(bblBd);

  Log::Get().CloseSection();
}

std::string FunPlus::GenerateFunCode(const UBFreeExec &exec) {
  const auto *fun = exec.GetFun();
  Assert(fun != nullptr, "Function is not generated yet!");
  std::ostringstream oss;
  symir::SymCxLower lower(oss);
  lower.Lower(*exec.GetFun());
  return oss.str();
}

std::string FunPlus::GenerateFunSexpCode(const UBFreeExec &exec) {
  const auto *fun = exec.GetFun();
  Assert(fun != nullptr, "Function is not generated yet!");
  std::ostringstream oss;
  symir::SymSexpLower lower(oss);
  lower.Lower(*exec.GetFun());
  return oss.str();
}

std::unique_ptr<jnif::ClassFile> FunPlus::GenerateFunJavaCode(
    const UBFreeExec &exec, const std::string &className, bool main, bool debug
) {
  const auto *fun = exec.GetFun();
  Assert(fun != nullptr, "Function is not generated yet!");

  Log::Get().OpenSection("Function to Class");

  symir::SymJavaBytecodeLower lower(className);
  lower.Lower(*fun);

  Log::Get().OpenSection("Bytecode");
  Log::Get().Out() << lower.GetJavaMethod()->instList().toString(/*offset=*/true);
  Log::Get().CloseSection();

  Log::Get().CloseSection();

  jnif::ClassFile *clazz = lower.GetJavaClass();
  if (!main) {
    return lower.TakeJavaClass();
  }

  Log::Get().OpenSection("Main Method");

  const auto &initializations = exec.GetInitializations();
  const auto &finalizations = exec.GetFinalizations();
  Assert(
      initializations.size() == finalizations.size(),
      "Initializations and finalizations must have the same size"
  );

  const auto &mainMethod = lower.GetJavaMainMethod();
  const auto &methodRef = lower.GetJavaMethodIndex();
  const auto checkMethodRef = clazz->addMethodRef(
      clazz->getIndexOfClass(JavaStatelessChecksum::GetClassName().c_str()),
      JavaStatelessChecksum::GetCheckChksumName().c_str(),
      JavaStatelessChecksum::GetCheckChksumDesc().c_str()
  );

  for (auto i = 0; i < initializations.size(); i++) {
    const auto &init = initializations[i];
    const auto &fina = finalizations[i];
    // TODO: The checksum computation should be using Java as Java and C might be different
    const auto checksum = StatelessChecksum::Compute(fina);
    mainMethod->instList().addLdc(jnif::Opcode::ldc, clazz->addInteger(checksum));
    for (const int &a: init) {
      mainMethod->instList().addLdc(jnif::Opcode::ldc, clazz->addInteger(a));
    }
    mainMethod->instList().addInvoke(jnif::Opcode::invokestatic, methodRef);
    if (debug) {
      mainMethod->instList().addZero(jnif::Opcode::iconst_1);
    } else {
      mainMethod->instList().addZero(jnif::Opcode::iconst_0);
    }
    mainMethod->instList().addInvoke(jnif::Opcode::invokestatic, checkMethodRef);
  }

  mainMethod->instList().addZero(jnif::Opcode::RETURN);

  clazz->computeFrames(nullptr);

  Log::Get().OpenSection("Bytecode");
  Log::Get().Out() << mainMethod->instList().toString(/*offset=*/true);
  Log::Get().CloseSection();

  Log::Get().CloseSection();

  return lower.TakeJavaClass();
}

std::string FunPlus::GenerateMainCode(const UBFreeExec &exec, bool debug) {
  const auto *fun = exec.GetFun();
  Assert(fun != nullptr, "Function is not generated yet!");

  const auto &initializations = exec.GetInitializations();
  const auto &finalizations = exec.GetFinalizations();
  Assert(
      initializations.size() == finalizations.size(),
      "Initializations and finalizations must have the same size"
  );
  std::ostringstream main;
  main << StatelessChecksum::GetCheckChksumCode(debug) << std::endl;
  main << "int main(int argc, int* argv[])" << std::endl;
  main << "{" << std::endl;
  for (auto i = 0; i < initializations.size(); i++) {
    const auto &init = initializations[i];
    const auto &fina = finalizations[i];
    const auto numParams = static_cast<int>(init.size());
    std::ostringstream chk_oss;
    main << "    " << StatelessChecksum::GetCheckChksumName() << "("
         << StatelessChecksum::Compute(fina) << ", " << fun->GetName() << "(";
    for (auto j = 0; j < numParams; j++) {
      main << init[j];
      if (j < numParams - 1) {
        main << ", ";
      }
    }
    main << "));" << std::endl;
  }
  main << "  return 0;" << std::endl;
  main << "}" << std::endl;
  return main.str();
}

std::string FunPlus::GenerateMappingCode(const UBFreeExec &exec) {
  std::ostringstream mapping;
  for (int i = 0; i < exec.GetInitializations().size(); i++) {
    for (auto x: exec.GetInitializations()[i]) {
      mapping << x << ",";
    }
    mapping << " : ";
    for (auto x: exec.GetFinalizations()[i]) {
      mapping << x << ",";
    }
    mapping << std::endl;
  }
  return mapping.str();
}
