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

#include "lib/chksum.hpp"

#include <cassert>
#include <cstdint>
#include <string>

extern "C" int computeStatelessChecksum(int num_args, ...);

int StatelessChecksum::Compute(const std::vector<int> &values) {
  // This is a trick to call into a variadic function.
  // Suppose we can only handle to the maximum of 32 values.
  assert(values.size() <= 32 && "Too many (>32) values provided to compute checksum");
  static int args[32];
  for (int i = 0; i < values.size(); i++) {
    args[i] = values[i];
  }
  // clang-format off
  return computeStatelessChecksum(
      values.size(),
      args[0],  args[1],  args[2],  args[3],  args[4],  args[5],  args[6],  args[7],
      args[8],  args[9],  args[10], args[11], args[12], args[13], args[14], args[15],
      args[16], args[17], args[18], args[19], args[20], args[21], args[22], args[23],
      args[24], args[25], args[26], args[27], args[28], args[29], args[30], args[31]
  );
  // clang-format on
}

std::string StatelessChecksum::GetComputeName() { return "computeStatelessChecksum"; }

std::string StatelessChecksum::GetRawCode() {
#ifndef STATELESS_CHECKSUM_CODE
  static_assert(false, "STATELESS_CHECKSUM_CODE is not defined");
#else
  return STATELESS_CHECKSUM_CODE;
#endif
}
