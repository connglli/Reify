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

import sys
from pathlib import Path

from configs import get_map_file_for_func_file, get_funcs_dir

if __name__ == "__main__":
  if len(sys.argv) != 2:
    print("Usage: retouch <gen_dir>")
    exit(1)
  # Iterate over all .c files in the procedures directory
  for func_path in get_funcs_dir(Path(sys.argv[1])).iterdir():
    if func_path.is_dir() or func_path.suffix != ".c":
      continue
    print(f"INSPECT {func_path.stem}")
    map_path = get_map_file_for_func_file(func_path)
    # Check if we'd delete the generated function
    del_cond = (
      # The .c is empty
      func_path.stat().st_size == 0
      or
      # The .map does not exist
      not map_path.exists()
      or
      # The .map is empty
      map_path.stat().st_size == 0
    )
    if not del_cond:
      continue
    # Delete the corresponding mapping file if it exists
    if map_path.exists():
      map_path.unlink()
    func_path.unlink()
    print(f"DELETE {func_path.stem}")
