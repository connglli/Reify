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
#include "lib/parsers.hpp"
#include "lib/random.hpp"
#include "lib/samputils.hpp"
#include "lib/strutils.hpp"

void FunPlus::Generate(bool allowDeadCode) {
  Log::Get().OpenSection("FunPlus: Generate Function " + name);

  auto randProba = Random::Get().UniformReal();

  // Generate the sketch of our control flow graph
  cfg.Generate(allowDeadCode);
  for (int i = 0; i < Random::Get().Uniform(0, maxNumLoops)(); i++) {
    cfg.GenerateReduLoop(maxNumBblsPerLoop, allowDeadCode);
  }
  cfg.Print();

  // Create the function builder to build the function
  auto builder = std::make_unique<symir::FunctBuilder>(name, symir::SymIR::I32);
  // The first numParams variables are parameters
  for (int i = 0; i < numParams; i++) {
    if (GlobalOptions::Get().EnableVolatileVars &&
        randProba() < GlobalOptions::Get().VolatileVariableProba) {
      builder->SymParam(NameVar(i), symir::SymIR::Type::I32, /*IsVolatile=*/true);
    } else {
      builder->SymParam(NameVar(i), symir::SymIR::Type::I32, /*IsVolatile=*/false);
    }
  }
  // The next numLocals variables are locals
  for (int i = numParams; i < numParams + numLocals; i++) {
    if (GlobalOptions::Get().EnableVolatileVars &&
        randProba() < GlobalOptions::Get().VolatileVariableProba) {
      builder->SymLocal(
          NameVar(i), builder->SymCoef("init_var" + std::to_string(i), symir::SymIR::Type::I32),
          symir::SymIR::Type::I32, /*IsVolatile=*/true
      );
    } else {
      builder->SymLocal(
          NameVar(i), builder->SymCoef("init_var" + std::to_string(i), symir::SymIR::Type::I32),
          symir::SymIR::Type::I32, /*IsVolatile=*/false
      );
    }
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
  const auto assignments = SampleKDistinct(NumVars(), numAssPerBbl);

  for (int stmtIndex = 0; stmtIndex < numAssPerBbl; stmtIndex++) {
    int varIndex = assignments[stmtIndex];
    const auto var = funBd->FindVar(NameVar(varIndex));
    Assert(var != nullptr, "Variable %d for definition is not defined!", varIndex);
    Log::Get().Out() << "Generate assignment: " << varIndex << " <- ";

    // Sample the variables which will be used in the RHS of the assignment statement
    const auto numVarsPerAss = GlobalOptions::Get().NumVarsPerAssign;
    const auto assDeps = SampleKDistinct(NumVars(), numVarsPerAss);
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
    const auto randVar = Random::Get().Uniform(0, NumVars() - 1);
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

/// A SymIR to Cxx lower with printing reducible loops, etc.
class SymCxLowerWithLoops : public symir::SymCxLower {
public:
  explicit SymCxLowerWithLoops(std::ostream &os, const FunPlus &fun) :
      symir::SymCxLower(os), fun(fun) {}

  void Visit(const symir::Branch &b) override {
    if (curBbl != UBFreeExec::PassCounterBblLabel &&
        fun.GetCfg().GetBbl(curBblId).GetType() == BblSketch::Type::BLOCK_LOOP_LATCH) {
      decIndent();
      out << getIndent() << "} while (";
      b.GetCond()->Accept(*this);
      out << ");" << std::endl;
      out << getIndent() << "goto " << b.GetFalseTarget() << ";" << std::endl;
    } else {
      SymCxLower::Visit(b);
    }
  }

  void Visit(const symir::Block &b) override {
    curBbl = b.GetLabel();
    if (curBbl != UBFreeExec::PassCounterBblLabel) {
      curBblId++;
      if (fun.GetCfg().GetBbl(curBblId).GetType() == BblSketch::Type::BLOCK_LOOP_HEAD) {
        out << getIndent() << "do {" << std::endl;
        incIndent();
      }
    }
    SymCxLower::Visit(b);
    curBbl = "";
  }

private:
  const FunPlus &fun;
  std::string curBbl = "";
  int curBblId = -1;
};

std::string FunPlus::GenerateFunCode(const UBFreeExec &exec) const {
  Assert(exec.GetOwner() == this, "The execution does not belong to this function!");
  const auto *fun = exec.GetFun();
  Assert(fun != nullptr, "Function is not generated yet!");
  std::ostringstream oss;
  SymCxLowerWithLoops lower(oss, *this);
  lower.Lower(*exec.GetFun());
  return oss.str();
}

std::string FunPlus::GenerateFunSexpCode(const UBFreeExec &exec) const {
  Assert(exec.GetOwner() == this, "The execution does not belong to this function!");
  const auto *fun = exec.GetFun();
  Assert(fun != nullptr, "Function is not generated yet!");
  std::ostringstream oss;
  symir::SymSexpLower lower(oss);
  lower.Lower(*exec.GetFun());
  return oss.str();
}

std::unique_ptr<jnif::ClassFile> FunPlus::GenerateFunJavaCode(
    const UBFreeExec &exec, const std::string &className, bool main, bool debug
) const {
  Assert(exec.GetOwner() == this, "The execution does not belong to this function!");

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

  for (size_t i = 0; i < initializations.size(); i++) {
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

std::string FunPlus::GenerateMainCode(const UBFreeExec &exec, bool debug) const {
  Assert(exec.GetOwner() == this, "The execution does not belong to this function!");

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
  main << "int main(int argc, char* argv[])" << std::endl;
  main << "{" << std::endl;
  for (size_t i = 0; i < initializations.size(); i++) {
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

std::string FunPlus::GenerateMappingCode(const UBFreeExec &exec) const {
  Assert(exec.GetOwner() == this, "The execution does not belong to this function!");

  std::ostringstream mapping;
  for (size_t i = 0; i < exec.GetInitializations().size(); i++) {
    for (auto x: exec.GetInitializations()[i]) {
      mapping << x << ",";
    }
    mapping << " : ";
    for (auto x: exec.GetFinalizations()[i]) {
      mapping << x << ",";
    }
    mapping << " : " << StatelessChecksum::Compute(exec.GetFinalizations()[i]);
    mapping << std::endl;
  }
  return mapping.str();
}

std::unique_ptr<symir::Funct> FunPlus::ParseFunSexpCode(const std::string &sexpPath) {
  std::ifstream sifs(sexpPath);
  std::ostringstream oss;
  std::string line;
  while (std::getline(sifs, line)) {
    oss << line << std::endl;
  }
  sifs.close();
  auto parser = symir::SymSexpParser(oss.str());
  parser.Parse();
  return parser.TakeFunct();
}

FunPlus::InitFinaMap FunPlus::ParseMappingCode(const std::string &mapPath) {
  std::ostringstream moss;
  std::ifstream mifs(mapPath);
  Assert(mifs.is_open(), "Error: failed to open file: %s", mapPath.c_str());

  std::vector<std::vector<int>> initialisations;
  std::vector<std::vector<int>> finalizations;

  std::string line;
  while (std::getline(mifs, line)) {
    std::vector<std::string> iniFinChk = SplitStr(line, " : ", true);
    Assert(
        iniFinChk.size() == 3,
        "Invalid mapping line, each line should be in the format of "
        "initialisation : finalization : checksum. The invalid line is: %s",
        line.c_str()
    );

    std::vector<std::string> ini = SplitStr(iniFinChk[0], ",", true);
    std::vector<std::string> fin = SplitStr(iniFinChk[1], ",", true);
    Assert(ini.size() == fin.size(), "The size of initialisation and finalization is different");

    initialisations.emplace_back(ini.size());
    std::ranges::transform(ini, initialisations.back().begin(), [](const auto &s) -> int {
      return std::stoi(s);
    });
    finalizations.emplace_back(fin.size());
    std::ranges::transform(fin, finalizations.back().begin(), [](const auto &s) -> int {
      return std::stoi(s);
    });

    // Ensure our checksum is correct
    // int chksum = std::stoi(iniFinChk[2]);
    // Assert(
    //     chksum == StatelessChecksum::Compute(finalizations.back()),
    //     "The checksum %d does not match the finalization [%s]", chksum, JoinStr(fin, ",").c_str()
    // );
  }

  mifs.close();

  return InitFinaMap(std::move(initialisations), std::move(finalizations));
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"

std::vector<struct bpf_insn> FunPlus::GenerateFuneBPFCode(
    const UBFreeExec &exec, std::vector<struct bpf_insn> *provided_insns_buf
) const {
  using u8 = std::uint8_t;
  using u32 = std::uint32_t;

  Assert(exec.GetOwner() == this, "The execution does not belong to this function!");
  const auto *fun = exec.GetFun();
  Assert(fun != nullptr, "Function is not generated yet!");

  const auto &initializations = exec.GetInitializations();
  const auto &finalizations = exec.GetFinalizations();
  Assert(
      initializations.size() == finalizations.size(),
      "Initializations and finalizations must have the same size"
  );
  // The ctx pointer of R1 is preserved, so we don't generate it.
  Assert(initializations.size() <= 4, "Four parameters are supported for eBPF");

  /* eBPF Program Layout:
   *
   *  ╭─────────────────────────────────────────────────────────╮
   *  │                    MAIN FUNCTION                        │
   *  ├─────────────────────────────────────────────────────────┤
   *  │  R9 = 0  (counter initialization)                       │
   *  │                                                         │
   *  │  ┌─ FOR EACH TEST CASE ─────────────────────────────┐   │
   *  │  │  • Setup params (R2, R3, R4, R5)                 │   │
   *  │  │  • CALL generated_func                           │   │
   *  │  │  • Oracle: if (R0 != expected_csum) skip         │   │
   *  │  │  • R9++  (increment counter)                     │   │
   *  │  └──────────────────────────────────────────────────┘   │
   *  │                                                         │
   *  │  Final Oracle:                                          │
   *  │  • if (R9 != num_tests) exit(normal)                    │
   *  │  • R10 = 0  (bug detected signal)                       │
   *  │  • exit                                                 │
   *  ├─────────────────────────────────────────────────────────┤
   *  │                GENERATED FUNCTION                       │
   *  ├─────────────────────────────────────────────────────────┤
   *  │  Register Layout:                                       │
   *  │  • R2-R5: function parameters                           │
   *  │  • R6-R7: local variables                               │
   *  │  • R8 (AX0): temp for term results                      │
   *  │  • R9 (AX1): temp for expression results                │
   *  │                                                         │
   *  │  Function Body (from SymIR lowering):                   │
   *  │  • Param/Local initialization                           │
   *  │  • Basic blocks with control flow                       │
   *  │  • Arithmetic operations                                │
   *  │  • Return: XOR checksum of finals in R0                 │
   *  ╰─────────────────────────────────────────────────────────╯
   */

  std::vector<struct bpf_insn> local_insns_buf;
  std::vector<struct bpf_insn> *prog = provided_insns_buf ? provided_insns_buf : &local_insns_buf;

  prog->push_back(BPF_MOV32_IMM(BPF_REG_9, 0));

  std::vector<int> call_fixups;
  for (size_t i = 0; i < initializations.size(); i++) {
    const auto &init = initializations[i];
    const auto &fina = finalizations[i];
    const auto numParams = static_cast<u8>(init.size());
    for (auto j = 0; j < numParams; j++) {
      prog->push_back(BPF_MOV32_IMM(BPF_REG_2 + j, init[j]));
    }
    call_fixups.push_back(prog->size());
    prog->push_back(BPF_CALL_REL(0));

    u32 csum = 0;
    for (auto x: fina) {
      csum ^= x;
    }
    // counter
    prog->push_back(BPF_JMP32_IMM(BPF_JNE, BPF_REG_0, csum, 1));
    prog->push_back(BPF_ALU32_IMM(BPF_ADD, BPF_REG_9, 1));
  }
  // oracle
  prog->push_back(BPF_JMP32_IMM(BPF_JNE, BPF_REG_9, initializations.size(), 1));
  prog->push_back(BPF_MOV32_IMM(BPF_REG_10, 0));
  prog->push_back(BPF_EXIT_INSN());

  for (size_t i = 0; i < call_fixups.size(); i++)
    prog->at(call_fixups[i]).imm = prog->size() - call_fixups[i] - 1;

  symir::eBPFLower lower(*prog);
  lower.Lower(*fun); // append the real prog

  return local_insns_buf; // empty if `provided_insns_buf` exists
}

#pragma GCC diagnostic pop
