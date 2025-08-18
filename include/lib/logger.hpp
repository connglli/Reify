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

#ifndef REIFY_LOGGER_HPP
#define REIFY_LOGGER_HPP

#include <ostream>
#include <string>

class Log {

public:
  static Log &Get();
  ~Log();

  [[nodiscard]] std::basic_ostream<char> &Out() const;
  void SetCout();
  void SetFout(const std::string &file);

  void OpenSection(const std::string &name);
  void CloseSection();

private:
  Log();

  // CAREFUL: Delete fout_ may lead out to a danger pointer
  void deleteFoutSafely();

private:
  std::basic_ostream<char> *out;
  std::basic_ofstream<char> *fout_;
  int section = 0;
};

#endif // REIFY_LOGGER_HPP
