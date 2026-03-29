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
#include "lib/lang.hpp"
#include "lib/symexec.hpp"
#include "lib/ubfree.hpp"
#include <bitwuzla/cpp/bitwuzla.h>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

namespace varstate {
  std::vector<VariableState> allFromJsonFile(std::string filepath) {
    std::ifstream filestream(filepath);
    Assert(filestream.is_open(), "Error: failed to open file: %s", filepath.c_str());

    std::string line;
    // currently this is a single line json;
    std::getline(filestream, line);
    nlohmann::json mapObj = nlohmann::json::parse(line);

    std::vector<VariableState> res;
    res.resize(mapObj.size());
    size_t i = 0;
    for (const auto& [k, varStateJson] : mapObj.items()) {
      res[i++].fromJson(varStateJson);
    }
    return res;
  }
  void print_state(size_t nr_variables, std::vector<int32_t> states) {
    for (size_t i = 0; i < states.size(); i++) {
      std::cout << states[i];
      if (i != 0 && i % nr_variables == 0) std::cout << std::endl;
      else std::cout << ", ";
    }
  }
}


nlohmann::json VariableState::toJson() {
  nlohmann::json initArr = nlohmann::json::array();
  for (size_t i = 0; i < this->init.size(); i++) {
    initArr[i] = this->init[i];
  }
  nlohmann::json varMap = nlohmann::json::object();
  for (const auto& [idx, name] : this->varNamesMap) {
    varMap[name] = idx;
  }
  nlohmann::json pathArr = nlohmann::json::array();
  for (size_t i = 0; i < this->executionState.size(); i++) {
    pathArr[i] = nlohmann::json::object();
    pathArr[i]["block"] = this->executionState[i].blockId.first;
    pathArr[i]["blockLabel"] = this->executionState[i].blockId.second;
    nlohmann::json assignArr = nlohmann::json::array();
    for (size_t j = 0; j < this->executionState[i].assignments.size(); j++) {
      assignArr[j] = nlohmann::json::object();
      assignArr[j]["variable"] = this->executionState[i].assignments[j].first;
      assignArr[j]["value"] = this->executionState[i].assignments[j].second;
    }
    pathArr[i]["assignments"] = assignArr;
  }

  nlohmann::json mapObj = nlohmann::json::object();
  mapObj["init"] = initArr;
  mapObj["varnames"] = varMap;
  mapObj["path"] = pathArr;
  return mapObj;
}

void VariableState::fromJson(nlohmann::json mapObj) {
  this->symexec = nullptr;

  this->init.clear();
  this->varNamesMap.clear();
  this->executionState.clear();

  Assert(mapObj.contains("init"), "Varstate object does not contain an field 'init'");
  Assert(mapObj.contains("varnames"), "varstate object does not contain an field 'varname'");
  Assert(mapObj.contains("path"), "varstate object does not contain an field 'path'");
  nlohmann::json initArr = mapObj["init"];
  nlohmann::json varMap = mapObj["varnames"];
  nlohmann::json pathArr = mapObj["path"];
  Assert(
      initArr.is_array(), "The field 'init' is not an array but a %s",
      initArr.type_name()
  );
  Assert(
      varMap.is_object(), "The field 'varnames' is not an object but a %s",
      varMap.type_name()
  );
  Assert(
      pathArr.is_array(), "The field 'path' is not an array but a %s",
      pathArr.type_name()
  );

  // read init state of the variables
  this->init.resize(initArr.size());
  for (size_t i = 0; i < initArr.size(); i++) this->init[i] = initArr[i];

  // read variable indexes
  for (const auto& [name, idx] : varMap.items()) {
    this->varNamesMap[idx.get<int32_t>()] = name;
  }

  // read path states
  this->executionState.resize(pathArr.size());
  for (size_t i = 0; i < pathArr.size(); i++) {
    Assert(pathArr[i].contains("block"), "varstate object does not contain an field 'block'");
    Assert(pathArr[i].contains("blockLabel"), "varstate object does not contain an field 'blockLabel'");
    Assert(pathArr[i].contains("assignments"), "varstate object does not contain an field 'blockLabel'");
    this->executionState[i].blockId.first = pathArr[i]["block"];
    this->executionState[i].blockId.second= pathArr[i]["blockLabel"];

    nlohmann::json assignArr = pathArr[i]["assignments"];
    this->executionState[i].assignments.resize(assignArr.size());
    for (size_t j = 0; j < assignArr.size(); j++) {
      this->executionState[i].assignments[j].first = assignArr[j]["variable"];
      this->executionState[i].assignments[j].second = assignArr[j]["value"].get<int32_t>();
    }
  }
}

std::pair<size_t, std::vector<int32_t>> VariableState::query(size_t blockIndex, size_t stmtIndex) {
  std::vector<int32_t> varState = std::vector(this->init);

  size_t varsCount = varState.size();
  size_t currStateStartIdx = 0;
  for (const auto &blockState : this->executionState) {
    for (size_t assignIdx = 0; assignIdx < blockState.assignments.size(); assignIdx++) {
      // if we reached the target block and stmt save the current state at this row and move on to the next row
      if (blockIndex == (size_t) blockState.blockId.first && assignIdx == stmtIndex) {
        varState.resize(varState.size() + varsCount);
        for (size_t i = varState.size() - varsCount; i < varState.size() ; i++)
          varState[i] = varState[i - varsCount];
        currStateStartIdx += varsCount;
      }
      // update assigned variables state
      varState[blockState.assignments[assignIdx].first + currStateStartIdx] =
        blockState.assignments[assignIdx].second;
    }
    // if we reached the target block and stmt is after the last assignment stmt store anyway
    if (blockIndex == (size_t) blockState.blockId.first && blockState.assignments.size() <= stmtIndex) {
      varState.resize(varState.size() + varsCount);
      for (size_t i = varState.size() - varsCount; i < varState.size() ; i++)
        varState[i] = varState[i - varsCount];
      currStateStartIdx += varsCount;
    }
  }
  // cut the temporary top variable state before returning
  varState.resize(varState.size() - varsCount);
  Assert(varState.size() != 0, "unable to find variable state at block %ld, stmt %ld", blockIndex, stmtIndex);
  return std::make_pair(varsCount, varState);
}

void VariableState::extract(SymExec *symexec) {
  this->symexec = symexec;
  this->executionState.resize(symexec->execution.size());
  for (size_t i = 0; i < symexec->execution.size(); i++) {
    this->executionState[i].blockId.first = symexec->execution[i];
    this->executionState[i].blockId.second = symexec->executionByLabels[i];
  }
  symexec->fun->Accept(*this);
}

std::vector<int32_t> VariableState::getPathBlocksIndices() {
  std::vector<int32_t> path;
  path.reserve(this->executionState.size());
  for (const auto& es : executionState) {
    path.push_back(es.blockId.first);
  }
  return path;
}

std::vector<std::string> VariableState::getPathBlocksLabels() {
  std::vector<std::string> path;
  path.reserve(this->executionState.size());
  for (const auto& es : executionState) {
    path.push_back(es.blockId.second);
  }
  return path;
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
  std::vector<int32_t> currShape = varDef->GetVecShape();
  size_t currentShapeIdx = 0; // Index int32_to currShape

  std::ostringstream solverSuffix;
  int32_t flattenedIdx = 0;

  // Row-major flattening for the current contiguous array access chain.
  std::vector<int32_t> pendingArrayIndices;
  std::vector<int32_t> pendingArrayShape;

  for (size_t i = 0; i < access.size(); ++i) {
    access[i]->Accept(*this);
    auto idxExpr = popTerm();
    std::optional<int32_t> idxValOpt = symexec->extractTermFromModel(idxExpr);
    Assert(idxValOpt.has_value(), "Onpath index expression is not solved");
    int32_t idxVal = idxValOpt.value();

    if (currentShapeIdx < currShape.size()) {
      // Array Access
      int32_t dimLen = currShape[currentShapeIdx];

      Assert(0 <= idxVal || idxVal < dimLen, "This should have make the solver fail so something is very wrong");
      int32_t elLoc = idxVal;

      pendingArrayShape.push_back(dimLen);
      pendingArrayIndices.push_back(elLoc);
      currentShapeIdx++;

      if (currentShapeIdx == currShape.size()) {
        const int32_t flatLoc = ubsan::FlattenRowMajorIndex(pendingArrayShape, pendingArrayIndices);
        solverSuffix << "_el" << flatLoc;
        flattenedIdx += flatLoc;
        pendingArrayShape.clear();
        pendingArrayIndices.clear();

        // Array exhausted. Switch to base type.
        currType = currBaseType;
      }
    } else {
      // Struct Access
      Assert(currType == symir::SymIR::Type::STRUCT, "Excess access coefficients on non-struct");
      const auto *sDef = this->symexec->fun->GetStruct(currStruct);
      int32_t numFields = sDef->GetFields().size();

      Assert(0 <= idxVal || idxVal < numFields, "This should have make the solver fail so something is very wrong");
      int32_t  fIdx = idxVal;

      const auto &field = sDef->GetField(fIdx);
      solverSuffix << "_" << field.name;
      flattenedIdx += fIdx;

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
  std::optional<int32_t> varValueOpt = this->symexec->extractTermFromModel(varUseExpr);
  Assert(varValueOpt.has_value(), "VarUse %s is onpath and unsolved!", this->currAssignVarDef->GetName().c_str());

  // look for the variable index to get the correct index into init;
  size_t varIndex;
  bool foundIndex = false;
  for (const auto& [index, name] : this->varNamesMap) {
    if (name == this->currAssignVarDef->GetName()) {
      varIndex = index;
      foundIndex = true;
    }
  }
  Assert(foundIndex, "unable to find var %s in params / locals", this->currAssignVarDef->GetName().c_str());


  this->executionState[this->currBlock]
    .assignments
    .push_back(std::make_pair(
      varIndex + flattenedIdx,
      varValueOpt.value()
    ));
}

void VariableState::Visit(const symir::Coef &c) {
  Assert(c.GetType() == symir::SymIR::I32, "Only 32-bit int32_teger variables are supported for now!");
  auto coefExpr = symexec->ubSan->CreateCoefExpr(c);
  pushTerm(coefExpr);
}

void VariableState::Visit(const symir::Term &t) { /* Do Nothing */; }
void VariableState::Visit(const symir::ModExpr &e) {
  Panic("No ModExpr should exist during function creation");
}
void VariableState::Visit(const symir::Expr &e) { /* Do Nothing */; }
void VariableState::Visit(const symir::Cond &c) { /* Do Nothing */; }
void VariableState::Visit(const symir::ModAssStmt &a) {
  Panic("No ModAssStmt should exist during function creation");
}
void VariableState::Visit(const symir::AssStmt &a) {
  this->currAssignVarDef = a.GetVar()->GetDef();
  a.GetVar()->Accept(*this);
}

void VariableState::Visit(const symir::RetStmt &r) { /* Do Nothing */; };
void VariableState::Visit(const symir::Branch &b) { /* Do Nothing */; }
void VariableState::Visit(const symir::Goto &g) { /* Do Nothing */; }
void VariableState::Visit(const symir::ScaParam &p) {
  bitwuzla::Term varExpr = this->symexec->ubSan->CreateScaExpr(&p, 0);
  this->versions[p.GetName()] = 0;
  std::optional<int32_t> varValueOpt = this->symexec->extractTermFromModel(varExpr);
  Assert(varValueOpt.has_value(), "Parameter %s is onpath and unsolved!", p.GetName().c_str());
  this->init.push_back(varValueOpt.value());
}

void VariableState::Visit(const symir::VecParam &p) {
  int32_t numEls = p.GetVecNumEls();
  for (int32_t i = 0; i < numEls; i++) {
    auto elName = this->symexec->ubSan->GetVecElName(&p, i);
    auto elExpr = this->symexec->ubSan->CreateVecElExpr(&p, i, 0);
    this->versions[elName] = 0;
    std::optional<int32_t> varValueOpt = this->symexec->extractTermFromModel(elExpr);
    Assert(varValueOpt.has_value(), "Parameter %s is onpath and unsolved!", p.GetName().c_str());
    this->init.push_back(varValueOpt.value());
  }
}

void VariableState::Visit(const symir::StructParam &p) {
  const auto *sDef = this->symexec->fun->GetStruct(p.GetStructName());
  ubsan::IterateStructElements(*this->symexec->fun, sDef, [&](std::string elName) {
    auto fieldExpr = this->symexec->ubSan->CreateStructFieldExpr(&p, elName, 0);
    std::string fullFieldName = this->symexec->ubSan->GetStructFieldName(&p, elName);
    versions[fullFieldName] = 0;
    std::optional<int32_t> varValueOpt = this->symexec->extractTermFromModel(fieldExpr);
    Assert(varValueOpt.has_value(), "Parameter %s is onpath and unsolved!", p.GetName().c_str());
    this->init.push_back(varValueOpt.value());
  });
}

void VariableState::Visit(const symir::UnInitLocal &l) {
  Panic("No ModExpr should exist during function creation");
}

void VariableState::Visit(const symir::ScaLocal &l) {
  // we do not care for the coeffs term here
  auto varExpr = this->symexec->ubSan->CreateScaExpr(l.GetDefinition(), 0);
  this->versions[l.GetName()] = 0;
  std::optional<int32_t> varValueOpt = this->symexec->extractTermFromModel(varExpr);
  Assert(varValueOpt.has_value(), "Parameter %s is onpath and unsolved!", l.GetName().c_str());
  this->init.push_back(varValueOpt.value());
}

void VariableState::Visit(const symir::VecLocal &l) {
  int32_t numEls = l.GetVecNumEls();
  for (int32_t i = 0; i < numEls; i++) {
    auto elName = this->symexec->ubSan->GetVecElName(l.GetDefinition(), i);
    auto elExpr = this->symexec->ubSan->CreateVecElExpr(l.GetDefinition(), i, 0);
    this->versions[elName] = 0;
    std::optional<int32_t> varValueOpt = this->symexec->extractTermFromModel(elExpr);
    Assert(varValueOpt.has_value(), "Parameter %s is onpath and unsolved!", l.GetName().c_str());
    this->init.push_back(varValueOpt.value());
  }
}
void VariableState::Visit(const symir::StructLocal &l) {
  const auto *sDef = this->symexec->fun->GetStruct(l.GetStructName());
  ubsan::IterateStructElements(*this->symexec->fun, sDef, [&](std::string elName) {
    auto fieldExpr = this->symexec->ubSan->CreateStructFieldExpr(l.GetDefinition(), elName, 0);
    std::string fullFieldName = this->symexec->ubSan->GetStructFieldName(l.GetDefinition(), elName);
    versions[fullFieldName] = 0;
    std::optional<int32_t> varValueOpt = this->symexec->extractTermFromModel(fieldExpr);
    Assert(varValueOpt.has_value(), "Parameter %s is onpath and unsolved!", l.GetName().c_str());
    this->init.push_back(varValueOpt.value());
  });
}
void VariableState::Visit(const symir::StructDef &s) { /* Do Nothing */; };
void VariableState::Visit(const symir::Block &b) {
  for (const auto &stmt: b.GetStmts()) {
    stmt->Accept(*this);
  }
};
void VariableState::Visit(const symir::Funct &f) {
  for (const auto &param: f.GetParams()) {
    this->varNamesMap[this->init.size()] = param->GetName();
    param->Accept(*this);
  }
  for (const auto &local: f.GetLocals()) {
    this->varNamesMap[this->init.size()] = local->GetName();
    local->Accept(*this);
  }
  for (size_t i = 0; i < this->executionState.size(); i++) {
    this->currBlock = i;
    f.GetBlock(executionState[i].blockId.second)->Accept(*this);
  }
};
