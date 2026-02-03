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

#include "lib/ubinject.hpp"

#include <cstring>
#include <memory>
#include "global.hpp"

std::unique_ptr<symir::Funct> IntUBInject::InjectUBs(const symir::Funct *f, const symir::Block *b) {
  Assert(b != nullptr, "The basic block to inject UBs is a nullptr");
  currentFun = f;

  Log::Get().OpenSection("IntUBInject: Inject Int UBs to Block " + b->GetLabel());

  Reset();

  // Create a new solver instance with model production enabled
  bitwuzla::Options opts;
  opts.set(bitwuzla::Option::PRODUCE_MODELS, true);
  solver = std::make_unique<bitwuzla::Bitwuzla>(*tm, opts);

  // Collect constraints that'll introduce int-related UBs
  ensureValidInitsForUses(b);
  b->Accept(*this);
  Log::Get().Out() << "Collected constraints:" << std::endl;
  for (const auto &c: constraints) {
    Log::Get().Out() << c << std::endl;
  }

  // Check if we can find such a solution
  for (const auto &c: constraints) {
    solver->assert_formula(c);
  }
  if (solver->check_sat() != bitwuzla::Result::SAT) {
    Log::Get().Out() << "UNSAT" << std::endl;
    currentFun = nullptr;
    return nullptr;
  }
  Log::Get().Out() << "SAT" << std::endl;

  // Insert assignments to the used variables
  // clang-format off
  auto nf = symir::FunctCopier(
    f,
    nullptr,
    [&](symir::FunctBuilder *fbd, symir::BlockBuilder *bbd) {
      if (bbd->GetLabel() != b->GetLabel()) {
        return; // We only inject UBs into the given block
      }
      Log::Get().Out()
          << "Adding initializations to ensure undefined behaviors" << std::endl;
      // We need the old block b since we are at the starting point of the new block
      // which has no statements and therefore no uses/definitions can be found.
      this->extractAndInitializeUses(b, fbd, bbd);
    },
    nullptr,
    nullptr
  ).Copy();
  // clang-format on
  // Now we need to extract the symbols (solved coefficients) from our model
  extractSymbolsFromModel(nf.get(), nf->FindBlock(b->GetLabel()));

  Log::Get().CloseSection();
  currentFun = nullptr;

  return nf;
}

void IntUBInject::Visit(const symir::VarUse &v) {
  Assert(v.GetType() == symir::SymIR::I32, "Only 32-bit integer variables are supported for now!");
  if (v.IsScalar()) {
    if (v.GetDef()->GetType() == symir::SymIR::STRUCT) {
      const auto access = v.GetAccess();
      Assert(!access.empty(), "Struct access must have index");
      const auto *c = access[0];
      Assert(c->IsSolved(), "Struct access index must be solved");

      int idx = c->GetI32Value();
      const auto *sDef = currentFun->GetStruct(v.GetDef()->GetStructName());
      std::string fieldName = sDef->GetField(idx).name;
      pushExpression(CreateStructFieldExpr(v.GetDef(), fieldName));
    } else {
      // We're not going to restrict the value of any variable
      pushExpression(CreateScaExpr(v.GetDef()));
    }
  } else {
    const auto access = v.GetAccess();
    // We asked them to access the very last element
    for (int d = 0; d < v.GetVecNumDims(); d++) {
      access[d]->Accept(*this);
      auto coefExpr = popExpression();
      constraints.push_back(tm->mk_term(
          bitwuzla::Kind::EQUAL, {coefExpr, tm->mk_bv_value_int64(bvSort, v.GetVecDimLen(d) - 1)}
      ));
    }
    auto elExpr = CreateVecElExpr(v.GetDef(), v.GetVecNumEls() - 1);
    // We're not going to restrict the value of any element
    pushExpression(elExpr);
  }
}

void IntUBInject::Visit(const symir::Coef &c) {
  Assert(c.GetType() == symir::SymIR::I32, "Only 32-bit integer variables are supported for now!");
  const auto coefExpr = CreateCoefExpr(c);
  if (!c.IsSolved()) {
    // For the generated program, we'd like it as seemly "normal" as possible so that
    // both developers and compilers treat it as a "normal" program at a first galance.
    // So, let's make the coefficient to be in a range that is not too weird.
    constraints.push_back(tm->mk_term(
        bitwuzla::Kind::BV_SGE,
        {coefExpr, tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().LowerBound)}
    ));
    constraints.push_back(tm->mk_term(
        bitwuzla::Kind::BV_SLE,
        {coefExpr, tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().UpperBound)}
    ));
  }
  pushExpression(coefExpr);
}

void IntUBInject::Visit(const symir::Term &t) {
  t.GetCoef()->Accept(*this);
  const auto coefExpr = popExpression();
  bitwuzla::Term varExpr = tm->mk_bv_value_int64(bvSort, 0);
  if (t.GetVar() != nullptr) {
    t.GetVar()->Accept(*this);
    varExpr = popExpression();
  }
  switch (t.GetOp()) {
    case symir::Term::OP_CST:
      pushExpression(coefExpr);
      break;

    case symir::Term::OP_ADD:
      pushExpression(tm->mk_term(bitwuzla::Kind::BV_ADD, {coefExpr, varExpr}));
      break;

    case symir::Term::OP_SUB:
      pushExpression(tm->mk_term(bitwuzla::Kind::BV_SUB, {coefExpr, varExpr}));
      break;

    case symir::Term::OP_MUL:
      pushExpression(tm->mk_term(bitwuzla::Kind::BV_MUL, {coefExpr, varExpr}));
      break;

    case symir::Term::OP_DIV:
      constraints.push_back(
          tm->mk_term(bitwuzla::Kind::DISTINCT, {varExpr, tm->mk_bv_value_int64(bvSort, 0)})
      ); // We disallow division by zero
      pushExpression(tm->mk_term(bitwuzla::Kind::BV_SDIV, {coefExpr, varExpr}));
      break;

    case symir::Term::OP_REM:
      constraints.push_back(
          tm->mk_term(bitwuzla::Kind::DISTINCT, {varExpr, tm->mk_bv_value_int64(bvSort, 0)})
      ); // We disallow division by zero
      pushExpression(tm->mk_term(bitwuzla::Kind::BV_SREM, {coefExpr, varExpr}));
      break;

    default:
      Panic("Unknown term operation: %s", symir::Term::GetOpName(t.GetOp()).c_str());
      break;
  }
}

void IntUBInject::Visit(const symir::Expr &e) {
  std::vector<bitwuzla::Term> exprs;
  for (const auto t: e.GetTerms()) {
    t->Accept(*this);
    exprs.push_back(popExpression());
  }
  bitwuzla::Term result = exprs.back();
  exprs.pop_back();
  for (int i = static_cast<int>(exprs.size()) - 1; i >= 0; --i) {
    switch (e.GetOp()) {
      case symir::Expr::OP_ADD:
        result = tm->mk_term(bitwuzla::Kind::BV_ADD, {result, exprs[i]});
        break;

      case symir::Expr::OP_SUB:
        result = tm->mk_term(bitwuzla::Kind::BV_SUB, {result, exprs[i]});
        break;

      default:
        Panic("Unknown expression operation: %s", symir::Expr::GetOpName(e.GetOp()).c_str());
        break;
    }
  }
  pushExpression(result);
}

void IntUBInject::Visit(const symir::Cond &c) {
  c.GetExpr()->Accept(*this);
  const auto expr = popExpression();
  bitwuzla::Term upperBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().UpperBound);
  bitwuzla::Term lowerBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().LowerBound);
  bitwuzla::Term overflowHigh = tm->mk_term(bitwuzla::Kind::BV_SGT, {expr, upperBound});
  bitwuzla::Term overflowLow = tm->mk_term(bitwuzla::Kind::BV_SLT, {expr, lowerBound});
  ubConstraints.push_back(tm->mk_term(bitwuzla::Kind::OR, {overflowHigh, overflowLow})
  ); // Inserts signed overflow
  switch (c.GetOp()) {
    case symir::Cond::OP_EQZ:
      pushExpression(tm->mk_term(bitwuzla::Kind::EQUAL, {expr, tm->mk_bv_value_int64(bvSort, 0)}));
      break;

    case symir::Cond::OP_GTZ:
      pushExpression(tm->mk_term(bitwuzla::Kind::DISTINCT, {expr, tm->mk_bv_value_int64(bvSort, 0)})
      );
      break;

    case symir::Cond::OP_LTZ:
      pushExpression(tm->mk_term(bitwuzla::Kind::BV_SLT, {expr, tm->mk_bv_value_int64(bvSort, 0)}));
      break;

    default:
      break;
  }
}

void IntUBInject::Visit(const symir::AssStmt &a) {
  a.GetExpr()->Accept(*this);
  const auto expr = popExpression();
  bitwuzla::Term upperBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().UpperBound);
  bitwuzla::Term lowerBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().LowerBound);
  bitwuzla::Term overflowHigh = tm->mk_term(bitwuzla::Kind::BV_SGT, {expr, upperBound});
  bitwuzla::Term overflowLow = tm->mk_term(bitwuzla::Kind::BV_SLT, {expr, lowerBound});
  ubConstraints.push_back(tm->mk_term(bitwuzla::Kind::OR, {overflowHigh, overflowLow})
  ); // Inserts signed overflow
  if (a.GetVar()->IsScalar()) {
    if (a.GetVar()->GetDef()->GetType() == symir::SymIR::STRUCT) {
      const auto access = a.GetVar()->GetAccess();
      Assert(!access.empty(), "Struct access must have index");
      int idx = access[0]->GetI32Value();
      const auto *sDef = currentFun->GetStruct(a.GetVar()->GetDef()->GetStructName());
      std::string fieldName = sDef->GetField(idx).name;
      auto varNewExpr = CreateStructFieldExpr(a.GetVar()->GetDef(), fieldName, -2);
      constraints.push_back(tm->mk_term(bitwuzla::Kind::EQUAL, {varNewExpr, expr}));
    } else {
      auto varNewExpr = CreateScaExpr(a.GetVar()->GetDef(), -2);
      constraints.push_back(tm->mk_term(bitwuzla::Kind::EQUAL, {varNewExpr, expr}));
    }
  } else {
    // We only access the very last element. See Visit(const symir::VarUse &v).
    auto elNewExpr = CreateVecElExpr(a.GetVar()->GetDef(), a.GetVar()->GetVecNumEls() - 1, -2);
    constraints.push_back(tm->mk_term(bitwuzla::Kind::EQUAL, {elNewExpr, expr}));
  }
}

void IntUBInject::Visit(const symir::RetStmt &r) {
  // Do nothing
}

void IntUBInject::Visit(const symir::Branch &b) {
  b.GetCond()->Accept(*this);
  popExpression(); // We don't care about the condition of the branch
}

void IntUBInject::Visit(const symir::Goto &g) {
  // Do nothing
}

void IntUBInject::Visit(const symir::ScaParam &p) {
  Panic("Cannot reach here: We only inject UBs into basic blocks, not parameters");
}

void IntUBInject::Visit(const symir::VecParam &p) {
  Panic("Cannot reach here: We only inject UBs into basic blocks, not parameters");
}

void IntUBInject::Visit(const symir::ScaLocal &l) {
  Panic("Cannot reach here: We only inject UBs into basic blocks, not locals");
}

void IntUBInject::Visit(const symir::VecLocal &l) {
  Panic("Cannot reach here: We only inject UBs into basic blocks, not locals");
}

void IntUBInject::Visit(const symir::StructParam &p) {
  Panic("Cannot reach here: We only inject UBs into basic blocks, not parameters");
}

void IntUBInject::Visit(const symir::StructLocal &l) {
  Panic("Cannot reach here: We only inject UBs into basic blocks, not locals");
}

void IntUBInject::Visit(const symir::StructDef &s) {
  Panic("Cannot reach here: We only inject UBs into basic blocks, not struct defs");
}

void IntUBInject::Visit(const symir::Block &b) {
  for (const auto s: b.GetStmts()) {
    s->Accept(*this);
  }
  Assert(
      exprStack.empty(), "The expression stack is not empty after visiting the block %s",
      b.GetLabel().c_str()
  );
  // As long as one of the UB constraints is true, we have an UB.
  bitwuzla::Term ubc = tm->mk_false();
  for (auto &c: ubConstraints) {
    ubc = tm->mk_term(bitwuzla::Kind::OR, {ubc, c});
  }
  constraints.push_back(ubc);
}

void IntUBInject::Visit(const symir::Funct &f) {
  Panic("Cannot reach here: We only inject UBs into basic blocks, not functions");
}

void IntUBInject::ensureValidInitsForUses(const symir::Block *b) {
  for (const auto uv: b->GetUses()) {
    bitwuzla::Term varExpr = tm->mk_bv_value_int64(bvSort, 0);
    if (uv->IsScalar()) {
      if (uv->GetDef()->GetType() == symir::SymIR::STRUCT) {
        const auto *sDef = currentFun->GetStruct(uv->GetDef()->GetStructName());
        for (const auto &field: sDef->GetFields()) {
          varExpr = CreateStructFieldExpr(uv->GetDef(), field.name, 0);
          bitwuzla::Term upperBound =
              tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().UpperBound);
          bitwuzla::Term lowerBound =
              tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().LowerBound);
          constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SGE, {varExpr, lowerBound}));
          constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SLE, {varExpr, upperBound}));
        }
        continue;
      } else {
        varExpr = CreateScaExpr(uv->GetDef(), 0);
      }
    } else {
      // We only access the very last element. See Visit(const symir::VarUse &v).
      varExpr = CreateVecElExpr(uv->GetDef(), uv->GetVecNumEls() - 1, 0);
    }
    bitwuzla::Term upperBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().UpperBound);
    bitwuzla::Term lowerBound = tm->mk_bv_value_int64(bvSort, GlobalOptions::Get().LowerBound);
    constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SLE, {varExpr, upperBound}));
    constraints.push_back(tm->mk_term(bitwuzla::Kind::BV_SGE, {varExpr, lowerBound}));
  }
}

void IntUBInject::extractAndInitializeUses(
    const symir::Block *b, symir::FunctBuilder *funBd, symir::BlockBuilder *blkBd
) {
  for (const auto *uv: b->GetUses()) {
    const auto *nuvd = funBd->FindVar(uv->GetName());
    std::string varName;
    bitwuzla::Term varInitExpr = tm->mk_bv_value_int64(bvSort, 0);
    if (uv->IsScalar()) {
      if (nuvd->GetType() == symir::SymIR::STRUCT) {
        const auto access = uv->GetAccess();
        Assert(!access.empty(), "Struct access must have index");
        int idx = access[0]->GetI32Value();
        const auto *sDef = currentFun->GetStruct(nuvd->GetStructName());
        std::string fieldName = sDef->GetField(idx).name;
        varName = GetStructFieldName(nuvd, fieldName);
        varInitExpr = CreateStructFieldExpr(nuvd, fieldName, 0);
      } else {
        varName = uv->GetName();
        varInitExpr = CreateScaExpr(nuvd, 0);
      }
    } else {
      // We only access the very last element. See Visit(const symir::VarUse &v).
      varName = GetVecElName(nuvd, uv->GetVecNumEls() - 1);
      varInitExpr = CreateVecElExpr(nuvd, uv->GetVecNumEls() - 1, 0);
    }
    // Every use has to be solved - get value from the solver
    bitwuzla::Term varValue = solver->get_value(varInitExpr);
    std::string binaryStr = varValue.value<std::string>(2);
    // Convert binary string to signed integer (33-bit)
    int32_t varInitVal = 0;
    if (!binaryStr.empty()) {
      // For 33-bit values, check the MSB and convert appropriately
      // Parse as unsigned 64-bit first
      uint64_t unsigned_val = std::stoull(binaryStr, nullptr, 2);
      // Check if the 33rd bit (sign bit) is set
      if (unsigned_val & (1ULL << 32)) {
        // Negative number - sign extend to 32-bit by masking to lower 32 bits
        // The value already wraps around to the 32-bit signed range
        uint32_t u32 = static_cast<uint32_t>(unsigned_val & 0xFFFFFFFF);
        std::memcpy(&varInitVal, &u32, sizeof(int32_t));
      } else {
        // Positive number - just take lower 32 bits
        uint32_t u32 = static_cast<uint32_t>(unsigned_val & 0xFFFFFFFF);
        std::memcpy(&varInitVal, &u32, sizeof(int32_t));
      }
    }
    Log::Get().Out() << "Let variable: var=" << varName << ", value=" << varInitVal << std::endl;
    // Insert an assignment to the variable with its initial value like: uv <- 0* uv +
    // initial_value
    if (uv->IsScalar()) {
      if (nuvd->GetType() == symir::SymIR::STRUCT) {
        const auto uaccess = uv->GetAccess();
        Assert(!uaccess.empty(), "Struct access must have at least field index");

        // The first access element is always the field index (a constant)
        Assert(uaccess[0]->IsSolved(), "Field index must be a constant");
        int fieldIdx = uaccess[0]->GetI32Value();

        // Get the struct definition to check the field type
        const auto *sDef = funBd->FindStruct(nuvd->GetStructName());
        Assert(sDef, "Struct definition not found");
        const auto &field = sDef->GetField(fieldIdx);

        // Build the access path: start with field index and drill down to scalar
        std::vector<symir::Coef *> access;
        access.push_back(funBd->SymI32Const(fieldIdx));
        // Recursively handle the field's type until we reach a scalar
        symir::SymIR::Type currType = field.type;
        symir::SymIR::Type currBaseType = field.baseType;
        std::string currStructName = field.structName;
        std::vector<int> currShape = field.shape;

        while (currType == symir::SymIR::Type::ARRAY || currType == symir::SymIR::Type::STRUCT) {
          // Handle arrays
          if (currType == symir::SymIR::Type::ARRAY) {
            // Add constant indices for all array dimensions
            for (int dim: currShape) {
              access.push_back(funBd->SymI32Const(dim - 1));
            }
            // After consuming all array dimensions, continue with base type
            currType = currBaseType;
            currShape.clear();
          }
          // Handle structs - drill down to a scalar field
          else if (currType == symir::SymIR::Type::STRUCT) {
            const auto *nestedSDef = funBd->FindStruct(currStructName);
            Assert(nestedSDef, "Nested struct definition not found for %s", currStructName.c_str());
            int numFields = nestedSDef->GetFields().size();
            Assert(numFields > 0, "Struct %s has no fields", currStructName.c_str());

            // Pick the first field (deterministic for UB injection)
            int nestedFieldIdx = 0;
            access.push_back(funBd->SymI32Const(nestedFieldIdx));

            const auto &nestedField = nestedSDef->GetField(nestedFieldIdx);
            currType = nestedField.type;
            currBaseType = nestedField.baseType;
            currStructName = nestedField.structName;
            currShape = nestedField.shape;
          } else {
            // Should never reach here
            Assert(false, "Unexpected type in UB injection struct field drilling");
          }
        }

        blkBd->SymAssign(
            nuvd,
            blkBd->SymAddExpr(
                {blkBd->SymMulTerm(funBd->SymI32Const(0), nuvd, access),
                 blkBd->SymCstTerm(funBd->SymI32Const(varInitVal), nullptr)}
            ),
            access
        );
      } else {
        blkBd->SymAssign(
            nuvd, blkBd->SymAddExpr(
                      {blkBd->SymMulTerm(funBd->SymI32Const(0), nuvd),
                       blkBd->SymCstTerm(funBd->SymI32Const(varInitVal), nullptr)}
                  )
        );
      }
    } else {
      std::vector<symir::Coef *> access;
      for (int d = 0; d < uv->GetVecNumDims(); d++) {
        access.push_back(funBd->SymI32Const(uv->GetVecDimLen(d) - 1));
      }
      blkBd->SymAssign(
          nuvd,
          blkBd->SymAddExpr(
              {blkBd->SymMulTerm(funBd->SymI32Const(0), nuvd, access),
               blkBd->SymCstTerm(funBd->SymI32Const(varInitVal), nullptr)}
          ),
          access
      );
    }
  }
}

void IntUBInject::extractSymbolsFromModel(const symir::Funct *f, const symir::Block *b) {
  for (auto symbol: f->GetSymbols()) {
    if (symbol->IsSolved()) {
      continue;
    }
    const auto symName = symbol->GetName().c_str();
    // Currently, we only support coefficients
    const auto symKey = CreateCoefExpr(*dynamic_cast<const symir::Coef *>(symbol));

    // Get the value from the solver
    bitwuzla::Term symValue = solver->get_value(symKey);
    std::string binaryStr = symValue.value<std::string>(2);
    // Convert binary string to signed integer (33-bit)
    int32_t symVal = 0;
    if (!binaryStr.empty()) {
      // For 33-bit values, check the MSB and convert appropriately
      uint64_t unsigned_val = std::stoull(binaryStr, nullptr, 2);
      // Check if the 33rd bit (sign bit) is set
      if (unsigned_val & (1ULL << 32)) {
        // Negative number - take lower 32 bits and reinterpret as signed
        uint32_t u32 = static_cast<uint32_t>(unsigned_val & 0xFFFFFFFF);
        std::memcpy(&symVal, &u32, sizeof(int32_t));
      } else {
        // Positive number - just take lower 32 bits
        uint32_t u32 = static_cast<uint32_t>(unsigned_val & 0xFFFFFFFF);
        std::memcpy(&symVal, &u32, sizeof(int32_t));
      }
    }
    symbol->SetValue(std::to_string(symVal));
    Log::Get().Out() << "Let symbol: sym=" << symName << ", value=" << symVal << std::endl;
  }
}
