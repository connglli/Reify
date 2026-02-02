#  MIT License
#
#  Copyright (c) 2025.
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

import subprocess
import sys
from enum import Enum, unique
from pathlib import Path

from cmdline import run_proc
from configs import (
  FunArts,
  ProgArts,
)


@unique
class UBChkRes(Enum):
  CMP_ERROR = 0
  CMP_HANG = 1
  EXE_ERROR = 3
  EXE_HANG = 4
  UB_FOUND = 5
  UB_FREE = 6


def _ub_exists_in(txt):
  return "runtime error: " in txt


def _do_check_ubs(test_dir, o_file="/tmp/a.out", cmp_tmo=60, exe_tmo=120):
  c_files = [f for f in test_dir.iterdir() if f.suffix == ".c"]
  h_files = [f for f in test_dir.iterdir() if f.suffix == ".h"]
  c_files = [str(f) for f in c_files + h_files]
  try:
    run_proc(
      # TODO: support llubi
      [
        "gcc",
        "-fno-tree-slsr",
        "-fno-ivopts",
        # We now support array and struct, so we'd have to check the memory usages
        "-fsanitize=undefined,address",
        # Perhaps consider continuing the execution after UB is found?
        # "-fsanitize-recover=undefined,address,memory",
        "-o",
        str(o_file),
      ]
      + c_files,
      stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT,
      check=True,
      timeout=cmp_tmo,
    )
  except subprocess.CalledProcessError as e:
    return (
      UBChkRes.CMP_ERROR,
      e.returncode,
      e.stdout or "<no output>",
    )
  except subprocess.TimeoutExpired:
    return UBChkRes.CMP_HANG, 0, f"compilation timeout (>{cmp_tmo}s)"
  try:
    p = run_proc(
      [str(o_file)],
      stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT,
      check=True,
      timeout=exe_tmo,
    )
    out = p.stdout or "<no output>"
    if _ub_exists_in(out):
      return UBChkRes.UB_FOUND, p.returncode, out
    return UBChkRes.UB_FREE, p.returncode, out
  except subprocess.CalledProcessError as e:
    out = e.stdout or "<no output>"
    if _ub_exists_in(out):
      return UBChkRes.UB_FOUND, e.returncode, out
    return UBChkRes.EXE_ERROR, e.returncode, out
  except subprocess.TimeoutExpired:
    return UBChkRes.EXE_HANG, 0, f"execution timeout (>{exe_tmo}s)"


def _check_ubs(test_dir):
  ub_res, retc, out = _do_check_ubs(test_dir)
  if ub_res == UBChkRes.UB_FOUND:
    print("****************************")
    print("********* FOUND UB *********")
    print("****************************")
    print(f"Func : {test_dir.absolute()}")
    print(f"Retc : {retc}")
    print(f"Mesg : {out}")
    exit(1)
  elif ub_res == UBChkRes.CMP_ERROR:
    print(f"CMP ERROR ({retc}): {out}")
  elif ub_res == UBChkRes.CMP_HANG:
    print(f"CMP HANG (.): {out}")
  elif ub_res == UBChkRes.EXE_ERROR:
    print(f"EXE ERROR ({retc}): {out}")
    if "Checksum not equal" in out:
      print(f"Func : {test_dir.absolute()}")
      exit(1)
  elif ub_res == UBChkRes.EXE_HANG:
    print(f"EXE HANG (.): {out}")
  elif ub_res == UBChkRes.UB_FREE:
    pass


def check_func_ubs(arts):
  main_file = arts.get_main_file()
  assert main_file.exists(), f"main.c not found in {arts.get_test_dir()}"
  _check_ubs(arts.get_test_dir())


def check_prog_ubs(arts):
  _check_ubs(arts.get_test_dir())


if __name__ == "__main__":
  if len(sys.argv) != 2:
    print("Usage: ubchk <gen_dir>")
    exit(1)

  gen_dir = Path(sys.argv[1])

  # Allow passing a single generated test directory directly.
  if gen_dir.is_dir() and FunArts.is_test_dir(gen_dir):
    check_func_ubs(FunArts.from_test_dir(gen_dir, gen_dir=gen_dir.parent))
    sys.exit(0)
  if gen_dir.is_dir() and ProgArts.is_test_dir(gen_dir):
    check_prog_ubs(ProgArts.from_test_dir(gen_dir, gen_dir=gen_dir.parent))
    sys.exit(0)

  for test_dir in gen_dir.iterdir():
    if not test_dir.is_dir():
      continue

    print(f"Check {test_dir.name} ...")

    if FunArts.is_test_dir(test_dir):
      check_func_ubs(FunArts.from_test_dir(test_dir, gen_dir=gen_dir))
    elif ProgArts.is_test_dir(test_dir):
      check_prog_ubs(ProgArts.from_test_dir(test_dir, gen_dir=gen_dir))
    else:
      print("Skipping (no main.c or function.c+mapping)")
