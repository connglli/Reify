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

#ifndef REIFY_RULEINFO_HPP
#define REIFY_RULEINFO_HPP

#include <string>
#include <vector>

class RuleInfo {

public:
  static RuleInfo &Get();

public:
  void GlobalSeed(int seed);
  void ProgSeed(int seed);
  void Clear();
  void Sno(int sno);
  void NewFunction(std::string functionName);
  void NewBlock(std::string headerBlockLabel, int seed, size_t targetRuleCount);
  void AppendRule(std::string ruleName, size_t blockIndex, size_t stmtIndex);
  std::string ToJson();

private:
  struct Rule {
    size_t blockIndex;
    size_t stmtIndex;
    std::string ruleName;
  };

  struct Block {
    std::string headerBlockLabel;
    int blockSeed;
    size_t targetRuleCount;
    std::vector<Rule> rules;
  };

  struct Function {
    std::string functionName;
    std::vector<Block> blocks;
  };

private:
  RuleInfo() : functions({}) {}

private:
  int globalSeed;
  int progSeed;
  int sno;
  std::vector<Function> functions;
};

#endif // REIFY_RULEINFO_HPP
