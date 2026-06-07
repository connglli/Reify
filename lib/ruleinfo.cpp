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

#include "lib/ruleinfo.hpp"
#include "global.hpp"
#include "json.hpp"

RuleInfo &RuleInfo::Get() {
  static RuleInfo ruleinfo;
  return ruleinfo;
}

void RuleInfo::Seed(int seed) {
  this->seed = seed;
}

void RuleInfo::NewFunction(std::string functionName) {
  if (!GlobalOptions::Get().ruleInfo) return;

  this->functions.push_back(Function{functionName, {}});
}

void RuleInfo::NewBlock(std::string headerblockLabel, int seed, size_t targetRuleCount) {
  if (!GlobalOptions::Get().ruleInfo) return;

  Function &currFunction = *(this->functions.end() - 1);
  currFunction.blocks.push_back(Block{headerblockLabel, seed, targetRuleCount, {}});
};

void RuleInfo::AppendRule(std::string ruleName, size_t blockIndex, size_t stmtIndex) {
  if (!GlobalOptions::Get().ruleInfo) return;

  Function &currFunction = *(this->functions.end() - 1);
  Block &currBlock = *(currFunction.blocks.end() - 1);
  currBlock.rules.push_back(Rule{blockIndex, stmtIndex, ruleName});
};

std::string RuleInfo::ToJson() {
  size_t nrBlocks = 0;
  nlohmann::json ruleInfo = nlohmann::json::object();
  ruleInfo["seed"] = this->seed;
  ruleInfo["numberFunctions"] = this->functions.size();
  ruleInfo["functions"] = nlohmann::json::array();
  for (size_t i = 0; i < this->functions.size(); i++) {
    auto &function = this->functions[i];
    nrBlocks += function.blocks.size();
    nlohmann::json functionObj = nlohmann::json::object();
    functionObj["name"] = function.functionName;
    functionObj["numberBlocks"] = function.blocks.size();
    functionObj["blocks"] = nlohmann::json::array();
    for (size_t j = 0; j < function.blocks.size(); j++) {
      auto &block = function.blocks[j];
      nlohmann::json blockObj = nlohmann::json::object();
      blockObj["headerLabel"] = block.headerBlockLabel;
      blockObj["seed"] = block.blockSeed;
      blockObj["targetRuleCount"] = block.targetRuleCount;
      blockObj["numberRules"] = block.rules.size();
      blockObj["rules"] = nlohmann::json::array();
      for (size_t k = 0; k < block.rules.size(); k++) {
        auto &rule = block.rules[k];
        nlohmann::json rulesObj = nlohmann::json::object();
        rulesObj["blockIndex"] = rule.blockIndex;
        rulesObj["stmtIndex"] = rule.stmtIndex;
        rulesObj["name"] = rule.ruleName;
        blockObj["rules"][k] = rulesObj;
      }
      functionObj["blocks"][j] = blockObj;
    }
    ruleInfo["functions"][i] = functionObj;
    ruleInfo["totalNrBlocks"] = nrBlocks;
  }

  return ruleInfo.dump();
}
