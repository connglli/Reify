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

#ifndef REIFY_SAMPUTILS_HPP
#define REIFY_SAMPUTILS_HPP

#include <algorithm>
#include <bitwuzla/cpp/bitwuzla.h>
#include <ranges>
#include <vector>

#include "lib/dbgutils.hpp"
#include "lib/random.hpp"

// Encode "at most k of the input terms equal zero" as a single bit-vector
// constraint. The counter uses the minimum bit-width that can hold the count
// (ceil(log2(N+1))) rather than the input sort's full width: a 32-bit
// adder/ITE chain over N entries bit-blasts to ~32 AIG ANDs per cell, while
// a narrow chain shrinks each cell roughly proportionally to the width.
//
// Per-entry indicator bits are computed as 1-bit BVs (a 1-bit ITE collapses
// to a single multiplexer in AIG) and zero-extended to the counter width
// before being summed with unsigned addition. The final inequality uses
// unsigned compare because the counter is non-negative by construction.
static bitwuzla::Term
AtMostKZeroes(bitwuzla::TermManager &tm, const std::vector<bitwuzla::Term> &vec, int k) {
  Assert(!vec.empty(), "AtMostKZeroes requires at least one input term");
  const size_t N = vec.size();

  // Smallest width that can represent every possible count (0..N).
  unsigned countWidth = 1;
  while ((1ULL << countWidth) <= N) {
    ++countWidth;
  }

  auto bv1Sort = tm.mk_bv_sort(1);
  auto bv1Zero = tm.mk_bv_zero(bv1Sort);
  auto bv1One = tm.mk_bv_one(bv1Sort);
  auto cntSort = tm.mk_bv_sort(countWidth);
  auto cntZero = tm.mk_bv_zero(cntSort);
  auto origZero = tm.mk_bv_zero(vec[0].sort());

  // Sum (expr_i == 0 ? 1 : 0) at the narrow counter width.
  bitwuzla::Term sum = cntZero;
  bool first = true;
  for (const auto &expr: vec) {
    auto isZero = tm.mk_term(bitwuzla::Kind::EQUAL, {expr, origZero});
    auto bit1 = tm.mk_term(bitwuzla::Kind::ITE, {isZero, bv1One, bv1Zero});
    bitwuzla::Term wide;
    if (countWidth == 1) {
      wide = bit1;
    } else {
      wide = tm.mk_term(bitwuzla::Kind::BV_ZERO_EXTEND, {bit1}, {countWidth - 1});
    }
    if (first) {
      sum = wide;
      first = false;
    } else {
      sum = tm.mk_term(bitwuzla::Kind::BV_ADD, {sum, wide});
    }
  }

  // Clamp k into the valid range to keep the BV value well-defined.
  int kClamp = std::max(0, std::min(k, static_cast<int>(N)));
  auto kTerm = tm.mk_bv_value_int64(cntSort, kClamp);
  return tm.mk_term(bitwuzla::Kind::BV_ULE, {sum, kTerm});
}

static std::vector<int> SampleKDistinct(int n, int k) {
  n -= 1;
  Assert(k <= n + 1, "k (=%d) must be at most n + 1 (=%d) to sample k distinct numbers", k, n + 1);
  std::vector<int> numbers(n + 1);
  for (int i = 0; i <= n; ++i) {
    numbers[i] = i;
  }
  std::ranges::shuffle(numbers, Random::Get().GetRNG());
  return std::vector<int>(numbers.begin(), numbers.begin() + k);
}


#endif // REIFY_SAMPUTILS_HPP
