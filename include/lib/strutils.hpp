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

#ifndef GRAPHFUZZ_STRUTILS_HPP
#define GRAPHFUZZ_STRUTILS_HPP

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

static std::vector<std::string>
SplitStr(const std::string &s, const std::string &delim, bool escapeEmpty = false) {
  std::vector<std::string> tokens;
  std::string::size_type curr = 0;
  auto delSize = delim.size();
  do {
    auto next = s.find_first_of(delim, curr);
    auto tk = s.substr(curr, next - curr);
    if (!escapeEmpty || !tk.empty()) {
      tokens.push_back(tk);
    }
    if (next == std::string::npos) {
      break;
    }
    curr = next + delSize;
  } while (true);
  return tokens;
}

static std::string JoinStr(const std::vector<std::string> &tokens, const std::string &delim) {
  if (tokens.empty()) {
    return "";
  }
  if (tokens.size() == 1) {
    return tokens[0];
  }

  std::ostringstream oss;
  for (int i = 0; i < tokens.size() - 1; i++) {
    oss << tokens[i] << delim;
  }
  oss << tokens[tokens.size() - 1];

  return oss.str();
}

static std::string JoinInt(const std::vector<int> &tokens, const std::string &delim) {
  std::vector<std::string> strtokens(tokens.size());
  std::ranges::transform(tokens, strtokens.begin(), [](const int t) { return std::to_string(t); });
  return JoinStr(strtokens, delim);
}

static std::string &StripStr(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](const auto c) {
            return !std::isspace(c);
          }));
  s.erase(
      std::find_if(s.rbegin(), s.rend(), [](const auto c) { return !std::isspace(c); }).base(),
      s.end()
  );
  return s;
}

#endif // GRAPHFUZZ_STRUTILS_HPP
