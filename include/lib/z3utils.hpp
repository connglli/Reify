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

#ifndef REIFY_Z3UTILS_HPP
#define REIFY_Z3UTILS_HPP

#include "lib/dbgutils.hpp"
#include "z3++.h"

namespace z3 {
  static expr cxx_idiv(const expr &m, const expr &n) {
    Assert(m.is_int(), "The dividend m is not an integer expression: %s", m.to_string().c_str());
    Assert(n.is_int(), "The divisor n is not an integer expression: %s", n.to_string().c_str());
    // Z3's division semantics differ from C/C++, for "m / n":
    // + Z3: Regardless of sign of m,
    //   - when n is positive, (div m n) is the floor of the rational number m/n;
    //   - when n is negative, (div m n) is the ceiling of m/n.
    //   - Refs: https://smt-lib.org/theories-Ints.shtml
    // + C/C++: always rounds towards zero.
    //   - Refs: https://en.cppreference.com/w/cpp/language/operator_arithmetic.html
    // Therefore, we need to adjust the division operation based on the sign of n:
    // - m>=0,n>0 => m/n>0, Z3 takes floor(m/n)   => same as C/C++ (towards zero)
    // - m>=0,n<0 => m/n<0, Z3 takes ceiling(m/n) => same as C/C++ (towards zero)
    // - m<=0,n>0 => m/n<0, Z3 takes floor(m/n)   => different => we change to ceiling() or +1
    // - m<=0,n<0 => m/n>0, Z3 takes ceiling(m/n) => different => we change to floor() or -1
    auto t = m / n;
    return ite(m % n == 0, t, ite(m >= 0, t, ite(n > 0, t + 1, t - 1)));
  }

  static expr cxx_irem(const expr &m, const expr &n) {
    // Z3's remainder semantics differ from C/C++, for "m % n":
    // - Z3: (rem m n) == (if (>= n 0) (mod m n) (- (mod m n))
    //   - Refs: https://github.com/Z3Prover/z3/issues/6462#issuecomment-1324842907
    // - C/C++: If m / n is representable in the result type, (m / n) * n + m % n == m.
    //   - Refs: https://en.cppreference.com/w/cpp/language/operator_arithmetic.html
    return m - cxx_idiv(m, n) * n;
  }

  static expr cxx_bvdiv(const expr &m, const expr &n) {
    Assert(m.is_bv(), "The dividend m is not a bitvec expression: %s", m.to_string().c_str());
    Assert(n.is_bv(), "The divisor n is not a bitvec expression: %s", n.to_string().c_str());
    auto t = m / n;
    return ite(m % n == 0, t, ite(m >= 0, t, ite(n > 0, t + 1, t - 1)));
  }

  static expr cxx_bvrem(const expr &m, const expr &n) { return m - cxx_bvdiv(m, n) * n; }
} // namespace z3

#endif // REIFY_Z3UTILS_HPP
