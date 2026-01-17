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
#include "lib/jnifutils.hpp"
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
  auto randArrayDim = Random::Get().Uniform(1, GlobalOptions::Get().MaxNumArrayDims);
  auto randArrayLen = Random::Get().Uniform(1, GlobalOptions::Get().MaxNumElsPerArrayDim);

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
    bool isVolatile = false;
    bool isArray = false;
    if (GlobalOptions::Get().EnableVolatileVars &&
        randProba() < GlobalOptions::Get().VolatileVariableProba) {
      isVolatile = true;
    }
    if (GlobalOptions::Get().EnableArrayVars &&
        randProba() < GlobalOptions::Get().ArrayVariableProba) {
      isArray = true;
    }
    if (isArray) {
      std::vector<int> shape;
      for (int j = 0; j < randArrayDim(); j++) {
        shape.push_back(randArrayLen());
      }
      builder->SymVecParam(NameVar(i), shape, symir::SymIR::Type::I32, isVolatile);
    } else {
      builder->SymScaParam(NameVar(i), symir::SymIR::Type::I32, isVolatile);
    }
  }
  // The next numLocals variables are locals
  for (int i = numParams; i < numParams + numLocals; i++) {
    bool isVolatile = false;
    bool isArray = false;
    if (GlobalOptions::Get().EnableVolatileVars &&
        randProba() < GlobalOptions::Get().VolatileVariableProba) {
      isVolatile = true;
    }
    if (GlobalOptions::Get().EnableArrayVars &&
        randProba() < GlobalOptions::Get().ArrayVariableProba) {
      isArray = true;
    }
    if (isArray) {
      std::vector<int> shape;
      for (int j = 0; j < randArrayDim(); j++) {
        shape.push_back(randArrayLen());
      }
      std::vector<symir::Coef *> inits;
      for (int j = 0; j < symir::VarDef::GetVecNumEls(shape); j++) {
        inits.push_back(builder->SymCoef(
            "init_var" + std::to_string(i) + "_el" + std::to_string(j), symir::SymIR::Type::I32
        ));
      }
      builder->SymVecLocal(NameVar(i), shape, inits, symir::SymIR::Type::I32, isVolatile);
    } else {
      builder->SymScaLocal(
          NameVar(i), builder->SymCoef("init_var" + std::to_string(i), symir::SymIR::Type::I32),
          symir::SymIR::Type::I32, isVolatile
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
      int whichId = i + 1;
      const auto depCoef =
          funBd->SymCoef(NameCoef(bblId, stmtIndex, whichId), symir::SymIR::Type::I32);
      auto depVarAcc = generateVarAccess(funBd, depVar, bblId, stmtIndex, whichId);
      terms.push_back(
          bblBd->SymTerm(static_cast<symir::Term::Op>(randTermOp()), depCoef, depVar, depVarAcc)
      );
    }
    // Assign a constant to the current variable being defined
    terms.push_back(bblBd->SymCstTerm(
        funBd->SymCoef(NameConst(bblId, stmtIndex), symir::SymIR::Type::I32), nullptr
    ));

    // Create the assignment statement with a random expression
    std::vector<symir::Coef *> assAcc = generateVarAccess(funBd, var, bblId, stmtIndex, 0);
    bblBd->SymAssign(
        var, bblBd->SymExpr(static_cast<symir::Expr::Op>(randExprOp()), terms), assAcc
    );
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
      int whichId = i + 1;
      const auto depCoef =
          funBd->SymCoef(NameCondCoef(bblId, numAssPerBbl, whichId), symir::SymIR::Type::I32);
      std::vector<symir::Coef *> depVarAcc =
          generateVarAccess(funBd, depVar, bblId, numAssPerBbl, whichId);
      condTerms.push_back(
          bblBd->SymTerm(static_cast<symir::Term::Op>(randTermOp()), depCoef, depVar, depVarAcc)
      );
    }
    condTerms.push_back(bblBd->SymCstTerm(
        funBd->SymCoef(NameCondConst(bblId, numAssPerBbl), symir::SymIR::Type::I32), nullptr
    ));

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

std::vector<symir::Coef *> FunPlus::generateVarAccess(
    symir::FunctBuilder *funBd, const symir::VarDef *var, int bbl, int stmt, int which
) const {
  if (var->IsScalar()) {
    return {};
  }
  std::vector<symir::Coef *> coef;
  for (int dim = 0; dim < var->GetVecNumDims(); dim++) {
    coef.push_back(funBd->SymCoef(NameAccess(bbl, stmt, which, dim), symir::SymIR::Type::I32));
  }
  return coef;
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
  oss << "extern " << StatelessChecksum::GetComputePrototype() << ";" << std::endl;
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

// TODO: This implementation is exactly the same as SymJavaBytecodeLower::CreateArray,
//       we should refactor it to avoid code duplication.
void createArray(jnif::ClassFile &cls, jnif::Method &met, const ArgPlus<int> &var) {
  Assert(var.IsVector(), "The variable for creating array is a scalar");
  const std::function<void(int, int)> createArray = [&](const int dim, const int offset) {
    const auto numDims = var.GetVecNumDims();
    const auto dimLen = var.GetVecDimLen(dim);
    const auto dimNumEls = var.GetVecNumEls(dim);
    if (dim == numDims - 1) {
      Assert(dimNumEls == 1, "The last dimension should have exactly one element");
      // The very last dimension is an [I
      jnif::PushInteger(cls, met, dimLen);
      met.instList().addNewArray(jnif::model::NewArrayInst::NewArrayType::NEWARRAYTYPE_INT);
      // Fill each element with its initialization value
      for (int i = 0; i < dimLen; i++) {
        met.instList().addZero(jnif::Opcode::dup);
        jnif::PushInteger(cls, met, i);
        jnif::PushInteger(cls, met, var.GetValue(offset + i));
        met.instList().addZero(jnif::Opcode::iastore);
      }
      return;
    }
    // The other dimensions are [[...[[I.
    jnif::PushInteger(cls, met, dimLen);
    met.instList().addType(
        jnif::Opcode::anewarray, cls.putClass((std::string(numDims - dim - 1, '[') + "I").c_str())
    );
    // Fill each element with a sub-array
    for (int i = 0; i < dimLen; i++) {
      met.instList().addZero(jnif::Opcode::dup);
      jnif::PushInteger(cls, met, i);
      createArray(dim + 1, offset + i * dimNumEls);
      met.instList().addZero(jnif::Opcode::aastore);
    }
  };
  createArray(0, 0);
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

  const auto &initializations = exec.GetInits();
  const auto &finalizations = exec.GetFinas();
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
    // Java's CRC32 is exactly the same as C/C++'s one, so we can directly
    // compute the checksum in C/C++ and pass it to Java.
    const auto checksum = StatelessChecksum::Compute(fina);
    jnif::PushInteger(*clazz, *mainMethod, checksum);
    for (const auto &a: init) {
      if (a.IsVector()) {
        createArray(*clazz, *mainMethod, a);
      } else {
        jnif::PushInteger(*clazz, *mainMethod, a.GetValue());
      }
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

  const auto &initializations = exec.GetInits();
  const auto &finalizations = exec.GetFinas();
  Assert(
      initializations.size() == finalizations.size(),
      "Initializations and finalizations must have the same size"
  );
  std::ostringstream main;

  // Generate the checksum code
  main << StatelessChecksum::GetRawCode() << std::endl;
  main << StatelessChecksum::GetCheckChksumCode(debug) << std::endl;

  // Declare the function prototype
  main << "extern " << symir::SymCxLower::GetFunPrototype(*fun) << ";" << std::endl << std::endl;

  // Generate the main function
  main << "int main(int argc, char* argv[])" << std::endl;
  main << "{" << std::endl;
  main << "  " << StatelessChecksum::GetCrc32InitName() << "();" << std::endl;
  for (size_t i = 0; i < initializations.size(); i++) {
    const auto &init = initializations[i];
    const auto &fina = finalizations[i];
    const auto numParams = static_cast<int>(init.size());
    std::ostringstream chk_oss;
    main << "  " << StatelessChecksum::GetCheckChksumName() << "("
         << StatelessChecksum::Compute(fina) << ", " << fun->GetName() << "(";
    for (auto j = 0; j < numParams; j++) {
      main << init[j].ToCxStr();
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
  for (size_t i = 0; i < exec.GetInits().size(); i++) {
    nlohmann::json mapObj = nlohmann::json::object();
    const auto &init = exec.GetInits()[i];
    mapObj["ini"] = nlohmann::json::array();
    for (const auto &arg: init) {
      mapObj["ini"].push_back(arg.ToJson());
    }
    const auto &fina = exec.GetFinas()[i];
    mapObj["fin"] = nlohmann::json::array();
    for (const auto &arg: fina) {
      mapObj["fin"].push_back(arg.ToJson());
    }
    mapObj["chk"] = StatelessChecksum::Compute(fina);
    mapping << mapObj.dump() << std::endl;
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

FunPlus::IniFinMap FunPlus::ParseMappingCode(const std::string &mapPath) {
  std::ostringstream moss;
  std::ifstream mifs(mapPath);
  Assert(mifs.is_open(), "Error: failed to open file: %s", mapPath.c_str());

  std::vector<std::vector<ArgPlus<int>>> inits;
  std::vector<std::vector<ArgPlus<int>>> finas;

  std::string line;
  while (std::getline(mifs, line)) {
    nlohmann::json mapObj = nlohmann::json::parse(line);

    Assert(mapObj.contains("ini"), "Mapping object does not contain an field 'ini'");
    Assert(
        mapObj["ini"].is_array(), "The field 'ini' is not an array but a %s",
        mapObj["ini"].type_name()
    );
    Assert(mapObj.contains("fin"), "Mapping object does not contain an field 'fin'");
    Assert(
        mapObj["fin"].is_array(), "The field 'fin' is not an array but a %s",
        mapObj["fin"].type_name()
    );
    Assert(mapObj.contains("chk"), "Mapping object does not contain an field 'chk'");
    Assert(
        mapObj["chk"].is_number_integer(), "The field 'chk' is not an integer but a %s",
        mapObj["chk"].type_name()
    );

    std::vector<ArgPlus<int>> ini;
    for (const auto &argObj: mapObj["ini"]) {
      ini.push_back(ArgPlus<int>::FromJson(argObj));
    }

    std::vector<ArgPlus<int>> fin;
    for (const auto &argObj: mapObj["fin"]) {
      fin.push_back(ArgPlus<int>::FromJson(argObj));
    }

    // Ensure our checksum is correct
    int chk1 = mapObj["chk"];
    int chk2 = StatelessChecksum::Compute(fin);
    Assert(chk1 == chk2, "The saved checksum %d does not match the finalization (%d)", chk1, chk2);

    inits.push_back(std::move(ini));
    finas.push_back(std::move(fin));
  }

  mifs.close();

  return IniFinMap(std::move(inits), std::move(finas));
}
