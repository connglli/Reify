/*
 * MIT License
 *
 * Copyright (c) 2025 Cong Li (cong.li@inf.ethz.ch)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

import java.util.zip.CRC32;

public class JChecksum {

  public static class ChecksumNotEqualsException extends RuntimeException {
    public ChecksumNotEqualsException(int expected, int actual) {
      super("Checksum not equals, expected " + expected + ", actual " + actual + ". ");
    }
  }

  public static int long_to_int(long u) {
    return (int) (u % 2147483647);
  }

  public static int computeStatelessChecksum(int[] args) {
    CRC32 crc32 = new CRC32();
    for (int i = 0; i < args.length; ++i) {
      byte[] bytes = new byte[4];
      bytes[0] = (byte) ((args[i] >> 0) & 0xFF);
      bytes[1] = (byte) ((args[i] >> 8) & 0xFF);
      bytes[2] = (byte) ((args[i] >> 16) & 0xFF);
      bytes[3] = (byte) ((args[i] >> 24) & 0xFF);
      crc32.update(bytes, 0, 4);
    }
    return long_to_int(crc32.getValue());
  }

  public static void check(int expected, int actual, int debug) {
    if (debug == 1 && (expected != actual)) {
      throw new ChecksumNotEqualsException(expected, actual);
    }
  }
}
