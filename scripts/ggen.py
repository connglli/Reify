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

import glob
import json
import random
import re
import shlex
import shutil
import tempfile
import time
from abc import ABC, abstractmethod
from argparse import ArgumentParser
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path
from subprocess import CalledProcessError, TimeoutExpired
from typing import List, Optional, Tuple
from uuid import uuid4 as uuid

import cmdline
import pydot
from fuzz import MAX_I32, mlog, rand_int

SUPPORTED_PROGRAM_GENERATORS = ["csmith", "yarpgen", "preset"]
GLOBAL_OPTION_MIN_CFG_BLOCKS = 5
GLOBAL_OPTION_MAX_CFG_BLOCKS = 256
GLOBAL_OPTION_MIN_CFG_JUMPS = 10
GLOBAL_OPTION_MAX_CFG_JUMPS = GLOBAL_OPTION_MAX_CFG_BLOCKS * 2


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
    self.clangxx = self.home / "bin" / "clang++"
    if not self.clang.exists():
      merror(f"LLVM clang binary not found at {self.clang}. Please build LLVM first.")
    self.opt = self.home / "bin" / "opt"
    if not self.opt.exists():
      merror(f"LLVM optimizer binary not found at {self.opt}. Please build LLVM first.")

  def emit_llvm(
    self, prog: Path, extra_opts: List[str], timeout: int
  ) -> Tuple[Optional[Path], Optional[str]]:
    bprog = prog.with_suffix(".ll")
    try:
      # TODO: Disable function inlining
      cmdline.check_out(
        [
          str(self.clang),
          "-Xclang",
          "-disable-llvm-passes",
          "-emit-llvm",
          "-S",
        ]
        + extra_opts
        + ["-o", str(bprog), str(prog)],
        timeout=timeout,
      )
      return bprog, None
    except CalledProcessError as e:
      return None, f"LLVM compilation failed: {e.stdout}"
    except TimeoutExpired as e:
      return None, f"LLVM compilation timed out: {e.stdout}"
    except Exception as e:
      return None, f"Unexpected error during LLVM compilation: {str(e)}"

  def emit_cfg_dots(
    self, bprog: Path, *, fn_pat: str, funcs: List[str], timeout: int
  ) -> Tuple[Optional[List[Path]], Optional[str]]:
    """
    Emit the control flow graph (CFG) of functions (matching fn_pat) in the given LLVM program
    in DOT format and return the paths to the generated DOT files for each function specified by funcs.
    If fn_pat is not specified, it will generate the CFG for all functions.
    If funcs is not specified, it will return paths to all the generated CFGs.
    """
    prefix = bprog.stem
    try:
      fn_opt = [f"-cfg-func-name={fn_pat}"] if fn_pat else []
      cmdline.check_out(
        [
          str(self.opt),
          "-disable-output",
          "-passes=dot-cfg",  # dot-cfg-only runs very slowly, so we use dot-cfg
          f"-cfg-dot-filename-prefix={prefix}",
        ]
        + fn_opt
        + [
          str(bprog.name),
        ],
        cwd=str(bprog.parent),
        timeout=timeout,
      )
      if funcs:
        cfg_files = [bprog.parent / f"{prefix}.{fn}.dot" for fn in funcs]
      else:
        cfg_files = [
          bprog.parent / s for s in glob.glob(f"{prefix}.*.dot", root_dir=str(bprog.parent))
        ]
      # There're chances that the functions are inlined and not found in the LLVM program
      return [cf for cf in cfg_files if cf.exists()], None
    except CalledProcessError as e:
      return None, f"CFG generation failed: {e.stdout}"
    except TimeoutExpired as e:
      return None, f"CFG generation timed out: {e.stdout}"
    except Exception as e:
      return None, f"Unexpected error during CFG generation: {str(e)}"


# -==========================================================
# Program generators (for LLVM compatible programs)
# -==========================================================


class LLVMProgGenRes:
  file: Optional[Path] = None  # Path to the generated LLVM (bitcode) program
  funcs: Optional[
    List[str]
  ] = None  # The pattern of the test functions in the generated program, None or [] means all functions
  errmsg: Optional[str] = None  # Error message if the generation failed

  @staticmethod
  def success(file: Path, funcs: List[str]) -> "LLVMProgGenRes":
    result = LLVMProgGenRes()
    result.file = file
    result.funcs = funcs
    result.errmsg = None
    return result

  @staticmethod
  def failure(errmsg: str) -> "LLVMProgGenRes":
    result = LLVMProgGenRes()
    result.file = None
    result.funcs = None
    result.errmsg = errmsg
    return result


class LLVMProgGen(ABC):
  @abstractmethod
  def next(self, name: str, outdir: Path, seed: int, timeout: int) -> LLVMProgGenRes:
    """Return the next generated LLVM program. Or raise StopIteration if no more programs can be generated."""
    ...

  def fn_pat(self) -> str:
    """Return the common pattern of the generated functions in the generated program."""
    ...


class Csmith(LLVMProgGen):
  _FUNC_PATTERN = re.compile(r"^static .*? (func_\d+)\(.*?\);$")

  def __init__(self, bin: Path, llvm: LLVM):
    super().__init__()
    self.bin = bin
    if not self.bin.exists():
      merror(f"Csmith binary not found at {self.bin}. Please build Csmith first.")
    self.home = self.bin.parent.parent
    self.llvm = llvm

  def next(self, name: str, outdir: Path, seed: int, timeout: int) -> LLVMProgGenRes:
    mlog(f"Generating C program with ID {name} using Csmith ... ")
    res = self.next_cprog(name=name, outdir=outdir, seed=seed, timeout=timeout)
    if res.file is None:
      return res
    mlog(f"Succeeded: {res.file}")
    mlog("Compiling the C program into LLVM (bitcode) program (text) ... ")
    bprog, errmsg = self.llvm.emit_llvm(
      res.file, extra_opts=[f"-I{self.home / 'include'}"], timeout=10
    )  # seconds
    if bprog is None:
      return LLVMProgGenRes.failure(errmsg)
    return LLVMProgGenRes.success(bprog, res.funcs)

  def fn_pat(self) -> str:
    return "func_"

  def next_cprog(self, name: str, outdir: Path, seed: int, timeout: int) -> LLVMProgGenRes:
    out_file = outdir / f"{name}.c"
    try:
      cmdline.check_out(
        [
          str(self.bin),
          "--seed",
          str(seed),
          "--output",
          str(out_file),
        ],
        timeout=timeout,
      )
    except CalledProcessError as e:
      return LLVMProgGenRes.failure(f"Generation failure: {e.stdout}")
    except TimeoutExpired as e:
      return LLVMProgGenRes.failure(f"Generation timed out: {e.stdout}")
    except Exception as e:
      return LLVMProgGenRes.failure(f"Unexpected error during generation: {str(e)}")

    # Parse to find all test functions
    cont = out_file.read_text()
    lines = cont.splitlines()
    # All functions are forward declared between these two markers
    try:
      start_index = lines.index("/* --- FORWARD DECLARATIONS --- */")
    except ValueError:
      out_file.unlink()
      return LLVMProgGenRes.failure(
        f"No '/* --- FORWARD DECLARATIONS --- */' found in the generated C program: {cont}"
      )
    try:
      end_index = lines.index("/* --- FUNCTIONS --- */")
    except ValueError:
      out_file.unlink()
      return LLVMProgGenRes.failure(
        f"No '/* --- FUNCTIONS --- */' found in the generated C program: {cont}"
      )

    test_funcs = []
    for line in lines[start_index + 1 : end_index]:
      mat = self._FUNC_PATTERN.fullmatch(line)
      if mat is None:
        continue  # Skip non-function lines
      test_funcs.append(mat.group(1))

    return LLVMProgGenRes.success(out_file, test_funcs)


class YARPGen(LLVMProgGen):
  def __init__(self, bin: Path, llvm: LLVM):
    super().__init__()
    self.bin = bin
    if not self.bin.exists():
      merror(f"YARPGen binary not found at {self.bin}. Please build YARPGen first.")
    self.llvm = llvm

  def fn_pat(self) -> str:
    return "test"

  def next(self, name: str, outdir: Path, seed: int, timeout: int) -> LLVMProgGenRes:
    mlog(f"Generating C program with ID {name} using YARPGen ... ")
    cres = self.next_cprog(name=name, outdir=outdir, seed=seed, timeout=timeout)
    if cres.file is None:
      return cres
    mlog(f"Succeeded: {cres.file}")
    mlog("Compiling the C program into LLVM (bitcode) program (text) ... ")
    bprog, errmsg = self.llvm.emit_llvm(
      cres.file, extra_opts=["-mcmodel=large"], timeout=10
    )  # seconds
    if bprog is None:
      return LLVMProgGenRes.failure(errmsg)
    return LLVMProgGenRes.success(bprog, cres.funcs)

  def next_cprog(self, name: str, outdir: Path, seed: int, timeout: int) -> LLVMProgGenRes:
    tmpdir = tempfile.mkdtemp(prefix="yarpgen-", dir=tempfile.gettempdir())
    try:
      cmdline.check_out(
        [
          str(self.bin),
          f"--seed={seed}",
          "--std=c",
          f"--out-dir={tmpdir}",
        ],
        timeout=timeout,
      )
    except CalledProcessError as e:
      shutil.rmtree(tmpdir)
      return LLVMProgGenRes.failure(f"Generation failure: {e.stdout}")
    except TimeoutExpired as e:
      shutil.rmtree(tmpdir)
      return LLVMProgGenRes.failure(f"Generation timed out: {e.stdout}")
    except Exception as e:
      shutil.rmtree(tmpdir)
      return LLVMProgGenRes.failure(f"Unexpected error during generation: {str(e)}")

    # Merge the generated files into a single C file
    tmp_path = Path(tmpdir)
    driv_file = tmp_path / "driver.c"
    if not driv_file.exists():
      shutil.rmtree(tmpdir)
      return LLVMProgGenRes.failure("driver.c not found in the generated output.")
    fun_file = tmp_path / "func.c"
    out_file = outdir / f"{name}.c"
    if not fun_file.exists():
      shutil.rmtree(tmpdir)
      return LLVMProgGenRes.failure("func.c not found in the generated output.")
    with out_file.open("w") as fou:
      # Replace the init header as it only declares extern variables
      # that are defined in the driver file.
      fou.write(driv_file.read_text())
      fou.write("\n\n")
      fou.write(fun_file.read_text().replace('#include "init.h"', ""))
    shutil.rmtree(tmpdir)

    return LLVMProgGenRes.success(out_file, ["test"])


class Preset(LLVMProgGen):
  _C_COMMAND_PATTERN = re.compile(r"^(/?(?:\w+/)*(?:clang|gcc|cc))\s+(.*?)\s+(-o\s+.*?)\s+(.*?)")
  _CXX_COMMAND_PATTERN = re.compile(
    r"^(/?(?:\w+/)*(?:clang\+\+|g\+\+|c\+\+))\s+(.*?)\s+(-o\s+.*?)\s+(.*?)"
  )

  def __init__(self, preset: Path, llvm: LLVM):
    super().__init__()
    self.preset = preset
    if not self.preset.exists():
      merror(f"Preset file (should be a compile_commands.json) not found at {self.preset}.")
    with self.preset.open() as fin:
      self.commands = json.load(fin)
    self._ptr = -1
    self.llvm = llvm

  def next(self, name: str, outdir: Path, seed: int, timeout: int) -> LLVMProgGenRes:
    mlog(f"Generating the next program with ID {name} using Preset ... ")
    bprog = outdir / f"{name}.ll"
    cmd, workdir = self._parse_next_command(bprog)
    try:
      cmdline.check_out(shlex.split(cmd), timeout=timeout, cwd=workdir)
      return LLVMProgGenRes.success(bprog, [])
    except CalledProcessError as e:
      return LLVMProgGenRes.failure(f"LLVM compilation failed: {e.stdout}")
    except TimeoutExpired as e:
      return LLVMProgGenRes.failure(f"LLVM compilation timed out: {e.stdout}")
    except Exception as e:
      return LLVMProgGenRes.failure(f"Unexpected error during LLVM compilation: {str(e)}")

  def fn_pat(self) -> str:
    return ""  # Indicating all found functions in the preset

  def _parse_next_command(self, bprog: Path) -> Tuple[str, str]:
    while self._ptr < len(self.commands) - 1:
      self._ptr += 1
      cmd_obj = self.commands[self._ptr]
      mlog(
        f"Testing command at index {self._ptr}: "
        + (cmd_obj["file"] if "file" in cmd_obj else "<unknown-file>")
      )
      if "directory" not in cmd_obj:
        mwarning(f"Failure: 'directory' not found in the command object, skipping: {cmd_obj}")
        continue
      workdir = cmd_obj["directory"]
      if "command" not in cmd_obj:
        if "arguments" not in cmd_obj:
          mwarning(
            f"Failure: Neither 'command' nor 'arguments' found in the command object, skipping: {cmd_obj}"
          )
          continue
        cmd_obj["command"] = " ".join(cmd_obj["arguments"])
      orig_cmd = cmd_obj["command"]
      match = self._C_COMMAND_PATTERN.fullmatch(orig_cmd)
      compiler = str(self.llvm.clang)
      extra_incl = ""
      if match is None:
        match = self._CXX_COMMAND_PATTERN.fullmatch(orig_cmd)
        compiler = str(self.llvm.clangxx)
        extra_incl = "-I/usr/include/c++/v1 "  # For libc++ headers
      if match is None:
        mwarning(f"Failure: Invalid command (neither for C nor for C++), skipping: {orig_cmd}")
        continue
      cmd = f"{compiler} -Xclang -disable-llvm-passes -emit-llvm -S {extra_incl} {match[2]} -o {bprog} {match[4]}"
      mlog(f"Success: Transform to command: {cmd}")
      return cmd, workdir
    mlog("All commands have been processed")
    raise StopIteration()


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
      ],
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
    if which not in [0, 1, -1]:
      merror(
        f"Invalid successor index {which} which is neither 0, 1, nor -1 for successor edge: {u} -> {v}"
      )
    if u not in self.bbls:
      merror(f"Basic block {u} is not in graph")
    if v not in self.bbls:
      merror(f"Basic block {v} is not in graph")
    ui = self.bbls.index(u)
    vi = self.bbls.index(v)
    if which == -1:  # Append to the first available successor slot
      if self.succs[ui][0] == -1:
        self.succs[ui][0] = vi
      elif self.succs[ui][1] == -1:
        self.succs[ui][1] = vi
      else:
        merror(
          f"Basic block {u} (id={ui}) already has two successors: "
          f"{self.bbls[self.succs[ui][0]]} (id={self.succs[ui][0]}), "
          f"{self.bbls[self.succs[ui][1]]} (id={self.succs[ui][1]})"
        )
      return
    if self.succs[ui][which] != -1:
      merror(
        f"Basic block {u} (id={ui}) already has a successor at index {which}: "
        + f"{self.bbls[self.succs[ui][which]]} (id={self.succs[ui][which]})"
      )
    if vi == self.succs[ui][which]:
      merror(
        f"Successor {v} (id={vi}) of basic block {u} (id={ui}) already exists at index {which}: {u} -> {v}"
      )
    self.succs[ui][which] = vi

  @staticmethod
  def parse(dot: pydot.Dot) -> "CfgSkel":
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
        if ":s" in source:
          raise ValueError(f"Unexpected successor index for Node: {source}")
        which = -1
      # mlog(f"Edge: {edge.get_source()}:{which} -> {edge.get_destination()}")
      # for a in edge.get_attributes():
      #   mlog(f"  {a} = {edge.get_attributes()[a]}")
      cfg.set_succ(source, destination, which=which)
    return cfg


def parse_dot_to_cfgs(dot_file: Path) -> List[CfgSkel]:
  global \
    GLOBAL_OPTION_MIN_CFG_BLOCKS, \
    GLOBAL_OPTION_MAX_CFG_BLOCKS, \
    GLOBAL_OPTION_MIN_CFG_JUMPS, \
    GLOBAL_OPTION_MAX_CFG_JUMPS
  try:
    dot_list = pydot.graph_from_dot_file(str(dot_file))
  except Exception as e:
    mwarning(f"Unable to parse the dot file: {dot_file}: {e}")
    return []
  if not dot_list:
    mwarning(f"No graphs found in the dot file: {dot_file}")
    return []
  results = []
  for dot in dot_list:
    mlog(f">> CFG: {dot.get_name()}")
    try:
      cfg = CfgSkel.parse(dot)
    except Exception as e:
      mwarning(f"Failed to parse CFG: {e}")
      continue
    mlog(f">> Parsed CFG with {cfg.num_bbls()} basic blocks and {cfg.num_jmps()} block jumps.")
    if cfg.num_bbls() == 0 and cfg.num_jmps() == 0:
      mwarning("Empty CFG: Skipping.")
      continue
    elif (
      cfg.num_bbls() <= GLOBAL_OPTION_MIN_CFG_BLOCKS
      or cfg.num_jmps() <= GLOBAL_OPTION_MIN_CFG_JUMPS
    ):
      mwarning("Small CFG: Skipping.")
      continue
    elif (
      cfg.num_bbls() > GLOBAL_OPTION_MAX_CFG_BLOCKS or cfg.num_jmps() > GLOBAL_OPTION_MAX_CFG_JUMPS
    ):
      mwarning("Large CFG: Skipping.")
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
  jmp_dist: Counter = field(default_factory=Counter)

  def contrib(self, g: CfgSkel, t: float):
    self.num_graphs += 1
    self.num_bbls += g.num_bbls()
    self.num_jmps += g.num_jmps()
    self.time_spent += t
    self.bbl_dist[g.num_bbls()] += 1
    self.jmp_dist[g.num_jmps()] += 1

  @property
  def num_bbls_per(self) -> float:
    return self.num_bbls / self.num_graphs if self.num_graphs > 0 else 0.0

  @property
  def num_jmps_per(self) -> float:
    return self.num_jmps / self.num_graphs if self.num_graphs > 0 else 0.0

  @property
  def time_spent_per(self) -> float:
    return self.time_spent / self.num_graphs if self.num_graphs > 0 else 0.0

  @property
  def max_num_bbls(self) -> int:
    largest = -1
    for bbl, count in self.bbl_dist.items():
      if bbl > largest:
        largest = bbl
    return largest

  @property
  def min_num_bbls(self) -> int:
    smallest = MAX_I32
    for bbl, count in self.bbl_dist.items():
      if bbl < smallest:
        smallest = bbl
    return smallest

  @property
  def max_num_jmps(self) -> int:
    largest = -1
    for jmp, count in self.jmp_dist.items():
      if jmp > largest:
        largest = jmp
    return largest

  @property
  def min_num_jmps(self) -> int:
    smallest = MAX_I32
    for jmp, count in self.jmp_dist.items():
      if jmp < smallest:
        smallest = jmp
    return smallest


def run_gen_loop(
  gid: str, *, pgen: LLVMProgGen, llvm: LLVM, limit: int, output: Path, timeout: int
):
  outdir = Path(tempfile.mkdtemp(prefix=f"ggen-{gid}-", dir=tempfile.gettempdir()))
  mlog(f"Working on: {outdir}")
  stats = GenStats()
  simple_hash_cache = set()
  while limit <= 0 or stats.num_graphs < limit:
    counter = stats.num_graphs
    time_per = time.time()
    fname = f"{gid}_{counter:08}"
    mlog(f"=> {counter}: Generating LLVM program with ID {fname} ... ", color="blue")
    try:
      gen_res = pgen.next(name=fname, outdir=outdir, seed=rand_int(), timeout=timeout)
    except StopIteration:
      mlog("No more programs can be generated due to StopIteration, stopping.")
      break
    if gen_res.file is None:
      mwarning(f"Failed: {gen_res.errmsg}")
      continue
    mlog(f"Succeeded: {gen_res.file}")
    mlog("Extracting the control flow graph ... ")
    cfg_files, errmsg = llvm.emit_cfg_dots(
      gen_res.file, fn_pat=pgen.fn_pat(), funcs=gen_res.funcs, timeout=15 + timeout
    )
    if not cfg_files:
      mwarning(f"Failed: {errmsg}")
      continue
    mlog(f"Succeeded: {len(cfg_files)} returned ([{cfg_files[0].relative_to(outdir)}, ...])")
    mlog("Parsing the DOT file into a CFG skeleton ... ")
    cfgs = []
    for cf in cfg_files:
      cfgs = cfgs + parse_dot_to_cfgs(cf)
    if not cfgs:
      mwarning("Failed: No valid CFGs found in the DOT file.")
      continue
    mlog("Succeeded")
    time_per = time.time() - time_per
    with output.open("a") as fou:
      for g in cfgs:
        # We enforce a simplest dedup strategy. However this is not
        # sufficient to remove all isomorphic graphs.
        g_str = json.dumps(g.to_dict())
        g_hash = hash(g_str)
        if g_hash in simple_hash_cache:
          mwarning("Duplicate graph detected, skipping.")
          continue
        fou.write(g_str + "\n")
        simple_hash_cache.add(g_hash)
        stats.contrib(g, time_per / len(cfgs))
  shutil.rmtree(outdir)
  # TODO: Leverage NetworkX to remove isomorphic graphs
  mlog(f"The generated graphs are saved into: {output}", color="green")
  mlog("Generation statistics:")
  mlog(f"  Number of control-flow graphs : {stats.num_graphs}")
  mlog(
    f"  Total number of basic blocks  : {stats.num_bbls} "
    f"(ave: {stats.num_bbls_per:.2f}, max: {stats.max_num_bbls}, min: {stats.min_num_bbls})"
  )
  mlog(
    f"  Total number of block jumps   : {stats.num_jmps} "
    f"(ave: {stats.num_jmps_per:.2f}, max: {stats.max_num_jmps}, min: {stats.min_num_jmps})"
  )
  mlog(
    f"  Total time spent (seconds)    : {stats.time_spent:.2f} (ave: {stats.time_spent_per:.2f})"
  )
  mlog("  Basic blocks distribution     :")
  for bbl, count in stats.bbl_dist.most_common(10):
    mlog(f"    {bbl:5d} basic blocks          : {count:5d} ({count / stats.num_graphs * 100:.2f}%)")
  mlog("  Block jumps distribution      :")
  for jmp, count in stats.jmp_dist.most_common(10):
    mlog(f"    {jmp:5d} block jumps           : {count:5d} ({count / stats.num_graphs * 100:.2f}%)")


def main():
  global \
    GLOBAL_OPTION_MIN_CFG_BLOCKS, \
    GLOBAL_OPTION_MAX_CFG_BLOCKS, \
    GLOBAL_OPTION_MIN_CFG_JUMPS, \
    GLOBAL_OPTION_MAX_CFG_JUMPS

  parser = ArgumentParser("gen", description="Generate a graph database from a set of functions")
  parser.add_argument(
    "-l",
    "--llvm",
    default="/usr",
    type=str,
    help="Path to the LLVM installation directory (default: /usr)",
  )
  parser.add_argument(
    "-L",
    "--limit",
    default=MAX_I32,
    type=int,
    help=f"Limit the number of graphs to generate, non-positive meaning {MAX_I32} (default: {MAX_I32})",
  )
  parser.add_argument(
    "-g",
    "--generator",
    default="yarpgen",
    type=str,
    choices=SUPPORTED_PROGRAM_GENERATORS,
    help="The program generator to use (default: yarpgen)",
  )
  parser.add_argument(
    "-s",
    "--seed",
    type=int,
    default=-1,
    help="Seed for the randomness of the generation process, negative meaning no seed (default: -1)",
  )
  parser.add_argument(
    "--csmith",
    default=None,
    type=str,
    help="Path to the csmith binary. This is required when "
    "csmith is chosen as the program generator (default: None, not used)",
  )
  parser.add_argument(
    "--yarpgen",
    default=None,
    type=str,
    help="Path to the yarpgen binary. This is required when "
    "yarpgen is chosen as the program generator (default: None, not used)",
  )
  parser.add_argument(
    "--preset",
    default=None,
    type=str,
    help="Path to the compile_commands.json saving the compile commands of a preset of programs. "
    "The compile_commands.json is a widely used format for storing compilation commands. It can be "
    "generated it using CMake with the option -DCMAKE_EXPORT_COMPILE_COMMANDS=ON, or tools like Bear. "
    "Note that, each command in the compile_commands.json should be a valid command to compile a "
    "C/C++ program (with an explicit -o option) using either GCC or Clang. "
    "We did not rely on the 'arguments' field of the compile_commands.json as it is not always available. "
    "See: https://clang.llvm.org/docs/JSONCompilationDatabase.html."
    "This is required when preset is chosen as the program generator (default: None, not used)",
  )
  parser.add_argument(
    "--min-blocks",
    default=GLOBAL_OPTION_MIN_CFG_BLOCKS,
    type=int,
    help="Minimum number of basic blocks (nodes) in the generated control flow graphs. This is used to "
    f"filter out small graphs that are not useful for testing (default: {GLOBAL_OPTION_MIN_CFG_BLOCKS})",
  )
  parser.add_argument(
    "--max-blocks",
    default=GLOBAL_OPTION_MAX_CFG_BLOCKS,
    type=int,
    help="Maximum number of basic blocks (nodes) in the generated control flow graphs. This is used to "
    f"filter out overly large graphs that are not harmful for testing (default: {GLOBAL_OPTION_MAX_CFG_BLOCKS})",
  )
  parser.add_argument(
    "--min-jumps",
    default=GLOBAL_OPTION_MIN_CFG_JUMPS,
    type=int,
    help="Minimum number of block jumps (edges) in the generated control flow graphs. This is used to "
    f"filter out small graphs that are not useful for testing (default: {GLOBAL_OPTION_MIN_CFG_JUMPS})",
  )
  parser.add_argument(
    "--max-jumps",
    default=GLOBAL_OPTION_MAX_CFG_JUMPS,
    type=int,
    help="Maximum number of block jumps (edges) in the generated control flow graphs. This is used to "
    f"filter out overly large graphs that are not harmful for testing (default: {GLOBAL_OPTION_MAX_CFG_JUMPS})",
  )
  parser.add_argument("output", type=str, help="Path to the output file (.jsonl)")

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
      merror("Option --csmith must be specified when using csmith as the program generator.")
    gen = Csmith(Path(args.csmith), llvm)
  elif gen == "yarpgen":
    if args.yarpgen is None:
      merror("Option --yarpgen must be specified when using YARPGen as the program generator.")
    gen = YARPGen(Path(args.yarpgen), llvm)
  elif gen == "preset":
    if args.preset is None:
      merror("Option --preset must be specified when using Preset as the program generator.")
    gen = Preset(Path(args.preset), llvm)
  else:
    merror(f"Unsupported program generator: {gen}")

  limit = args.limit
  if limit <= 0:
    limit = MAX_I32
  if limit > MAX_I32:
    mwarning(f"Limit (--limit) exceeds maximum allowed value ({MAX_I32}). Setting to {MAX_I32}.")
    limit = MAX_I32

  GLOBAL_OPTION_MIN_CFG_BLOCKS = args.min_blocks
  if GLOBAL_OPTION_MIN_CFG_BLOCKS <= 0:
    GLOBAL_OPTION_MIN_CFG_BLOCKS = 0
  if GLOBAL_OPTION_MIN_CFG_BLOCKS > MAX_I32:
    mwarning(
      f"Minimum blocks (--min-blocks) exceeds maximum allowed value ({MAX_I32}). Setting to {MAX_I32}."
    )
    GLOBAL_OPTION_MIN_CFG_BLOCKS = MAX_I32

  GLOBAL_OPTION_MAX_CFG_BLOCKS = args.max_blocks
  if GLOBAL_OPTION_MAX_CFG_BLOCKS <= 0:
    GLOBAL_OPTION_MAX_CFG_BLOCKS = 0
  if GLOBAL_OPTION_MAX_CFG_BLOCKS > MAX_I32:
    mwarning(
      f"Maximum blocks (--max-blocks) exceeds maximum allowed value ({MAX_I32}). Setting to {MAX_I32}."
    )
    GLOBAL_OPTION_MAX_CFG_BLOCKS = MAX_I32

  GLOBAL_OPTION_MIN_CFG_JUMPS = args.min_jumps
  if GLOBAL_OPTION_MIN_CFG_JUMPS <= 0:
    GLOBAL_OPTION_MIN_CFG_JUMPS = 0
  if GLOBAL_OPTION_MIN_CFG_JUMPS > MAX_I32:
    mwarning(
      f"Minimum jumps (--min-jumps) exceeds maximum allowed value ({MAX_I32}). Setting to {MAX_I32}."
    )
    GLOBAL_OPTION_MIN_CFG_JUMPS = MAX_I32

  GLOBAL_OPTION_MAX_CFG_JUMPS = args.max_jumps
  if GLOBAL_OPTION_MAX_CFG_JUMPS <= 0:
    GLOBAL_OPTION_MAX_CFG_JUMPS = 0
  if GLOBAL_OPTION_MAX_CFG_JUMPS > MAX_I32:
    mwarning(
      f"Minimum jumps (--min-jumps) exceeds maximum allowed value ({MAX_I32}). Setting to {MAX_I32}."
    )
    GLOBAL_OPTION_MAX_CFG_JUMPS = MAX_I32

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
      color="yellow",
      end="",
      flush=True,
    )
    yes = input()
    if yes != "Y":
      mlog("The user chose not to overwrite the output directory. Exiting.", color="red")
      exit(0)
    output.unlink()

  run_gen_loop(
    gid=str(uuid()).replace("-", "_"),
    pgen=gen,
    llvm=llvm,
    limit=limit,
    output=output,
    timeout=15,  # Default timeout, can be adjusted
  )


if __name__ == "__main__":
  main()
