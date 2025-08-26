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

#ifndef REIFY_JAVAUTILS_HPP
#define REIFY_JAVAUTILS_HPP

#include "jnif/jnif.hpp"

namespace jnif {
  static void PushInteger(ClassFile &c, Method &m, const int v) {
    switch (v) {
      case 0:
        m.instList().addZero(Opcode::iconst_0);
        break;
      case 1:
        m.instList().addZero(Opcode::iconst_1);
        break;
      case 2:
        m.instList().addZero(Opcode::iconst_2);
        break;
      case 3:
        m.instList().addZero(Opcode::iconst_3);
        break;
      case 4:
        m.instList().addZero(Opcode::iconst_4);
        break;
      case 5:
        m.instList().addZero(Opcode::iconst_5);
        break;
      default:
        m.instList().addLdc(Opcode::ldc, c.addInteger(v));
        break;
    }
  }
} // namespace jnif

#endif // REIFY_JAVAUTILS_HPP
