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

#include "lib/varstate.hpp"
#include "lib/symexec.hpp"
#include "lib/ubfree.hpp"
#include <bitwuzla/cpp/bitwuzla.h>
#include <sstream>
#include <utility>

nlohmann::json VariableState::toJson() {
  nlohmann::json initMap = nlohmann::json::object();
  for (auto it = this->init.cbegin(); it != this->init.cend(); it++) {
    initMap[it->first] = nlohmann::json::array();
    for (const auto &arg: it->second) {
      initMap[it->first].push_back(arg);
    }
  }
  nlohmann::json pathArr = nlohmann::json::object();
  for (size_t i = 0; i < this->executionState.size(); i++) {
    pathArr[this->executionState[i].blockId.second] = nlohmann::json::object();
    for (size_t j = 0; j < this->executionState[i].assignments.size(); j++) {
      pathArr[i][this->executionState[i].assignments[j].first.c_str()] = this->executionState[i].assignments[j].second;
    }
  }

  nlohmann::json mapObj = nlohmann::json::object();
  mapObj["init"] = initMap;
  mapObj["path"] = pathArr;
  return mapObj;
}

void VariableState::extract(SymExec *symexec) {
  this->symexec = symexec;
  this->executionState.resize(this->symexec->execution.size());
  for (size_t i = 0; i < this->symexec->execution.size(); i++) {
    this->executionState[i].blockId.first = this->symexec->execution[i];
    this->executionState[i].blockId.second = this->symexec->executionByLabels[i];
  }
  this->symexec->fun->Accept(*this);
}

void VariableState::Visit(const symir::VarUse &v) {
  const auto *varDef = v.GetDef();
  const auto &access = v.GetAccess();

  symir::SymIR::Type currType = varDef->GetType();
  symir::SymIR::Type currBaseType = varDef->GetBaseType();
  std::string currStruct =
      (currType == symir::SymIR::Type::STRUCT)
          ? varDef->GetStructName()
          : (currBaseType == symir::SymIR::Type::STRUCT ? varDef->GetStructName() : "");
  std::vector<int> currShape = varDef->GetVecShape();
  size_t currentShapeIdx = 0; // Index into currShape

  std::ostringstream solverSuffix;
  std::ostringstream stateName;
  stateName << this->currAssignVarDef->GetName();

  // Row-major flattening for the current contiguous array access chain.
  std::vector<int> pendingArrayIndices;
  std::vector<int> pendingArrayShape;

  for (size_t i = 0; i < access.size(); ++i) {
    access[i]->Accept(*this);
    auto idxExpr = popTerm();
    std::optional<int> idxValOpt = symexec->extractTermFromModel(idxExpr);
    Assert(idxValOpt.has_value(), "Onpath index expression is not solved");
    int idxVal = idxValOpt.value();

    if (currentShapeIdx < currShape.size()) {
      // Array Access
      int dimLen = currShape[currentShapeIdx];

      Assert(0 <= idxVal || idxVal < dimLen, "This should have make the solver fail so something is very wrong");
      int elLoc = idxVal;

      pendingArrayShape.push_back(dimLen);
      pendingArrayIndices.push_back(elLoc);
      currentShapeIdx++;

      stateName << '.' << idxVal;
      if (currentShapeIdx == currShape.size()) {
        const int flatLoc = ubsan::FlattenRowMajorIndex(pendingArrayShape, pendingArrayIndices);
        solverSuffix << "_el" << flatLoc;
        pendingArrayShape.clear();
        pendingArrayIndices.clear();

        // Array exhausted. Switch to base type.
        currType = currBaseType;
      }
    } else {
      // Struct Access
      Assert(currType == symir::SymIR::Type::STRUCT, "Excess access coefficients on non-struct");
      const auto *sDef = this->symexec->fun->GetStruct(currStruct);
      int numFields = sDef->GetFields().size();

      Assert(0 <= idxVal || idxVal < numFields, "This should have make the solver fail so something is very wrong");
      int  fIdx = idxVal;

      const auto &field = sDef->GetField(fIdx);
      solverSuffix << "_" << field.name;
      stateName << "." << field.name;

      // Update state for next level
      currType = field.type;
      currBaseType = field.baseType;
      if (currType == symir::SymIR::Type::STRUCT) {
        currStruct = field.structName;
      } else if (currBaseType == symir::SymIR::Type::STRUCT) {
        currStruct = field.structName;
      }
      currShape = field.shape;
      currentShapeIdx = 0;

      pendingArrayShape.clear();
      pendingArrayIndices.clear();
    }
  }
  // handling our own version to avoid messing with the one from UBSan
  std::string solverName = this->currAssignVarDef->GetName() + solverSuffix.str();
  Assert(versions.contains(solverName), "if the variable is used along the path is should have been initialized and hence versioned");
  this->versions[solverName] += 1;

  bitwuzla::Term varUseExpr = 
    this->symexec->ubSan->CreateVersionedExpr(
      this->currAssignVarDef, solverSuffix.str(), this->versions[solverName]
    );
  std::optional<int> varValueOpt = this->symexec->extractTermFromModel(varUseExpr);
  Assert(varValueOpt.has_value(), "VarUse %s is onpath and unsolved!", stateName.str().c_str());

  this->executionState[this->currBlock]
    .assignments
    .push_back(std::make_pair(
      stateName.str(),
      varValueOpt.value()
    ));
}

void VariableState::Visit(const symir::Coef &c) {
  Assert(c.GetType() == symir::SymIR::I32, "Only 32-bit integer variables are supported for now!");
  auto coefExpr = symexec->ubSan->CreateCoefExpr(c);
  pushTerm(coefExpr);
}

void VariableState::Visit(const symir::Term &t) { ((void) t); }
void VariableState::Visit(const symir::Expr &e) { ((void) e); }
void VariableState::Visit(const symir::Cond &c) { ((void) c); }
void VariableState::Visit(const symir::AssStmt &a) {
  // we do not care about the details of the expression just the variable
  // We store the current to be assigned variables name to be used in VarUse;
  this->currAssignVarDef = a.GetVar()->GetDef();
  a.GetVar()->Accept(*this);
}

void VariableState::Visit(const symir::RetStmt &r) { ((void) r); };
void VariableState::Visit(const symir::Branch &b) { ((void) b); }
void VariableState::Visit(const symir::Goto &g) { ((void) g); }
void VariableState::Visit(const symir::ScaParam &p) {
  bitwuzla::Term varExpr = this->symexec->ubSan->CreateScaExpr(&p, 0);
  this->versions[p.GetName()] = 0;
  std::optional<int> varValueOpt = this->symexec->extractTermFromModel(varExpr);
  Assert(varValueOpt.has_value(), "Parameter %s is onpath and unsolved!", p.GetName().c_str());
  this->init[p.GetName()] = std::vector<int>(1, varValueOpt.value());
}

void VariableState::Visit(const symir::VecParam &p) {
  int numEls = p.GetVecNumEls();
  this->init[p.GetName()].resize(numEls);
  for (int i = 0; i < numEls; i++) {
    auto elName = this->symexec->ubSan->GetVecElName(&p, i);
    auto elExpr = this->symexec->ubSan->CreateVecElExpr(&p, i, 0);
    this->versions[elName] = 0;
    std::optional<int> varValueOpt = this->symexec->extractTermFromModel(elExpr);
    Assert(varValueOpt.has_value(), "Parameter %s is onpath and unsolved!", p.GetName().c_str());
    this->init[p.GetName()][i] = varValueOpt.value();
  }
}

void VariableState::Visit(const symir::StructParam &p) {
  const auto *sDef = this->symexec->fun->GetStruct(p.GetStructName());
  const int numInit = sDef->GetFields().size();

  size_t initIdx = 0;
  this->init[p.GetName()].resize(numInit);
  ubsan::IterateStructElements(*this->symexec->fun, sDef, [&](std::string elName) {
    auto fieldExpr = this->symexec->ubSan->CreateStructFieldExpr(&p, elName, 0);
    std::string fullFieldName = this->symexec->ubSan->GetStructFieldName(&p, elName);
    versions[fullFieldName] = 0;
    std::optional<int> varValueOpt = this->symexec->extractTermFromModel(fieldExpr);
    Assert(varValueOpt.has_value(), "Parameter %s is onpath and unsolved!", p.GetName().c_str());
    this->init[p.GetName()][initIdx++] = varValueOpt.value();
  });
}
void VariableState::Visit(const symir::ScaLocal &l) {
  // we do not care for the coeffs term here
  auto varExpr = this->symexec->ubSan->CreateScaExpr(l.GetDefinition(), 0);
  this->versions[l.GetName()] = 0;
  std::optional<int> varValueOpt = this->symexec->extractTermFromModel(varExpr);
  Assert(varValueOpt.has_value(), "Parameter %s is onpath and unsolved!", l.GetName().c_str());
  this->init[l.GetDefinition()->GetName()] = std::vector<int>(1, varValueOpt.value());
}

void VariableState::Visit(const symir::VecLocal &l) {
  int numEls = l.GetVecNumEls();
  this->init[l.GetDefinition()->GetName()].resize(numEls);
  for (int i = 0; i < numEls; i++) {
    auto elName = this->symexec->ubSan->GetVecElName(l.GetDefinition(), i);
    auto elExpr = this->symexec->ubSan->CreateVecElExpr(l.GetDefinition(), i, 0);
    this->versions[elName] = 0;
    std::optional<int> varValueOpt = this->symexec->extractTermFromModel(elExpr);
    Assert(varValueOpt.has_value(), "Parameter %s is onpath and unsolved!", l.GetName().c_str());
    this->init[l.GetDefinition()->GetName()][i] = varValueOpt.value();
  }
}
void VariableState::Visit(const symir::StructLocal &l) {
  const auto *sDef = this->symexec->fun->GetStruct(l.GetStructName());
  const int numInit = sDef->GetFields().size();

  size_t initIdx = 0;
  this->init[l.GetDefinition()->GetName()].resize(numInit);
  ubsan::IterateStructElements(*this->symexec->fun, sDef, [&](std::string elName) {
    auto fieldExpr = this->symexec->ubSan->CreateStructFieldExpr(l.GetDefinition(), elName, 0);
    std::string fullFieldName = this->symexec->ubSan->GetStructFieldName(l.GetDefinition(), elName);
    versions[fullFieldName] = 0;
    std::optional<int> varValueOpt = this->symexec->extractTermFromModel(fieldExpr);
    Assert(varValueOpt.has_value(), "Parameter %s is onpath and unsolved!", l.GetName().c_str());
    this->init[l.GetDefinition()->GetName()][initIdx++] = varValueOpt.value();
  });
}
void VariableState::Visit(const symir::StructDef &s) { ((void) s); };
void VariableState::Visit(const symir::Block &b) {
  for (const auto &stmt: b.GetStmts()) {
    stmt->Accept(*this);
  }
};
void VariableState::Visit(const symir::Funct &f) {
  for (const auto &param: f.GetParams()) {
    param->Accept(*this);
  }
  for (const auto &local: f.GetLocals()) {
    local->Accept(*this);
  }
  for (size_t i = 0; i < this->executionState.size(); i++) {
    this->currBlock = i;
    f.GetBlock(executionState[i].blockId.second)->Accept(*this);
  }
};
