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

#include <sstream>
#include <string>

#include "lib/dbgutils.hpp"

extern "C" int computeStatelessChecksum(int num_args, int args[]);

int StatelessChecksum::Compute(const std::vector<int> &values) {
  // This is a trick to call into a variadic function.
  // Suppose we can only handle to the maximum of 128 values.
  Assert(values.size() <= 128, "Too many (>128) values provided to compute checksum");
  static int args[128];
  for (int i = 0; i < values.size(); i++) {
    args[i] = values[i];
  }
  return computeStatelessChecksum(static_cast<int>(values.size()), args);
}

std::string StatelessChecksum::GetComputeName() { return "computeStatelessChecksum"; }

std::string StatelessChecksum::GetRawCode() {
#ifndef STATELESS_CHECKSUM_CODE
  static_assert(false, "STATELESS_CHECKSUM_CODE is not defined");
#else
  return STATELESS_CHECKSUM_CODE;
#endif
}

std::string StatelessChecksum::GetCheckChksumName() { return "check_chksum"; }

std::string StatelessChecksum::GetCheckChksumCode(bool debug) {
  std::ostringstream oss;
  if (debug) {
    oss << "#include <assert.h>" << std::endl << std::endl;
    oss << "#define " << GetCheckChksumName()
        << "(expected, actual) (assert((expected)==(actual) && \"Checksum "
           "not equal\"), (actual))"
        << std::endl;
  } else {
    oss << "#define " << GetCheckChksumName() << "(expected, actual) (actual)" << std::endl;
  }
  return oss.str();
}

std::string JavaStatelessChecksum::GetClassName() { return "JChecksum"; }

std::string JavaStatelessChecksum::GetCheckChksumName() { return "check"; }

std::string JavaStatelessChecksum::GetCheckChksumDesc() { return "(III)V"; }

std::string JavaStatelessChecksum::GetComputeName() { return "computeStatelessChecksum"; }

std::string JavaStatelessChecksum::GetComputeDesc() { return "([I)I"; }
