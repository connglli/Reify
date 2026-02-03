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

static bitwuzla::Term
AtMostKZeroes(bitwuzla::TermManager &tm, const std::vector<bitwuzla::Term> &vec, int k) {
  auto bvSort = vec[0].sort();
  auto zero = tm.mk_bv_zero(bvSort);
  auto one = tm.mk_bv_one(bvSort);

  std::vector<bitwuzla::Term> zero_constraints;
  for (const auto &expr: vec) {
    // If expr == 0, add 1, else add 0
    auto is_zero = tm.mk_term(bitwuzla::Kind::EQUAL, {expr, zero});
    zero_constraints.push_back(tm.mk_term(bitwuzla::Kind::ITE, {is_zero, one, zero}));
  }

  // Sum all the constraints
  bitwuzla::Term sum_zeros = zero_constraints[0];
  for (size_t i = 1; i < zero_constraints.size(); i++) {
    sum_zeros = tm.mk_term(bitwuzla::Kind::BV_ADD, {sum_zeros, zero_constraints[i]});
  }

  // sum_zeros <= k
  auto k_val = tm.mk_bv_value_int64(bvSort, k);
  return tm.mk_term(bitwuzla::Kind::BV_SLE, {sum_zeros, k_val});
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
