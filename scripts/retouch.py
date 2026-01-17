#  MIT License
#
#  Copyright (c) 2025
#
#  Kavya Chopra (chopra.kavya04@gmail.com)
#  Cong Li (cong.li@inf.ethz.ch)
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in all
#  copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#  SOFTWARE.

import shutil
import sys
from pathlib import Path

from configs import (
  FILENAME_FUNCTION_C,
  FILENAME_MAIN_C,
  FILENAME_MAPPING_JSONL,
  FUNCTION_PREFIX,
  PROGRAM_PREFIX,
)

if __name__ == "__main__":
  if len(sys.argv) != 2:
    print("Usage: retouch <gen_dir>")
    exit(1)
  gen_dir = Path(sys.argv[1])

  # Iterate over all test directories
  for test_dir in gen_dir.iterdir():
    if test_dir.is_file():
      continue

    print(f"INSPECT {test_dir.name}")

    if test_dir.name.startswith(FUNCTION_PREFIX + "_"):
      func_path = test_dir / FILENAME_FUNCTION_C
      map_path = test_dir / FILENAME_MAPPING_JSONL

      # Check if we'd delete the generated function (and the whole test dir)
      del_cond = (
        not func_path.exists()
        or func_path.stat().st_size == 0
        or not map_path.exists()
        or map_path.stat().st_size == 0
      )
      if not del_cond:
        continue

      # Delete the whole test directory
      shutil.rmtree(test_dir)
      print(f"DELETE {test_dir.name}")
    elif test_dir.name.startswith(PROGRAM_PREFIX + "_"):
      main_path = test_dir / FILENAME_MAIN_C

      # Check if we'd delete the generated program (and the whole test dir)
      del_cond = not main_path.exists() or main_path.stat().st_size == 0
      if not del_cond:
        continue

      # Delete the whole test directory
      shutil.rmtree(test_dir)
      print(f"DELETE {test_dir.name}")
