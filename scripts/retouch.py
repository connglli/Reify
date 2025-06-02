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

from params import PROCEDURES_DIR, get_map_file_for_func_file

# Iterate over all .c files in the procedures directory
for func_path in PROCEDURES_DIR.iterdir():
    if func_path.is_dir() or func_path.suffix != '.c':
        continue
    # Check if the .c file is empty
    if func_path.stat().st_size != 0:
        continue
    map_path = get_map_file_for_func_file(func_path)
    # Delete the corresponding mapping file if it exists
    if not map_path.exists():
        map_path.unlink()
    if func_path.exists():
        func_path.unlink()
    print(f"DEL {func_path.stem}")
