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

#include <stdint.h>

static uint32_t crc32_tab[256];

void generate_crc32_table() {
  const uint32_t poly = 0xEDB88320UL;
  for (uint32_t i = 0; i < 256; i++) {
    uint32_t crc = i;
    for (int j = 8; j > 0; j--) {
      if (crc & 1) {
        crc = (crc >> 1) ^ poly;
      } else {
        crc >>= 1;
      }
    }
    crc32_tab[i] = crc;
  }
}

static int32_t uint32_to_int32(uint32_t u) { return u % 2147483647; }

static uint32_t context_free_crc32_byte(uint32_t context, uint8_t b) {
  return ((context >> 8) & 0x00FFFFFF) ^ crc32_tab[(context ^ b) & 0xFF];
}

static uint32_t context_free_crc32_4bytes(uint32_t context, uint32_t val) {
  context = context_free_crc32_byte(context, (val >> 0) & 0xFF);
  context = context_free_crc32_byte(context, (val >> 8) & 0xFF);
  context = context_free_crc32_byte(context, (val >> 16) & 0xFF);
  context = context_free_crc32_byte(context, (val >> 24) & 0xFF);
  return context;
}

int computeStatelessChecksum(int num_args, int args[]) {
  uint32_t checksum = 0xFFFFFFFFUL;
  int i = 0;
  for (; i < num_args; ++i) {
    checksum = context_free_crc32_4bytes(checksum, args[i]);
  }
  checksum = uint32_to_int32(checksum ^ 0xFFFFFFFFUL);
  return checksum;
}

#include <assert.h>
#include <stdio.h>

static inline int check_chksum(int expected, int actual)
{
  if (expected != actual) {
    fprintf(stderr, "Checksum not equal: expected=%d actual=%d\n", expected, actual);
  }
  assert(expected == actual && "Checksum not equal");
  return actual;
}

int func_0g6z1t_61(int v0, int v1, int v2, int v3, int v4, int v5);

int main(int argc, char* argv[])
{
  generate_crc32_table();
  check_chksum(137549320, func_0g6z1t_61(2, 268435397, -11538400, -2147483647, -1, -1));
  check_chksum(577955629, func_0g6z1t_61(-1191114, 485490548, -11542068, -1770520674, -2, -1));
  check_chksum(328666023, func_0g6z1t_61(-1191113, 485490549, -11542068, -1770520674, -2, -2));
  return 0;
}

