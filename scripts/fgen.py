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

from configs import DEFAULT_OUTPUT_DIR, get_func_map_files, get_prog_file, parse_mapping, CHKSUM_FUNC, get_crealdb_file
from fuzz import FuncGenOptions, generate_function
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


def crealize(cdb_file: Path, func_file: Path, map_file: Path):
  func_name = func_file.stem
  func_code = func_file.read_text()
  func_maps = parse_mapping(map_file)
  num_maps = len(func_maps)
  num_params = len(func_maps[0][0])

  # Change the checksum function into a checksum check function
  # and add the new checksum check function to the code.
  chkchk_name = func_name + "_checksum"
  func_code = func_code.replace(CHKSUM_FUNC, chkchk_name)
  func_code = f"""\
int {chkchk_name}(int size, int args[]) {{
  int outs[{num_maps}][{num_params}] = {{
{",\n".join(["    {" + ",".join([str(x) for x in m[1]]) + "}" for m in func_maps])}
  }};
  int chks[{num_maps}] = {{
    {",".join([str(m[2]) for m in func_maps])}
  }};
  for (int i = 0; i < {num_maps}; i ++) {{
    int found = 1;
    for (int j = 0; j < {num_params}; j ++) {{
      if (args[j] != outs[i][j]) {{
        found = 0;
        break; // Checksum mismatch
      }}
    }}
    if (found) {{
      return chks[i]; // Return the checksum value
    }}
  }}
  abort(); // No matching checksum found
  return -2147483648; // Should never reach here
}}\n
""" + func_code

  with cdb_file.open("a") as fout:
    fout.write(json.dumps({
      "function_name": func_name,
      "parameter_types": ["int"] * num_params,
      "return_type": "int",
      "function": func_code,
      "io_list": [[
        [int(x) for x in m[0]], int(m[2])
      ] for m in func_maps],
      "misc": [],
      "src_file": "",
      "include_headers": ["stdlib.h"],
      "include_sources": [],
    }) + "\n")

  return True


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
    f"sexp={fopts.sexp}, main={fopts.main}, allops={fopts.allops}, injubs={fopts.injubs}, "
    f"check={check}, crealdb={crealdb} timeout={timeout}s, "
    f"extra={'\'' + fopts.extra + '\'' if fopts.extra else '<NONE>'}"
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

  args = parser.parse_args()

  outdir = Path(args.output).resolve().absolute()
  run_gen_loop(
    FuncGenOptions(
      bin="./build/bin/fgen",
      uuid=str(uuid.uuid4()),
      sno=0,
      outdir=outdir,
      verbose=args.check,
      main=args.main,
      sexp=args.sexp,
      allops=args.allops,
      injubs=args.injubs,
      seed=args.seed,  # We save it as the initial seed
      extra=args.extra,
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
