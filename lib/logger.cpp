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

#include "lib/logger.hpp"

#include <fstream>
#include <iostream>

#include "lib/dbgutils.hpp"

Log::Log() : fout_(new std::ofstream("/dev/null")) { out = fout_; }

Log::~Log() { deleteFoutSafely(); }

Log &Log::Get() {
  static Log logger;
  return logger;
}

std::basic_ostream<char> &Log::Out() const { return *out; }

void Log::SetCout() {
  deleteFoutSafely();
  out = &std::cout;
}

void Log::SetFout(const std::string &file) {
  deleteFoutSafely();
  fout_ = new std::ofstream(file);
  if (!fout_->is_open()) {
    std::cerr << "Failed to open file: " << file << std::endl;
  }
  out = fout_;
}

void Log::OpenSection(const std::string &name) {
  section++;
  Out() << std::string((section + 1) * 2, '-') << name << std::string(50 - section * 2, '-')
        << std::endl
        << std::flush;
}

void Log::CloseSection() {
  section--;
  Assert(section >= 0, "More close sections than open sections");
}

void Log::deleteFoutSafely() {
  if (fout_ != nullptr) {
    fout_->close();
    delete fout_;
    fout_ = nullptr;
  }
}
