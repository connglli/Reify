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

#include "lib/reduceinfo.hpp"
#include "lib/dbgutils.hpp"
#include <fstream>
#include <iostream>
#include <utility>
#include "json.hpp"


ReduceInfo &ReduceInfo::Get() {
  static ReduceInfo ruleinfo;
  return ruleinfo;
}

size_t ReduceInfo::GetRuleCount(std::string functionName, std::string headBlockLabel) {
  return this->ruleCountMap[std::make_pair(functionName, headBlockLabel)];
}

void ReduceInfo::FromJson(std::string path) {
    std::ifstream filestream(path);
    Assert(filestream.is_open(), "Error: failed to open file: %s", path.c_str());

    std::string line;
    // currently this is a single line json;
    std::getline(filestream, line);
    nlohmann::json reduceInfo = nlohmann::json::parse(line);
    this->sno = reduceInfo["sno"];
    for (size_t i = 0; i < reduceInfo["functions"].size(); i++) {
      nlohmann::json functionObj = reduceInfo["functions"][i];
      for (size_t j = 0; j < functionObj["blocks"].size(); j++) {
        nlohmann::json blockObj = functionObj["blocks"][j];
        this->ruleCountMap[std::make_pair(functionObj["name"], blockObj["headerLabel"])] = blockObj["targetRuleCount"];
      }
    }
}
