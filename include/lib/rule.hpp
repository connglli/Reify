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

#ifndef REIFY_RULE_HPP
#define REIFY_RULE_HPP

#include "lib/lang.hpp"
#include <string>

struct Rule {
  Rule(std::string locPrefix = "local_created_by_rule_without_proper_locPrefix") : locPrefix(locPrefix) {}
  virtual ~Rule() = default;
  virtual bool match(const symir::Stmt *stmt) const = 0;
  virtual std::vector<symir::BlockBuilder::StmtID> rewrite(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::Stmt *stmt
  ) = 0;

  /// get a new scalar local that is not yet used in the block with label `blockLabel`
  const symir::VarDef *getNewScaLocal(symir::FunctBuilder *funBd, std::string blockLabel, symir::SymIR::Type type = symir::SymIR::Type::I32) {
    if (!this->varCounterMap.contains(blockLabel)) this->varCounterMap[blockLabel] = 0;
    std::string locName = this->locPrefix + "_" + std::to_string(this->varCounterMap[blockLabel]++);
    Assert(funBd->FindParam(locName) == nullptr, "New Local %s should never be a Parameter", locName.c_str());
    const symir::VarDef *loc = funBd->FindLocal(locName);
    if (loc == nullptr) loc = funBd->SymScaLocal(locName, nullptr);
    return loc;
  }

  /// get a new vector local that is not yet used in the block with label `blockLabel`
  const symir::VarDef *getNewVecLocal(
      symir::FunctBuilder *funBd,
      std::string blockLabel, 
      const std::vector<int> &shape,
      symir::SymIR::Type type = symir::SymIR::Type::I32,
      std::string structName = ""
    ) {
    if (!this->varCounterMap.contains(blockLabel)) this->varCounterMap[blockLabel] = 0;
    std::string locName = this->locPrefix + "_" + std::to_string(this->varCounterMap[blockLabel]++);
    for (int i : shape) {
      locName += "_" + std::to_string(i);
    }
    Assert(funBd->FindParam(locName) == nullptr, "New Local %s should never be a Parameter", locName.c_str());
    const symir::VarDef *loc = funBd->FindLocal(locName);
    if (loc == nullptr) loc = funBd->SymVecLocal(locName, shape, {}, type, structName);
    return loc;
  }

protected:
  std::string locPrefix;
  std::map<std::string, size_t> varCounterMap;
};

#endif // REIFY_RULE_HPP
