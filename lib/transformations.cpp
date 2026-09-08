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


#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <string>

#include "lib/lang.hpp"
#include "lib/logger.hpp"
#include "lib/patternmatch.hpp"
#include "lib/random.hpp"
#include "lib/ruleinfo.hpp"
#include "lib/transformations.hpp"

using namespace patternmatch;

void RewriteEngine::AddRule(std::unique_ptr<Rule> rule, int weight) {
  rules.push_back(std::move(rule));
  weights.push_back(weight);
}

void RewriteEngine::Run(
    symir::FunctBuilder *funBd, std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState, size_t times
) const {
  Log::Get().OpenSection("Running RewriteEngine for Blocks in " + funBd->GetName());
  Log::Get().Out() << "Running " << times << " times with " << this->rules.size() << " rules"
                   << std::endl;

  for (size_t t = 0; t < times; t++) {
    for (size_t i = 0; i < 3; i++) {
      size_t stmtIdx, blockIdx;
      {
        size_t nrStmts = 0;
        for (size_t i = 0; i < blockBds.size(); i++) {
          nrStmts += blockBds[i]->GetNumberOfCommitedStmt();
          if (blockBds[i]->HasTarget())
            nrStmts += 1;
        }


        size_t randStmt = Random::Get().Uniform(0, static_cast<int>(nrStmts) - 1)();
        size_t i;
        for (i = 0; i < blockBds.size(); i++) {
          size_t nrStmts = 0;
          nrStmts += blockBds[i]->GetNumberOfCommitedStmt();
          if (blockBds[i]->HasTarget())
            nrStmts += 1;
          if (nrStmts > randStmt)
            break;
          randStmt -= nrStmts;
        }

        blockIdx = i;
        stmtIdx = randStmt;
      }

      const symir::Stmt *stmt = blockBds[blockIdx]->GetCommitedStmtOrTarget(stmtIdx);
      // TODO: Allow Rules/Matching over multiple Stmts
      std::optional rule = this->GetRandomMatchingRule(stmt);
      if (rule.has_value()) {
        RuleInfo::Get().AppendRule(rule.value()->RuleName(), blockIdx, stmtIdx);
        rule.value()->Rewrite(funBd, blockBds, varState, blockIdx, stmtIdx);
        Assert(blockBds.size() > 0, "rewrite deleted all blocks!");
        break;
      }
      // If we failed to find a rule we retry with a new stmt up to 3 times
    }
  }
  Log::Get().CloseSection();
}

void RewriteEngine::RunAsPass(
    symir::FunctBuilder *funBd, std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState, Rule &rule
) const {
  Log::Get().OpenSection("Running RewriteEngine as Pass in " + funBd->GetName());
  size_t nrBlocks = blockBds.size();
  for (int j = nrBlocks - 1; j >= 0; j--) {
    auto blockBd = blockBds[j];
    size_t nrStmt = blockBd->GetNumberOfCommitedStmt();
    if (blockBd->HasTarget())
      nrStmt += 1;
    for (int i = nrStmt - 1; i >= 0; i--) {
      if (rule.Match(blockBd->GetCommitedStmtOrTarget(i))) {
        rule.Rewrite(funBd, blockBds, varState, j, i);
      }
    }
  }
  Log::Get().CloseSection();
}

std::optional<Rule *> RewriteEngine::GetRandomMatchingRule(const symir::Stmt *stmt) const {
  std::vector<size_t> matchingRules;
  int totalWeight = 0;
  for (size_t i = 0; i < this->rules.size(); i++) {
    if (!this->rules[i]->Match(stmt))
      continue;
    matchingRules.push_back(i);
    totalWeight += this->weights[i];
  }

  if (totalWeight == 0)
    return {};
  int selWeight = Random::Get().Uniform(0, totalWeight)();
  int index = 0;
  while (selWeight > this->weights[matchingRules[index]])
    selWeight -= this->weights[matchingRules[index++]];
  return this->rules[matchingRules[index]].get();
}
