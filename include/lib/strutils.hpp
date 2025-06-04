//
// Created by congli on 6/4/25.
//

#ifndef GRAPHFUZZ_STRUTILS_HPP
#define GRAPHFUZZ_STRUTILS_HPP

#include <sstream>
#include <string>
#include <vector>

static std::vector<std::string> SplitStr(const std::string &s, const std::string &delim, bool escapeEmpty = false) {
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

#endif // GRAPHFUZZ_STRUTILS_HPP
