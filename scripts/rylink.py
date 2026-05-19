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

import random
import time
from argparse import ArgumentParser
from pathlib import Path

from configs import DEFAULT_OUTPUT_DIR
from fuzz import (
  PGEN_SUGGESTED_CONFIGS,
  ProgGenOptions,
  generate_programs,
  next_uuid,
)
from ubchk import check_prog_ubs


def generate(opts: ProgGenOptions):
  st_time = time.time()
  arts, errmsg = generate_programs(opts)
  ed_time = time.time()
  if arts is None:
    print(f"FAIL ({errmsg or '<no output>'})")
    return False, None, None
  return True, ed_time - st_time, arts


def run_gen_loop(popts: ProgGenOptions, *, check: bool, shuffle: bool):
  assert popts.seed >= 0, "No randomness seed is given for running the generation loop"
  random.seed(popts.seed)
  total_limit = popts.limit
  print(
    f"input={popts.indir}, "
    f"limit={total_limit if total_limit != 0 else '<INF>'}, "
    f"seed={popts.seed if popts.seed >= 0 else '<RND>'}, "
    f"check={check}, shuffle={shuffle}, "
    f"extra={"'" + popts.extra + "'" if popts.extra else '<NONE>'}"
  )
  num_confs = len(PGEN_SUGGESTED_CONFIGS) if shuffle else 1
  # If limit is 0, it means we want to generate programs without a limit. In this case, we can just keep generating 1000 programs each config, and round robin.
  # Otherwise, we generate limit/numconfigs per config.
  # Change the uuid for each gen.
  batch = 1000 if total_limit == 0 else max(1, total_limit // num_confs)

  generated = 0
  while total_limit == 0 or generated < total_limit:
    popts.uuid = next_uuid()
    popts.seed = random.randint(0, 2147483647)
    popts.limit = batch if total_limit == 0 else min(batch, total_limit - generated)
    if shuffle:
      popts.config = random.choice(PGEN_SUGGESTED_CONFIGS)
    else:
      popts.config = PGEN_SUGGESTED_CONFIGS[-1]
    # In limited mode, shrink the final batch so we land on exactly
    # `total_limit` instead of overshooting it.

    print(f"[{popts.uuid}]: Generate ...", end=" ", flush=True)
    succ, elapsed, arts = generate(popts)
    if succ:
      print(f"SUCC (n={len(arts)}, time={elapsed}s)")
      generated += len(arts)
    if check and succ:
      print(f"[{popts.uuid}]: CheckUBs ...", end=" ", flush=True)
      for a in arts:
        check_prog_ubs(a)
      print("NO UBs")


if __name__ == "__main__":
  parser = ArgumentParser("rylink", description="Reify tool for generating a set of whole programs")

  parser.add_argument(
    "--input",
    type=str,
    default=str(DEFAULT_OUTPUT_DIR),
    help="the directory saving the already generated functions and to save the generated programs",
  )
  parser.add_argument(
    "--limit",
    type=int,
    default=0,
    help="the number of programs to generate (0 for unlimited)",
  )
  parser.add_argument(
    "--seed",
    type=int,
    default=-1,
    help="the seed for generation (negative for truly random)",
  )
  parser.add_argument(
    "--disable-shuffle",
    action="store_true",
    default=False,
    help="disable shuffling recommended configurations",
  )
  parser.add_argument(
    "--check",
    action="store_true",
    default=False,
    help="enable UB check per generated program",
  )
  parser.add_argument("--extra", type=str, default=None, help="extra options passed to rylink")

  args = parser.parse_args()

  indir = Path(args.input).resolve().absolute()
  run_gen_loop(
    ProgGenOptions(
      bin="./build/bin/rylink",
      uuid=next_uuid(),
      indir=indir,
      limit=args.limit,
      config=PGEN_SUGGESTED_CONFIGS[0],
      seed=args.seed if args.seed >= 0 else time.time_ns(),
      extra=args.extra,
    ),
    check=args.check,
    shuffle=not args.disable_shuffle,
  )
