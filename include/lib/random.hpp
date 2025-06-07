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

#ifndef GRAPHFUZZ_RANDOM_HPP
#define GRAPHFUZZ_RANDOM_HPP

#include <functional>
#include <random>

class Random {

public:
  static Random &Get();

public:
  Random(const Random &) = delete;
  Random &operator=(const Random &) = delete;

  [[nodiscard]] auto &GetRNG() { return rng; }

  void Seed(int s);

  template<typename Int = int>
  [[nodiscard]] std::function<Int()> Uniform(Int min = 0, Int max = RAND_MAX) {
    auto dist = std::uniform_int_distribution<Int>(min, max);
    return [dist, this]() mutable -> Int {
      Int x = dist(this->rng);
      return x;
    };
  }

  template<typename Real = double>
  [[nodiscard]] std::function<Real()> UniformReal(Real min = 0., Real max = 1.) {
    auto dist = std::uniform_real_distribution<Real>(min, max);
    return [dist, this]() mutable -> Real {
      Real x = dist(this->rng);
      return x;
    };
  }

private:
  Random() : rng(std::random_device()()) {}

  std::mt19937 rng;
};


#endif // GRAPHFUZZ_RANDOM_HPP
