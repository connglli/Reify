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

#ifndef GRAPHFUZZ_NAMING_HPP
#define GRAPHFUZZ_NAMING_HPP

#include <string>

static std::string NamePassCounter() { return "pass_counter"; }

static std::string NameLabel(int block) { return "BB" + std::to_string(block); }

static std::string NameVar(int index) { return "var_" + std::to_string(index); }

static std::string NameCoeff(int block, int stmt, int which) {
  return "a_" + std::to_string(block) + "_" + std::to_string(stmt) + "_" + std::to_string(which);
}

static std::string NameConst(int block, int stmt) { return "a_" + std::to_string(block) + "_" + std::to_string(stmt); }

static std::string NameCondCoeff(int block, int stmt, int which) {
  return "b_" + std::to_string(block) + "_" + std::to_string(stmt) + "_" + std::to_string(which);
}

static std::string NameCondConst(int block, int stmt) {
  return "b_" + std::to_string(block) + "_" + std::to_string(stmt);
}


#endif // GRAPHFUZZ_NAMING_HPP
