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

import os
import random
import time
from argparse import ArgumentParser
from pathlib import Path

from configs import (
  DEFAULT_OUTPUT_DIR,
  FunArts,
)
from fuzz import FGEN_SUGGESTED_CONFIGS, FuncGenOptions, generate_function, next_uuid
from ubchk import check_func_ubs


def generate(opts: FuncGenOptions, *, timeout: int):
  st_time = time.time()
  arts, errmsg = generate_function(opts, timeout=timeout)
  ed_time = time.time()
  if arts is None:
    print(f"FAIL ({errmsg or '<no output>'})")
    return False, None
  return True, ed_time - st_time


def run_gen_loop(fopts: FuncGenOptions, *, limit: int, shuffle: bool, check: bool, timeout: int):
  assert fopts.seed >= 0, "No randomness seed is given for running the generation loop"
  random.seed(fopts.seed)
  print(
    f"UUID={fopts.uuid}, output={fopts.outdir}, "
    f"limit={limit if limit != 0 else '<INF>'}, "
    f"seed={fopts.seed if fopts.seed >= 0 else '<RND>'}, "
    f"sexp={fopts.sexp}, main={fopts.main}, allops={fopts.allops}, injubs={fopts.injubs}, "
    f"shuffle={shuffle}, check={check}, timeout={timeout}s, "
    f"extra={"'" + fopts.extra + "'" if fopts.extra else '<NONE>'}"
  )
  fopts.sno = 0
  while limit == 0 or fopts.sno < limit:
    print(f"[{fopts.sno}]: Generate ...", end=" ", flush=True)
    fopts.seed = random.randint(0, 2147483647)
    if shuffle:
      fopts.config = random.choice(FGEN_SUGGESTED_CONFIGS)
    succ, elapsed = generate(fopts, timeout=timeout)
    if succ:
      print(f"SUCC (time={elapsed}s)")
    if check and succ:
      print(f"[{fopts.sno}]: CheckUBs ...", end=" ", flush=True)
      check_func_ubs(FunArts(fopts.uuid, fopts.sno, gen_dir=fopts.outdir))
      print("NO UBs")

    fopts.sno += 1


if __name__ == "__main__":
  parser = ArgumentParser(
    "rysmith", description="Reify tool for generating a set of leaf functions"
  )

  parser.add_argument(
    "--output",
    type=str,
    default=str(DEFAULT_OUTPUT_DIR),
    help="the directory saving the generated functions and their mappings",
  )
  parser.add_argument(
    "--limit",
    type=int,
    default=0,
    help="the number of functions to generate (0 for unlimited)",
  )
  parser.add_argument(
    "--seed",
    type=int,
    default=1773315633618043270,
    help="the seed for generation (negative for truly random)",
  )
  parser.add_argument(
    "--disable-sexp",
    action="store_true",
    default=False,
    help="disable the generation of the S Expression",
  )
  parser.add_argument(
    "--disable-allops",
    action="store_true",
    default=False,
    help="disable using all kinds of term and expression operators",
  )
  parser.add_argument(
    "--enable-injubs",
    action="store_true",
    default=False,
    help="enable injecting UBs into unexecuted basic blocks",
  )
  parser.add_argument(
    "--enable-shuffle",
    action="store_true",
    default=False,
    help="enable shuffling recommended configurations",
  )
  parser.add_argument(
    "--check",
    action="store_true",
    default=False,
    help="enable UB check per generated function",
  )
  parser.add_argument(
    "--timeout",
    type=int,
    default=3,
    help="timeout (in seconds) for generating a function",
  )

  args = parser.parse_args()

  outdir = Path(args.output).resolve().absolute()
  run_gen_loop(
    FuncGenOptions(
      bin="./build/bin/rysmith",
      uuid=next_uuid(),
      sno=0,
      outdir=outdir,
      config=FGEN_SUGGESTED_CONFIGS[0],
      verbose=args.check,
      main=True,
      sexp=not args.disable_sexp,
      allops=not args.disable_allops,
      injubs=args.enable_injubs,
      seed=args.seed if args.seed >= 0 else time.time_ns(),
      extra=f'--Xbitwuzla-threads {os.cpu_count()} --Xdisable-struct-vars --Xarray-var-proba 0.01',
    ),
    limit=args.limit,
    shuffle=args.enable_shuffle,
    check=args.check,
    timeout=args.timeout,
  )
