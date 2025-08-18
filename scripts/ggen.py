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

import json
import random
import shutil
import tempfile
import time
from abc import abstractmethod, ABC
from argparse import ArgumentParser
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path
from subprocess import CalledProcessError, TimeoutExpired
from typing import Optional, Tuple, List
from uuid import uuid4 as uuid

import pydot

import cmdline
from fuzz import MAX_I32, mlog, rand_int

SUPPORTED_PROGRAM_GENERATORS = ["csmith", "yarpgen", "preset"]


# -==========================================================
# Logging and utility functions
# -==========================================================


def merror(msg: str, exit_prog: bool = True):
  mlog("ERROR: " + msg, color="red")
  if exit_prog:
    exit(1)


def mwarning(msg: str):
  mlog("WARNING: " + msg, color="yellow")


# -==========================================================
# LLVM toolchain
# -==========================================================


class LLVM:

  def __init__(self, home: Path):
    self.home = home
    self.clang = self.home / "bin" / "clang"
    if not self.clang.exists():
      merror(f"LLVM clang binary not found at {self.clang}. Please build LLVM first.")
    self.opt = self.home / "bin" / "opt"
    if not self.opt.exists():
      merror(f"LLVM optimizer binary not found at {self.opt}. Please build LLVM first.")

  def emit_llvm(self, prog: Path, extra_opts: List[str], timeout: int) -> Tuple[Optional[Path], Optional[str]]:
    bprog = prog.with_suffix('.ll')
    try:
      cmdline.check_out(
        [
          str(self.clang),
          "-Xclang",
          "-disable-llvm-passes",
          "-emit-llvm",
          "-S",
        ] + extra_opts + [
          "-o",
          str(bprog),
          str(prog)
        ]
      )
      return bprog, None
    except CalledProcessError as e:
      return None, f"LLVM compilation failed: {e.stdout}"
    except TimeoutExpired as e:
      return None, f"LLVM compilation timed out: {e.stdout}"
    except Exception as e:
      return None, f"Unexpected error during LLVM compilation: {str(e)}"

  def emit_cfg_dot(self, bprog: Path, func: str, timeout: int) -> Tuple[Optional[Path], Optional[str]]:
    prefix = bprog.stem
    cfg_file = bprog.parent / f"{prefix}.{func}.dot"
    try:
      cmdline.check_out(
        [
          str(self.opt),
          "-disable-output",
          "-passes=dot-cfg",  # dot-cfg-only runs very slowly, so we use dot-cfg
          f"-cfg-dot-filename-prefix={prefix}",
          f"-cfg-func-name={func}",
          str(bprog.name),
        ],
        cwd=str(bprog.parent),
        timeout=timeout
      )
      return cfg_file, None
    except CalledProcessError as e:
      return None, f"CFG generation failed: {e.stdout}"
    except TimeoutExpired as e:
      return None, f"CFG generation timed out: {e.stdout}"
    except Exception as e:
      return None, f"Unexpected error during CFG generation: {str(e)}"


# -==========================================================
# Program generators (for LLVM compatible programs)
# -==========================================================


class BitCodeProgGen(ABC):

  @abstractmethod
  def next(self, name: str, outdir: Path, seed: int, timeout: int) -> Tuple[Optional[Path], Optional[str]]:
    """Return the path to the next generated LLVM bitcode program. If failed, return None and an error message."""
    ...

  def test_func(self) -> str:
    """Return the name of the test function to be used in the generated program."""
    ...


class YARPGen(BitCodeProgGen):

  def __init__(self, bin: Path, llvm: LLVM):
    super().__init__()
    self.bin = bin
    if not self.bin.exists():
      merror(f"YARPGen binary not found at {self.bin}. Please build YARPGen first.")
    self.llvm = llvm

  def test_func(self) -> str:
    return "test"

  def next(self, name: str, outdir: Path, seed: int, timeout: int) -> Tuple[Optional[Path], Optional[str]]:
    mlog(f"Generating C program with ID {name} using YARPGen ... ")
    cprog, errmsg = self.next_cprog(name=name, outdir=outdir, seed=seed, timeout=timeout)
    if cprog is None:
      return None, errmsg
    mlog(f"Succeeded: {cprog}")
    mlog(f"Compiling the C program into LLVM bitcode (text) ... ")
    return self.llvm.emit_llvm(cprog, extra_opts=["-mcmodel=large"], timeout=10)  # seconds

  def next_cprog(self, name: str, outdir: Path, seed: int, timeout: int) -> Tuple[Optional[Path], Optional[str]]:
    tmpdir = tempfile.mkdtemp(prefix="yarpgen-", dir=tempfile.gettempdir())
    try:
      cmdline.check_out([
        str(self.bin),
        f"--seed={seed}",
        "--std=c",
        f"--out-dir={tmpdir}",
      ], timeout=timeout)
    except CalledProcessError as e:
      shutil.rmtree(tmpdir)
      return None, f"Generation failure: {e.stdout}"
    except TimeoutExpired as e:
      shutil.rmtree(tmpdir)
      return None, f"Generation timed out: {e.stdout}"
    except Exception as e:
      shutil.rmtree(tmpdir)
      return None, f"Unexpected error during generation: {str(e)}"

    # Merge the generated files into a single C file
    tmp_path = Path(tmpdir)
    driv_file = tmp_path / "driver.c"
    if not driv_file.exists():
      shutil.rmtree(tmpdir)
      return None, "driver.c not found in the generated output."
    fun_file = tmp_path / "func.c"
    out_file = outdir / f"{name}.c"
    if not fun_file.exists():
      shutil.rmtree(tmpdir)
      return None, "func.c not found in the generated output."
    with out_file.open("w") as fou:
      # Replace the init header as it only declares extern variables
      # that are defined in the driver file.
      fou.write(driv_file.read_text())
      fou.write("\n\n")
      fou.write(fun_file.read_text().replace("#include \"init.h\"", ""))
    shutil.rmtree(tmpdir)

    return out_file, None


# -==========================================================
# Graph generation loop
# -==========================================================

class CfgSkel:
  def __init__(self) -> None:
    self.bbls: List[str] = []
    self.succs: List[List[int]] = []

  def to_dict(self) -> dict:
    return {
      "num_bbls": self.num_bbls(),
      "adj_tab": [
        [i, self.succs[i][0], self.succs[i][1]]
        for i in range(self.num_bbls())
        if self.succs[i][0] != -1 or self.succs[i][1] != -1
      ]
    }

  def num_bbls(self) -> int:
    return len(self.bbls)

  def num_jmps(self) -> int:
    return sum(len(s) for s in self.succs)

  def add_bbl(self, n: str) -> None:
    if n in self.bbls:
      merror(f"Basic block {n} already exists")
    self.bbls.append(n)
    self.succs.append([-1, -1])

  def set_succ(self, u: str, v: str, which: int) -> None:
    if which > 1 or which < 0:
      merror(f"Invalid successor index {which} which is neither 0 nor 1 for successor edge: {u} -> {v}")
    if u not in self.bbls:
      merror(f"Basic block {u} is not in graph")
    if v not in self.bbls:
      merror(f"Basic block {v} is not in graph")
    ui = self.bbls.index(u)
    if self.succs[ui][which] != -1:
      merror(
        f"Basic block {u} (id={ui}) already has a successor at index {which}: " +
        f"{self.bbls[self.succs[ui][which]]} (id={self.succs[ui][which]})"
      )
    vi = self.bbls.index(v)
    if vi == self.succs[ui][which]:
      merror(f"Successor {v} (id={vi}) of basic block {u} (id={ui}) already exists at index {which}: {u} -> {v}")
    self.succs[ui][which] = vi


def parse_dot_to_cfgs(dot_file: Path) -> List[CfgSkel]:
  results = []
  for dot in pydot.graph_from_dot_file(str(dot_file)):
    mlog(f">> CFG: {dot.get_name()}")
    cfg = CfgSkel()
    for node in dot.get_nodes():
      node_name = node.get_name()
      if not node_name.startswith("Node0x"):
        merror(f"Encountered an unexpected node in the DOT file: {node}")
      # mlog(f"Node: {node_name}")
      # for a in node.get_attributes():
      #   mlog(f"  {a} = {node.get_attributes()[a]}")
      cfg.add_bbl(node_name)
    for edge in dot.get_edges():
      source = edge.get_source()
      destination = edge.get_destination()
      if source.endswith(":s0"):
        source = source[:-3]
        which = 0
      elif source.endswith(":s1"):
        source = source[:-3]
        which = 1
      else:
        which = 0
      # mlog(f"Edge: {edge.get_source()}:{which} -> {edge.get_destination()}")
      # for a in edge.get_attributes():
      #   mlog(f"  {a} = {edge.get_attributes()[a]}")
      cfg.set_succ(source, destination, which=which)
    mlog(f">> Parsed CFG with {cfg.num_bbls()} basic blocks and {cfg.num_jmps()} block jumps.")
    if cfg.num_bbls() == 0 and cfg.num_jmps() == 0:
      mwarning(f"Empty CFG: Skipping.")
      continue
    elif cfg.num_bbls() <= 2 or cfg.num_jmps() <= 5:
      mwarning(f"Small CFG: Skipping.")
      continue
    # TODO: More strategies to cut large CFGs
    results.append(cfg)
    mlog(">> Added")
  return results


@dataclass
class GenStats:
  num_graphs: int = 0
  num_bbls: int = 0
  num_jmps: int = 0
  time_spent: float = 0.0
  bbl_dist: Counter = field(default_factory=Counter)
  edges_dist: Counter = field(default_factory=Counter)

  def contrib(self, g: CfgSkel, t: float):
    self.num_graphs += 1
    self.num_bbls += g.num_bbls()
    self.num_jmps += g.num_jmps()
    self.time_spent += t
    self.bbl_dist[g.num_bbls()] += 1
    self.edges_dist[g.num_jmps()] += 1

  @property
  def num_bbls_per(self) -> float:
    return self.num_bbls / self.num_graphs if self.num_graphs > 0 else 0.0

  @property
  def num_jmps_per(self) -> float:
    return self.num_jmps / self.num_graphs if self.num_graphs > 0 else 0.0

  @property
  def time_spent_per(self) -> float:
    return self.time_spent / self.num_graphs if self.num_graphs > 0 else 0.0


def run_gen_loop(gid: str, *, pgen: BitCodeProgGen, llvm: LLVM, limit: int, output: Path, timeout: int):
  outdir = Path(tempfile.mkdtemp(prefix=f"ggen-{gid}-", dir=tempfile.gettempdir()))
  mlog(f"Working on: {outdir}")
  stats = GenStats()
  counter = 0
  while limit <= 0 or counter < limit:
    time_per = time.time()
    fname = f"{gid}_{counter:08}"
    mlog(f"=> {counter}: Generating LLVM program with ID {fname} ... ", color='blue')
    bcprog, errmsg = pgen.next(name=fname, outdir=outdir, seed=rand_int(), timeout=timeout)
    if bcprog is None:
      mwarning(f"Failed: {errmsg}")
      continue
    mlog(f"Succeeded: {bcprog}")
    mlog(f"Extracting the control flow graph ... ")
    cfg_dot, errmsg = llvm.emit_cfg_dot(bcprog, func=pgen.test_func(), timeout=15 + timeout)
    if cfg_dot is None:
      mwarning(f"Failed: {errmsg}")
      continue
    mlog(f"Succeeded: {cfg_dot}")
    mlog("Parsing the DOT file into a CFG skeleton ... ")
    cfgs = parse_dot_to_cfgs(cfg_dot)
    if not cfgs:
      mwarning("Failed: No valid CFGs found in the DOT file.")
      continue
    mlog("Succeeded")
    time_per = time.time() - time_per
    with output.open("a") as fou:
      for g in cfgs:
        fou.write(json.dumps(g.to_dict()) + "\n")
        stats.contrib(g, time_per / len(cfgs))
    counter += len(cfgs)
  shutil.rmtree(outdir)
  mlog(f"The generated graphs are saved into: {output}", color='green')
  mlog("Generation statistics:")
  mlog(f"  Number of control-flow graphs : {stats.num_graphs}")
  mlog(f"  Total number of basic blocks  : {stats.num_bbls} (average: {stats.num_bbls_per:.2f})")
  mlog(f"  Total number of block jumps   : {stats.num_jmps} (average: {stats.num_jmps_per:.2f})")
  mlog(f"  Total time spent (seconds)    : {stats.time_spent:.2f} (average: {stats.time_spent_per:.2f})")
  mlog(f"  Basic blocks distribution     :")
  for bbl, count in stats.bbl_dist.most_common(10):
    mlog(f"    {bbl:5d} basic blocks : {count:5d} ({count / stats.num_graphs * 100:.2f}%)")
  mlog(f"  Block jumps distribution      :")
  for jmp, count in stats.edges_dist.most_common(10):
    mlog(f"    {jmp:5d} block jumps  : {count:5d} ({count / stats.num_graphs * 100:.2f}%)")


def main():
  parser = ArgumentParser("gen", description="Generate a graph database from a set of functions")
  parser.add_argument(
    "-l", "--llvm",
    default="/usr",
    type=str,
    help="Path to the LLVM installation directory (default: /usr)"
  )
  parser.add_argument(
    "-L", "--limit",
    default=MAX_I32,
    type=int,
    help=f"Limit the number of graphs to generate, non-positive meaning {MAX_I32} (default: {MAX_I32})"
  )
  parser.add_argument(
    "-g", "--generator",
    default="yarpgen",
    type=str,
    choices=SUPPORTED_PROGRAM_GENERATORS,
    help="The program generator to use (default: yarpgen)"
  )
  parser.add_argument(
    "-s", "--seed",
    type=int,
    default=-1,
    help="Seed for the randomness of the generation process, negative meaning no seed (default: -1)",
  )
  parser.add_argument(
    "--csmith",
    default=None,
    type=str,
    help="Path to the csmith binary. This is required when "
         "csmith is chosen as the program generator (default: None, not used)"
  )
  parser.add_argument(
    "--yarpgen",
    default=None,
    type=str,
    help="Path to the yarpgen binary. This is required when "
         "yarpgen is chosen as the program generator (default: None, not used)"
  )
  parser.add_argument(
    "--preset",
    default=None,
    type=str,
    help="Path to the directory saving the preset of programs. This is required when "
         "preset is chosen as the program generator (default: None, not used)"
  )
  parser.add_argument(
    "output",
    type=str,
    help="Path to the output file (.jsonl)"
  )

  args = parser.parse_args()

  llvm = Path(args.llvm)
  if not llvm.exists():
    merror(f"The LLVM directory (--llvm) does not exist: {llvm}.")
  if not llvm.is_dir():
    merror(f"The LLVM directory (--llvm) is not a directory: {llvm}")
  llvm = LLVM(llvm)

  gen = args.generator
  if gen not in SUPPORTED_PROGRAM_GENERATORS:
    merror(
      f"Unsupported program generator (--generator) '{gen}'. "
      f"Supported generators are: {', '.join(SUPPORTED_PROGRAM_GENERATORS)}"
    )
  if gen == "csmith":
    if args.csmith is None:
      merror(
        f"Option --csmith must be specified when using csmith as the program generator."
      )
    merror("Csmith is not yet supported in this version of the generator.")
  elif gen == "yarpgen":
    if args.yarpgen is None:
      merror(f"Option --yarpgen must be specified when using YARPGen as the program generator.")
    gen = YARPGen(Path(args.yarpgen), llvm)
  elif gen == "preset":
    if args.preset is None:
      merror(f"Option --preset must be specified when using Preset as the program generator.")
    merror("Preset generator is not yet supported in this version of the generator.")

  limit = args.limit
  if limit <= 0:
    limit = MAX_I32
  if limit > MAX_I32:
    mwarning(f"Limit (--limit) exceeds maximum allowed value ({MAX_I32}). Setting to {MAX_I32}.")
    limit = MAX_I32

  seed = args.seed
  if seed >= 0:
    mlog(f"Setting seed for random number generation: {seed}")
    random.seed(seed)
  else:
    seed = time.time_ns()
    random.seed(seed)
    mlog("No seed provided, using system randomness.")

  output = Path(args.output)
  if output.exists():
    if output.is_dir():
      merror(f"The output path already exists and is a directory: {output}")
    mlog(
      "The output file already exists. Would you like to overwrite it? (Y/n)> ",
      color='yellow',
      end='',
      flush=True
    )
    yes = input()
    if yes != "Y":
      mlog("The user chose not to overwrite the output directory. Exiting.", color='red')
      exit(0)
    output.unlink()

  run_gen_loop(
    gid=str(uuid()).replace('-', '_'),
    pgen=gen,
    llvm=llvm,
    limit=limit,
    output=output,
    timeout=15,  # Default timeout, can be adjusted
  )


if __name__ == '__main__':
  main()
