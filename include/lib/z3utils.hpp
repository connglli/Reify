#ifndef GRAPHFUZZ_Z3UTILS_HPP
#define GRAPHFUZZ_Z3UTILS_HPP

#include "lib/dbgutils.hpp"
#include "z3++.h"

namespace z3 {
  static inline expr cxx_idiv(const expr &m, const expr &n) {
    Assert(z3::is_int(m), "The dividend m is not an integer expression");
    Assert(z3::is_int(n), "The divisor n is not an integer expression");
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
    return z3::ite(m % n == 0, t, z3::ite(m >= 0, t, z3::ite(n > 0, t + 1, t - 1)));
  }

  static inline expr cxx_irem(const expr &m, const expr &n) {
    // Z3's remainder semantics differ from C/C++, for "m % n":
    // - Z3: (rem m n) == (if (>= n 0) (mod m n) (- (mod m n))
    //   - Refs: https://github.com/Z3Prover/z3/issues/6462#issuecomment-1324842907
    // - C/C++: If m / n is representable in the result type, (m / n) * n + m % n == m.
    //   - Refs: https://en.cppreference.com/w/cpp/language/operator_arithmetic.html
    return m - cxx_idiv(m, n) * n;
  }
} // namespace z3

#endif // GRAPHFUZZ_Z3UTILS_HPP
