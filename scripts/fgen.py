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
import json
import random
import time
import uuid
from argparse import ArgumentParser
from pathlib import Path

from configs import DEFAULT_OUTPUT_DIR, get_func_map_files, get_prog_file, get_crealdb_file
from fuzz import FuncGenOptions, generate_function, crealize, FGEN_SUGGESTED_CONFIGS
from ubchk import check_ubs, check_ubs_once


def generate(opts: FuncGenOptions, *, timeout: int):
  st_time = time.time()
  func, errmsg = generate_function(opts, timeout=timeout)
  ed_time = time.time()
  if func is None:
    print(f"GEN ERROR (.): {errmsg or '<no output>'}")
    return False, None
  return True, ed_time - st_time


def get_tmp_crealdb_file(gen_dir: Path):
  return gen_dir / "crealdb.tmp.jsonl"


def run_gen_loop(fopts: FuncGenOptions, *, limit: int, check: bool, timeout: int, crealdb: bool):
  if fopts.seed >= 0:
    random.seed(fopts.seed)
    next_seed = lambda: random.randint(0, 2147483647)
  else:
    next_seed = lambda: None
  print(
    f"UUID={fopts.uuid}, output={fopts.outdir}, "
    f"limit={limit if limit != 0 else '<INF>'}, "
    f"seed={fopts.seed if fopts.seed >= 0 else '<RND>'}, "
    f"sexp={fopts.sexp}, main={fopts.main}, wasm={fopts.wasm}, allops={fopts.allops}, injubs={fopts.injubs}, "
    f"check={check}, crealdb={crealdb}, timeout={timeout}s, "
    f"extra={repr(fopts.extra) if fopts.extra else '<NONE>'}"
    f", wasm_sexp_pct={fopts.wasm_sexp_pct}%, wasm_unreachable_pct={fopts.wasm_unreachable_pct}%, "
    f"wasm_folding_pct={fopts.wasm_folding_pct}%, wasm_anon_decl_pct={fopts.wasm_anon_decl_pct}%, wasm_anon_usage_pct={fopts.wasm_anon_usage_pct}%"
  )
  fopts.sno = 0
  while limit == 0 or fopts.sno < limit:
    print(f"[{fopts.sno}]: Generate ...", end=" ", flush=True)
    fopts.seed = next_seed()
    succ, elapsed = generate(fopts, timeout=timeout)
    if succ:
      print(f"SUCC (time={elapsed}s)")
    if check and succ:
      print(f"[{fopts.sno}]: CheckUBs ...", end=" ", flush=True)
      check_ubs(*get_func_map_files(fopts.uuid, fopts.sno, gen_dir=fopts.outdir))
      if fopts.main:
        check_ubs_once(
          get_prog_file(fopts.uuid, fopts.sno, gen_dir=fopts.outdir)
        )
      print("NO UBs")
    if crealdb and succ:
      print(f"[{fopts.sno}]: GenCreal ...", end=" ", flush=True)
      if crealize(get_tmp_crealdb_file(fopts.outdir), *get_func_map_files(fopts.uuid, fopts.sno, gen_dir=fopts.outdir)):
        print("SUCC")
      else:
        print("FAIL")

    fopts.sno += 1


if __name__ == "__main__":
  parser = ArgumentParser(
    "fgen", description="Tool for generating a set of functions"
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
    default=-1,
    help="the seed for generation (negative for truly random)",
  )
  parser.add_argument(
    "--sexp",
    action="store_true",
    default=False,
    help="enable the generation of the S Expression",
  )
  parser.add_argument(
    "--main",
    action="store_true",
    default=False,
    help="enable the generation of a main function",
  )
  parser.add_argument(
    "--wasm",
    action="store_true",
    default=False,
    help="enable the generation of a Wasm program",
  )
  parser.add_argument(
    "--allops",
    action="store_true",
    default=False,
    help="enable using all kinds of term and expression operators",
  )
  parser.add_argument(
    "--injubs",
    action="store_true",
    default=False,
    help="enable using all kinds of term and expression operators",
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
    # Our experiences are that in the default settings of fgen, if a program cannot
    # be generated within 3 seconds, then it won't be generated even given 15 seconds.
    # Thus, using 3s significantly improves the throughput of us.
    default=3,
    help="timeout (in seconds) for generating a function",
  )
  parser.add_argument(
    "--extra", type=str, default=None, help="extra options passed to fgen"
  )
  parser.add_argument(
    "--crealdb",
    action="store_true",
    default=False,
    help="build a Creal compatible function database from the generated functions",
  )
  parser.add_argument(
    "--wasm_sexp_pct",
    type=int,
    default=50,
    help="configure the chance that a construct is emitted in s-expression-style rather than line-by-line style",
  )
  parser.add_argument(
    "--wasm_unreachable_pct",
    type=int,
    default=50,
    help="configure the chance that a block that is not on the execution path is emitted as an unreachable block",
  )
  parser.add_argument(
    "--wasm_folding_pct",
    type=int,
    default=50,
    help="configure the chance that a local definition is folded (and anonymized) with a random amount of neighboring locals",
  )
  parser.add_argument(
    "--wasm_anon_decl_pct",
    type=int,
    default=50,
    help="configure the chance that a variable is declared without a name (and later referred to by index)",
  )
  parser.add_argument(
    "--wasm_anon_usage_pct",
    type=int,
    default=50,
    help="configure the chance that a variable use refers to a variable by index instead of by name",
  )
  

  args = parser.parse_args()

  outdir = Path(args.output).resolve().absolute()
  run_gen_loop(
    FuncGenOptions(
      bin="./build/bin/fgen",
      uuid=str(uuid.uuid4()),
      sno=0,
      outdir=outdir,
      config=FGEN_SUGGESTED_CONFIGS[0],
      verbose=args.check,
      main=args.main,
      sexp=args.sexp,
      wasm=args.wasm,
      allops=args.allops,
      injubs=args.injubs,
      seed=args.seed,  # We save it as the initial seed
      extra=args.extra,
      wasm_sexp_pct=args.wasm_sexp_pct,
      wasm_unreachable_pct=args.wasm_unreachable_pct,
      wasm_folding_pct=args.wasm_folding_pct,
      wasm_anon_decl_pct=args.wasm_anon_decl_pct,
      wasm_anon_usage_pct=args.wasm_anon_usage_pct,
    ),
    limit=args.limit,
    check=args.check,
    timeout=args.timeout,
    crealdb=args.crealdb
  )

  if args.crealdb:
    print(f"Converting CrealDB to JSON format ...")
    cdb_tmp_file = get_tmp_crealdb_file(outdir)
    with cdb_tmp_file.open() as fin:
      items = []
      for line in fin:
        if not line:
          continue
        items.append(json.loads(line.strip()))
    cdb_file = get_crealdb_file(outdir)
    with cdb_file.open("w") as fout:
      json.dump(items, fout, indent=2)
    print(f"CrealDB saved to {cdb_file}")
