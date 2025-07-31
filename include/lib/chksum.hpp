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


#ifndef GRAPHFUZZ_CHKSUM_HPP
#define GRAPHFUZZ_CHKSUM_HPP

#include <string>
#include <vector>

class StatelessChecksum {

public:
  // Compute the checksum of the given values
  static int Compute(const std::vector<int> &values);

  // Get the function name for the checksum computing function
  static std::string GetComputeName();

  // Get the raw C code for the  checksum computing function and its dependencies
  static std::string GetRawCode();

  // Get the function name for checking the checksum function
  static std::string GetCheckChksumName();

  // Get the code for check and return the checksum
  static std::string GetCheckChksumCode(bool debug);

private:
  StatelessChecksum() {}
};

class JavaStatelessChecksum {

public:
  // Get the name of the Java class for checksum
  static std::string GetClassName();

  // Get the method name for checking the checksum method
  static std::string GetCheckChksumName();

  // Get the method description (signature) for checking the checksum method
  static std::string GetCheckChksumDesc();

  // Get the method name for the checksum computing method
  static std::string GetComputeName();

  // Get the method description (signature)  for the checksum computing method
  static std::string GetComputeDesc();

private:
  JavaStatelessChecksum() {}
};


#endif // GRAPHFUZZ_CHKSUM_HPP
