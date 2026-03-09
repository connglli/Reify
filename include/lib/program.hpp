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

#ifndef REIFY_PROGRAM_HPP
#define REIFY_PROGRAM_HPP

#include <memory>
#include <string>
#include <vector>
#include "artifacts.hpp"
#include "lib/function.hpp"

namespace fs = std::filesystem;

/// ProgPlus is a generator which fuses a set of functions into an executable program
/// by replacing cofficients in the functions with calls to a specific function.
class ProgPlus {
public:
  ProgPlus(std::string uuid, const int sno, const std::vector<std::string> &funPaths);

  [[nodiscard]] std::string GetName() const { return uuid + "_" + sno; }

  void Generate() const;
  void GenerateCode(const ProgArts &arts) const;

private:
  bool replaceFirstCoef(
      symir::Funct *host, symir::Funct *guest, const std::vector<int> &inits,
      const std::vector<int> &finas
  ) const;

private:
  std::string uuid, sno;
  std::vector<std::unique_ptr<symir::Funct>> functions{};
  std::vector<FunPlus::IniFinMap> mappings{};
};

#endif // REIFY_PROGRAM_HPP
