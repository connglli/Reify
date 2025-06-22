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

#ifndef GRAPHFUZZ_UTILS_HPP
#define GRAPHFUZZ_UTILS_HPP

#include <algorithm>
#include <vector>
#include <z3++.h>

#include "lib/random.hpp"

static z3::expr AtMostKZeroes(z3::context &ctx, const std::vector<z3::expr> &vec, int k) {
  z3::expr_vector zero_constraints(ctx);
  for (const auto &expr: vec) {
    zero_constraints.push_back(z3::ite(expr == 0, ctx.int_val(1), ctx.int_val(0)));
  }
  z3::expr sum_zeros = sum(zero_constraints);
  return sum_zeros <= k;
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


#endif // GRAPHFUZZ_UTILS_HPP
