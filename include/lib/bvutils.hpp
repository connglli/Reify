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

#ifndef REIFY_BVUTILS_HPP
#define REIFY_BVUTILS_HPP

#include <bitwuzla/cpp/bitwuzla.h>
#include "dbgutils.hpp"

namespace bvutils {
  using namespace bitwuzla;

  // Helper function to create C-style signed division.
  // Note: Bitwuzla's BV_SDIV follows SMT-LIB semantics (truncate toward 0),
  // which matches C/C++ for signed division (ignoring UB cases like INT_MIN / -1,
  // which are handled separately via BV_SDIV_OVERFLOW guards).
  static Term cxx_bvsdiv(TermManager &tm, const Term &m, const Term &n) {
    Assert(m.sort().is_bv(), "The dividend m is not a bitvector expression");
    Assert(n.sort().is_bv(), "The divisor n is not a bitvector expression");

    return tm.mk_term(Kind::BV_SDIV, {m, n});
  }

  // Helper function to create C-style signed remainder
  static Term cxx_bvsrem(TermManager &tm, const Term &m, const Term &n) {
    Assert(m.sort().is_bv(), "The dividend m is not a bitvector expression");
    Assert(n.sort().is_bv(), "The divisor n is not a bitvector expression");

    // SMT-LIB BV_SREM matches C/C++ remainder with trunc-toward-zero division.
    return tm.mk_term(Kind::BV_SREM, {m, n});
  }
} // namespace bvutils

#endif // REIFY_BVUTILS_HPP
