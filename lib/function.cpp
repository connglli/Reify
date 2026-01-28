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
  auto randArrayDim = Random::Get().Uniform(1, 2);
  auto randArrayLen = Random::Get().Uniform(1, 3);

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
    bool isStruct = false;

    if (GlobalOptions::Get().EnableVolatileVars &&
        randProba() < GlobalOptions::Get().VolatileVariableProba) {
      isVolatile = true;
    }

    // We prioritize Struct over Array for now, or make them mutually exclusive?
    // Let's make them mutually exclusive for simplicity.
    if (GlobalOptions::Get().EnableStructVars &&
        randProba() < GlobalOptions::Get().StructVariableProba) {
      isStruct = true;
    } else if (GlobalOptions::Get().EnableArrayVars && randProba() < GlobalOptions::Get().ArrayVariableProba) {
      isArray = true;
    }

    if (isStruct) {
      std::string structName = pickOrGenerateStruct(builder.get());
      if (!structName.empty()) {
        builder->SymStructParam(NameVar(i), structName);
      } else {
        // Fallback to scalar
        builder->SymScaParam(NameVar(i), symir::SymIR::Type::I32, isVolatile);
      }
    } else if (isArray) {
      std::vector<int> shape;
      for (int j = 0; j < randArrayDim(); j++) {
        shape.push_back(randArrayLen());
      }
      builder->SymVecParam(NameVar(i), shape, symir::SymIR::Type::I32, "", isVolatile);
    } else {
      builder->SymScaParam(NameVar(i), symir::SymIR::Type::I32, isVolatile);
    }
  }
  // The next numLocals variables are locals
  for (int i = numParams; i < numParams + numLocals; i++) {
    bool isVolatile = false;
    bool isArray = false;
    bool isStruct = false;

    if (GlobalOptions::Get().EnableVolatileVars &&
        randProba() < GlobalOptions::Get().VolatileVariableProba) {
      isVolatile = true;
    }

    if (GlobalOptions::Get().EnableStructVars &&
        randProba() < GlobalOptions::Get().StructVariableProba) {
      isStruct = true;
    } else if (GlobalOptions::Get().EnableArrayVars && randProba() < GlobalOptions::Get().ArrayVariableProba) {
      isArray = true;
    }

    if (isStruct) {
      std::string structName = pickOrGenerateStruct(builder.get());
      if (!structName.empty()) {
        const auto *sDef = builder->FindStruct(structName);

        // Recursive initialization helper
        std::function<void(std::string, const symir::StructDef *, std::vector<symir::Coef *> &)>
            initStruct = [&](std::string prefix, const symir::StructDef *sd,
                             std::vector<symir::Coef *> &acc) {
              for (const auto &field: sd->GetFields()) {
                std::string fName = prefix + "_" + field.name;
                int numEls = 1;
                if (!field.shape.empty())
                  numEls = symir::VarDef::GetVecNumEls(field.shape);

                for (int k = 0; k < numEls; ++k) {
                  std::string elName =
                      fName + (field.shape.empty() ? "" : "_el" + std::to_string(k));
                  if (field.baseType == symir::SymIR::Type::STRUCT ||
                      field.type == symir::SymIR::Type::STRUCT) {
                    // field.type could be ARRAY (with baseType=STRUCT) or STRUCT
                    const auto *innerDef = builder->FindStruct(field.structName);
                    initStruct(elName, innerDef, acc);
                  } else {
                    acc.push_back(builder->SymCoef(elName, symir::SymIR::Type::I32));
                    // acc.push_back(builder->SymI32Const(0));
                  }
                }
              }
            };

        std::vector<symir::Coef *> inits;
        initStruct("init_var" + std::to_string(i), sDef, inits);

        builder->SymStructLocal(NameVar(i), structName, inits);
      } else {
        builder->SymScaLocal(
            NameVar(i), builder->SymCoef("init_var" + std::to_string(i), symir::SymIR::Type::I32),
            symir::SymIR::Type::I32, isVolatile
        );
      }
    } else if (isArray) {
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
      builder->SymVecLocal(NameVar(i), shape, inits, symir::SymIR::Type::I32, "", isVolatile);
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

std::string FunPlus::pickOrGenerateStruct(symir::FunctBuilder *funBd) {
  if (!GlobalOptions::Get().EnableStructVars) {
    return "";
  }

  // Helper for recursive generation
  std::function<std::string(int)>
      generate =
          [&](int depth
          ) -> std::
                string {
                  auto availStructs = funBd->GetStructs();
                  // Allow picking existing if available. If depth limit reached, MUST pick existing
                  // or avoid Struct.
                  bool canPick = !availStructs.empty();
                  bool mustPick = depth >= 2;

                  // Probabilities
                  bool pickExisting =
                      canPick && (mustPick || (Random::Get().UniformReal()() < 0.3));

                  if (pickExisting) {
                    int idx = Random::Get().Uniform(0, (int) availStructs.size() - 1)();
                    return availStructs[idx]->GetName();
                  } else if (mustPick) {
                    return ""; // Indicates fallback to I32 (or non-struct)
                  }

                  // Generate New Struct
                  std::vector<symir::StructDef::Field> fields;
                  int numFields =
                      Random::Get().Uniform(1, GlobalOptions::Get().MaxNumStructFields)();

                  // Reduce array sizes to avoid OOM with nesting
                  auto randArrayDim = Random::Get().Uniform(1, 2);
                  auto randArrayLen = Random::Get().Uniform(1, 2);
                  auto randProba = Random::Get().UniformReal();

                  for (int j = 0; j < numFields; ++j) {
                    symir::StructDef::Field field;
                    field.name = "f" + std::to_string(j);

                    float p = randProba();
                    bool fieldIsArray = false;
                    bool fieldIsStruct = false;

                    if (p < 0.3) {                                                // Scalar I32
                    } else if (p < 0.5 && GlobalOptions::Get().EnableArrayVars) { // Array I32
                      fieldIsArray = true;
                    } else if (p < 0.7 && GlobalOptions::Get().EnableStructVars) { // Struct
                      fieldIsStruct = true;
                    } else if (p < 0.9 && GlobalOptions::Get().EnableArrayVars &&
                 GlobalOptions::Get().EnableStructVars) { // Array Struct
                      fieldIsArray = true;
                      fieldIsStruct = true;
                    }

                    if (fieldIsStruct) {
                      std::string sName = generate(depth + 1);
                      if (sName.empty()) {
                        field.type =
                            fieldIsArray ? symir::SymIR::Type::ARRAY : symir::SymIR::Type::I32;
                        field.baseType = symir::SymIR::Type::I32;
                      } else {
                        field.structName = sName;
                        field.type =
                            fieldIsArray ? symir::SymIR::Type::ARRAY : symir::SymIR::Type::STRUCT;
                        field.baseType = symir::SymIR::Type::STRUCT;
                      }
                    } else {
                      field.type =
                          fieldIsArray ? symir::SymIR::Type::ARRAY : symir::SymIR::Type::I32;
                      field.baseType = symir::SymIR::Type::I32;
                    }

                    if (fieldIsArray) {
                      for (int k = 0; k < randArrayDim(); k++) {
                        field.shape.push_back(randArrayLen());
                      }
                    }

                    fields.push_back(field);
                  }

                  std::string structName = "S" + std::to_string(funBd->GetStructs().size());
                  funBd->SymStruct(structName, fields);
                  return structName;
                };

  return generate(0);
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
  std::vector<symir::Coef *> access;

  std::function<
      void(symir::SymIR::Type, symir::SymIR::Type, std::string, const std::vector<int> &, int)>
      walk = [&](symir::SymIR::Type type, symir::SymIR::Type baseType, std::string sName,
                 const std::vector<int> &shape, int depth) {
        // 1. Handle Array Dimensions
        if (!shape.empty()) {
          for (size_t d = 0; d < shape.size(); ++d) {
            // Use the last element of each dimension (to avoid out-of-bounds)
            int idx = shape[d] - 1;
            access.push_back(funBd->SymI32Const(idx));
          }
          // After consuming all array dimensions, continue with the base type
          type = baseType;
        }

        // 2. Handle Struct - drill down to a scalar field
        if (type == symir::SymIR::Type::STRUCT) {
          const auto *sDef = funBd->FindStruct(sName);
          Assert(sDef, "Struct definition not found for %s", sName.c_str());
          int numFields = sDef->GetFields().size();
          Assert(numFields > 0, "Struct %s has no fields, cannot generate access", sName.c_str());
          int fieldIdx = Random::Get().Uniform(0, numFields - 1)();
          access.push_back(funBd->SymI32Const(fieldIdx));
          const auto &field = sDef->GetField(fieldIdx);
          // Recurse with the field's type, baseType, and shape
          walk(field.type, field.baseType, field.structName, field.shape, depth + 1);
        }

        // If type is a scalar (I32, etc.), we're done - no more access needed
      };

  walk(
      var->GetType(), var->GetBaseType(),
      (var->GetType() == symir::SymIR::STRUCT || var->GetBaseType() == symir::SymIR::STRUCT
           ? var->GetStructName()
           : ""),
      var->GetVecShape(), 0
  );

  return access;
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

  const auto &params = fun->GetParams();
  for (size_t i = 0; i < initializations.size(); i++) {
    const auto &init = initializations[i];
    const auto &fina = finalizations[i];
    const auto numParams = static_cast<int>(init.size());

    // First, declare temporary variables for array/struct parameters
    std::vector<std::string> argNames;
    for (int j = 0; j < numParams; j++) {
      const auto *param = params[j];
      const auto &arg = init[j];

      if (arg.IsVector()) {
        // Array parameter - need a temporary variable
        std::string tmpName = "tmp_" + std::to_string(i) + "_" + std::to_string(j);
        argNames.push_back(tmpName);

        // Get the array type from the parameter
        const auto *vecParam = dynamic_cast<const symir::VecParam *>(param);
        Assert(vecParam != nullptr, "Expected VecParam for array argument");

        // Generate the array declaration with proper dimensions
        main << "  ";
        if (vecParam->GetBaseType() == symir::SymIR::STRUCT) {
          main << "struct " << vecParam->GetStructName();
        } else {
          main << symir::SymIR::GetTypeCName(vecParam->GetBaseType());
        }
        main << " " << tmpName;
        for (int dim: vecParam->GetVecShape()) {
          main << "[" << dim << "]";
        }
        main << " = " << arg.ToCxStr() << ";" << std::endl;
      } else if (arg.IsStruct()) {
        // Struct parameter - can use compound literal directly in C99
        argNames.push_back(arg.ToCxStr());
      } else {
        // Scalar parameter - pass directly
        argNames.push_back(arg.ToCxStr());
      }
    }

    // Now generate the function call with proper arguments
    main << "  " << StatelessChecksum::GetCheckChksumName() << "("
         << StatelessChecksum::Compute(fina) << ", " << fun->GetName() << "(";
    for (int j = 0; j < numParams; j++) {
      main << argNames[j];
      if (j != numParams - 1) {
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
