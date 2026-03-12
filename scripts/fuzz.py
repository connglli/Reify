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

import datetime
import enum
import json
import random
import re
import shlex
import shutil
import signal
import sys
import time
from argparse import ArgumentParser
from collections import namedtuple
from dataclasses import dataclass
from multiprocessing import Manager, Pool, Queue
from multiprocessing.synchronize import Event
from pathlib import Path
from queue import Empty as QueueIsEmpty
from subprocess import PIPE, STDOUT, CalledProcessError, TimeoutExpired
from threading import Thread
from typing import List, Optional, Tuple

import cmdline
import configs

# -==========================================================
# Function and program generation
# -==========================================================

# Configuration tuple for function generation
FuncGenConfig = namedtuple(
  "FuncGenConfig",
  [
    "bbls",  # Number of basic blocks per function
    "vars",  # Number of variables per basic block
    "assn",  # Number of assignments per basic block
    "tera",  # Number of terms per assignment per function
    "terc",  # Number of terms per condition per function
  ],
)

# List of suggested function generation configurations
FGEN_SUGGESTED_CONFIGS: List[FuncGenConfig] = [
  FuncGenConfig(15, 8, 2, 2, 3),
  FuncGenConfig(15, 8, 2, 2, 4),
  FuncGenConfig(15, 8, 3, 2, 4),
  FuncGenConfig(10, 8, 2, 2, 4),
  FuncGenConfig(8, 10, 2, 2, 3),
  FuncGenConfig(11, 8, 2, 2, 4),
  FuncGenConfig(10, 12, 2, 2, 4),
  FuncGenConfig(15, 10, 2, 2, 2),
  FuncGenConfig(10, 8, 3, 2, 4),
  FuncGenConfig(15, 8, 4, 2, 3),
  FuncGenConfig(10, 8, 1, 2, 4),
]


@dataclass
class FuncGenOptions:
  bin: str  # Path to the rysmith executable
  uuid: str  # Primary ID for the newly generated function
  sno: int  # Secondary ID for the newly generated function
  outdir: Path  # Directory to store the generated function files
  config: FuncGenConfig  # Configuration for the function generation
  verbose: bool = False  # Whether to print verbose output
  main: bool = True  # Whether to include a main function in the generated program
  sexp: bool = True  # Whether to include S-expression output
  allops: bool = True  # Whether to consider all possible operations in the generated program
  injubs: bool = True  # Whether to inject undefined behaviors for those unexecuted blocks
  seed: Optional[int] = None  # Seed for the random number generator (None means no seed)
  extra: Optional[str] = None  # Extra options to control the function generation process

  def to_dict(self) -> dict:
    return {
      "bin": self.bin,
      "uuid": self.uuid,
      "sno": self.sno,
      "outdir": str(self.outdir),
      "config": tuple(self.config),
      "verbose": self.verbose,
      "main": self.main,
      "sexp": self.sexp,
      "allops": self.allops,
      "injubs": self.injubs,
      "seed": self.seed,
      "extra": self.extra,
    }


def generate_function(
  opts: FuncGenOptions, timeout: int
) -> Tuple[Optional[configs.FunArts], Optional[str]]:
  arts = configs.FunArts(opts.uuid, opts.sno, gen_dir=opts.outdir)
  try:
    cmd = [
      opts.bin,
      "--Xnum-bbls-per-fun",
      str(opts.config.bbls),
      "--Xnum-vars-per-fun",
      str(opts.config.vars),
      "--Xnum-assigns-per-bbl",
      str(opts.config.assn),
      "--Xnum-vars-per-assign",
      str(opts.config.tera),
      "--Xnum-vars-in-cond",
      str(opts.config.terc),
    ]
    if opts.main:
      cmd += ["-m"]
    if opts.sexp:
      cmd += ["-S"]
    if opts.allops:
      cmd += ["-A"]
    if opts.injubs:
      cmd += ["-U"]
    if opts.verbose:
      cmd += ["-v"]
    if opts.seed:
      cmd += ["-s", str(opts.seed)]
    if opts.extra:
      cmd += shlex.split(opts.extra)
    cmd += ["-o", str(opts.outdir), "-n", str(opts.sno), opts.uuid]
    cmdline.check_out(cmd, timeout=timeout)
    with arts.get_func_file().open("a") as fout:
      fout.write(f"\n\n// rysmith options: {' '.join(cmd)}")
    result = arts, None
    if not configs.FunArts.is_test_dir(arts.get_test_dir()):
      result = (
        None,
        "generation failed: generated tests are uin valid (missing files)",
      )
  except CalledProcessError as e:
    result = None, (f"exitcode: {e.returncode}; message: {e.stdout or '<no output>'}")
  except TimeoutExpired:
    result = None, f"timeout"
  except Exception as e:
    result = None, f"unexpected error: {e}"
  if result[0] is None:
    shutil.rmtree(arts.get_test_dir(), ignore_errors=True)
  return result


# Configuration tuple for function generation
ProgGenConfig = namedtuple(
  "ProgGenConfig",
  [
    "funs",  # Number of functions per program
  ],
)

# List of suggested program generation configurations
PGEN_SUGGESTED_CONFIGS: List[ProgGenConfig] = [ProgGenConfig(n) for n in range(3, 15)]


@dataclass
class ProgGenOptions:
  bin: str  # Path to the rylink executable
  uuid: str  # Primary ID for the newly generated program
  indir: Path  # Directory to read the input function files
  limit: int  # The maximum number of programs to generate (0 means unlimited)
  config: ProgGenConfig  # Configuration for the program generation
  seed: Optional[int] = None  # Seed for the random number generator (None means no seed)
  debug = False  # Whether to print debug information
  extra: Optional[str] = None  # Extra options to control the program generation process

  def to_dict(self) -> dict:
    return {
      "bin": self.bin,
      "uuid": self.uuid,
      "indir": str(self.indir),
      "limit": self.limit,
      "config": tuple(self.config),
      "seed": self.seed,
      "debug": self.debug,
      "extra": self.extra,
    }


def generate_programs(
  opts: ProgGenOptions,
) -> Tuple[Optional[List[configs.ProgArts]], Optional[str]]:
  try:
    cmd = [
      opts.bin,
      "-i",
      str(opts.indir),
      "-l",
      str(opts.limit),
      "--Xfunction-depth",
      str(opts.config.funs),
    ]
    if opts.seed:
      cmd += ["-s", str(opts.seed)]
    if opts.debug:
      cmd += ["--debug"]
    if opts.extra:
      cmd += shlex.split(opts.extra)
    cmd += [opts.uuid]
    cmdline.check_out(cmd)
    arts = []
    for test_dir in opts.indir.iterdir():
      if opts.uuid not in test_dir.name:
        continue  # Not our generated program
      if not configs.ProgArts.is_test_dir(test_dir):
        shutil.rmtree(test_dir, ignore_errors=True)
        continue  # Not a valid program test dir
      arts.append(configs.ProgArts.from_test_dir(test_dir, gen_dir=opts.indir))
      with arts[-1].get_main_file().open("a") as fout:
        fout.write(f"\n\n// Pgen Options: {' '.join(cmd)}")
    if arts:
      return arts, None
    else:
      return None, "generation failed: no valid programs are generated"
  except CalledProcessError as e:
    return None, f"exit with {e.returncode}, message: {e.stdout or '<no output>'}"
  except Exception as e:
    return None, f"unexpected error: {e}"


# -==========================================================
# Logging and utility functions
# -==========================================================


MAX_I32 = 2147483647


def rand_int(a: int = 0, b=MAX_I32) -> int:
  return random.randint(a, b)


def next_uuid():
  keywords = "0123456789abcdefghijklmnopqrstuvwxyz"
  return "".join(random.choices(keywords, k=6))


def log(
  title: str,
  msg: str,
  *,
  color: Optional[str] = None,
  end: str = "\n",
  flush: bool = False,
  file=sys.stderr,
):
  enable = {
    "red": "\033[91m",
    "green": "\033[92m",
    "yellow": "\033[93m",
    "blue": "\033[94m",
    "magenta": "\033[95m",
    "cyan": "\033[96m",
    "white": "\033[97m",
  }.get(color, "")
  disable = "\033[0m" if enable else ""
  print(
    f"{enable}[{datetime.datetime.now().strftime('%Y-%m-%d::%H:%M:%S')}::{title}] {msg}{disable}",
    end=end,
    flush=flush,
    file=file,
  )


def mlog(msg, *, color: Optional[str] = None, end: str = "\n", flush: bool = False):
  log("Main", msg=msg, color=color, end=end, flush=flush)


# -==========================================================
# Compiler testing and result handling
# -==========================================================


GCC_CFLAGS_POOL = [
  "-fno-strict-aliasing",
  "-fstrict-aliasing",
  "-fstrict-overflow",
  "-fno-aggressive-loop-optimizations",
  "-ftree-vectorize",
  "-ftree-slp-vectorize",
  "-funroll-loops",
  "-fpeel-loops",
  "-finline-limit=1000",
  "-fno-builtin",
  "-fno-common",
  "-fipa-pta",
  "-fdevirtualize-at-ltrans",
  "-fsched-pressure",
  "-fsched-spec-load",
  "-fselective-scheduling",
  "-fselective-scheduling2",
  "-fsel-sched-pipelining",
  "-fsel-sched-pipelining-outer-loops",
  "-ftree-loop-distribution",
  "-ftree-loop-distribute-patterns",
  "-ftree-loop-im",
  "-ftree-loop-ivcanon",
  "-floop-nest-optimize",
  "-floop-parallelize-all",
  "-ftree-parallelize-loops=4",
  "-fvect-cost-model=unlimited",
  "-fsimd-cost-model=unlimited",
  "-ffast-math",
  "-fno-finite-math-only",
  "-fno-trapping-math",
  "-fno-signed-zeros",
  "-fno-associative-math",
  "-freciprocal-math",
  "-fno-rounding-math",
  "-fno-signaling-nans",
  "-fcx-limited-range",
  "-fcx-fortran-rules",
  "-fipa-icf",
  "-fno-plt",
  "-fno-semantic-interposition",
  "-fira-algorithm=priority",
  "-fira-region=all",
  "-fmodulo-sched",
  "-fmodulo-sched-allow-regmoves",
  "-fpartial-inlining",
  "-fpredictive-commoning",
  "-fsplit-loops",
  "-fsplit-paths",
  "-ftree-partial-pre",
  "-funswitch-loops",
  "-fversion-loops-for-strides",
]

CLANG_CFLAGS_POOL = [
  "-fstrict-aliasing",
  "-fno-strict-aliasing",
  "-fvectorize",
  "-fslp-vectorize",
  "-funroll-loops",
  "-fno-builtin",
  "-fno-common",
  "-ffast-math",
  "-fno-trapping-math",
  "-mllvm -force-vector-width=2",
  "-mllvm -force-vector-width=4",
  "-mllvm -force-vector-width=8",
  "-mllvm -force-vector-interleave=2",
  "-mllvm -enable-loop-distribute",
  "-mllvm -enable-loop-versioning-licm",
  "-mllvm -enable-load-pre",
  "-mllvm -aggressive-instcombine",
  "-mllvm -scalar-evolution-max-arith-depth=32",
  "-mllvm -reassociate",
  "-mllvm -enable-gvn-hoist",
  "-mllvm -enable-gvn-sink",
  "-mllvm -enable-loop-flatten",
  "-mllvm -enable-interprocedural-devirtualization",
  "-mllvm -enable-dfa-jump-threading",
  "-mllvm -enable-loop-versioning",
  "-mllvm -enable-cond-hoisting",
  "-mllvm -enable-loop-simplifycfg-term-folding",
  "-mllvm -enable-newgvn",
  "-mllvm -enable-partial-inlining",
  "-mllvm -enable-scalarizer",
  "-mllvm -inline-threshold=500",
]


EXITCODE_TIMEOUT = 100001


@dataclass
class TestRes:
  @enum.unique
  class Type(enum.Enum):
    SUC = "success"
    ICE = "internal_compiler_error"
    WRC = "wrong_code"
    CTO = "compilation_timeout"
    ETO = "execution_timeout"
    IVC = "invalid_compiler_command"

  cmd: str
  type: Type
  exitcode: int
  errmsg: Optional[str] = None

  def is_success(self) -> bool:
    return self.type == TestRes.Type.SUC

  def is_internal_compiler_error(self) -> bool:
    return self.type == TestRes.Type.ICE

  def is_wrong_code(self) -> bool:
    return self.type == TestRes.Type.WRC

  def is_compilation_timeout(self) -> bool:
    return self.type == TestRes.Type.CTO

  def is_execution_timeout(self) -> bool:
    return self.type == TestRes.Type.ETO

  def is_invalid_compiler_command(self) -> bool:
    return self.type == TestRes.Type.IVC

  def exitcode_str(self) -> str:
    if self.exitcode == 0:
      return "Success"
    elif self.exitcode == EXITCODE_TIMEOUT:
      return "Timeout"
    elif self.exitcode > 0:
      return "Unknown"
    else:
      try:
        return f"{signal.Signals(-self.exitcode).name} ({signal.strsignal(-self.exitcode)})"
      except ValueError:
        return "Unknown"

  def to_dict(self) -> dict:
    return {
      "cmd": self.cmd,
      "type": self.type.value,
      "exitcode": self.exitcode,
      "exitcode_str": self.exitcode_str(),
      "error_msg": self.errmsg or "<no-error-message>",
      "timestamp": datetime.datetime.now().isoformat(),
    }


def test_compiler(cc: str, test_dir: Path, out_file: Path, timeout: int) -> TestRes:
  """
  Test the given compiler command `cc` on the C files in `test_dir`, outputting the binary.
  This assumes that only one main() function exists in the C files under `test_dir`.
  """
  comp_cmd = (
    f"{cc} "
    f"-I {test_dir} "  # includes
    f"-o {out_file} "  # output binary
    f"{' '.join([str(s) for s in test_dir.iterdir() if s.is_file() and s.suffix == '.c'])}"  # sources
  )
  try:
    proc = cmdline.run_proc(
      shlex.split(comp_cmd),
      stdout=PIPE,
      stderr=STDOUT,
      timeout=timeout * 10,
      check=False,
    )
  except FileNotFoundError as e:
    return TestRes(comp_cmd, TestRes.Type.ICE, exitcode=-1, errmsg=str(e))
  except TimeoutExpired as e:
    return TestRes(comp_cmd, TestRes.Type.CTO, exitcode=EXITCODE_TIMEOUT, errmsg=str(e))
  if proc.returncode != 0:
    return TestRes(comp_cmd, TestRes.Type.ICE, exitcode=-proc.returncode, errmsg=proc.stdout)
  exec_cmd = str(out_file)
  full_cmd = f"{comp_cmd}; {exec_cmd}"
  try:
    proc = cmdline.run_proc(
      shlex.split(exec_cmd),
      stdout=PIPE,
      stderr=STDOUT,
      timeout=timeout * 20,
      check=False,
    )
  except TimeoutExpired as e:
    return TestRes(full_cmd, TestRes.Type.ETO, exitcode=EXITCODE_TIMEOUT, errmsg=str(e))
  if proc.returncode != 0:
    return TestRes(full_cmd, TestRes.Type.WRC, exitcode=proc.returncode, errmsg=proc.stdout)
  return TestRes(full_cmd, TestRes.Type.SUC, exitcode=0, errmsg=proc.stdout)


def test_compiler_diffopt(cc: str, test_dir: Path, out_file: Path, timeout: int) -> TestRes:
  """
  Perform differential testing on the given compiler command `cc` with different optimization levels.
  If the user specifies an optimization level, it compares it with the most different one (typically O0).
  Otherwise, it compares -O0 vs -O3.
  """
  # Detect existing optimization flag in the compiler command
  opt_match = re.search(r"-O([0123sz]|fast)", cc)
  if opt_match:
    opt_user = opt_match.group(0)
    # If user provided -O0, compare with -O3; otherwise compare with -O0
    opt_diff = {
      "-O0": "-O3",
      "-O1": "-O0",
      "-O2": "-O0",
      "-O3": "-O0",
      "-Os": "-O3",
      "-Oz": "-O3",
      "-Ofast": "-O0",
    }.get(opt_user, "-O0")
    cc1, label1 = cc, opt_user
    cc2, label2 = cc.replace(opt_user, opt_diff), opt_diff
  else:
    cc1, label1 = f"{cc} -O0", "-O0"
    cc2, label2 = f"{cc} -O3", "-O3"

  # Test with the first optimization level
  res1 = test_compiler(cc1, test_dir, out_file.with_suffix(f".{label1[1:]}"), timeout)
  if not res1.is_success():
    return res1

  # Test with the second optimization level
  res2 = test_compiler(cc2, test_dir, out_file.with_suffix(f".{label2[1:]}"), timeout)
  if not res2.is_success():
    return res2

  # Compare the results
  if res1.exitcode != res2.exitcode or res1.errmsg != res2.errmsg:
    return TestRes(
      f"{res1.cmd}\n{res2.cmd}",
      TestRes.Type.WRC,
      exitcode=res2.exitcode,
      errmsg=(
        f"Differential testing failed: {label1} and {label2} produced different results\n"
        f"[{label1}] exitcode={res1.exitcode}, output={res1.errmsg}\n"
        f"[{label2}] exitcode={res2.exitcode}, output={res2.errmsg}"
      ),
    )

  return TestRes(f"{res1.cmd}\n{res2.cmd}", TestRes.Type.SUC, exitcode=0, errmsg=res1.errmsg)


# -==========================================================
# Fuzzing worker to generate programs and test compilers
# -==========================================================


@dataclass
class WorkerConf:
  cc: str  # Compiler command to use (e.g., 'gcc -O3 -fno-tree-slsr', 'clang')
  wdir: Path  # Working directory for the worker
  switch_limit: int  # Number of functions before switching to program generation
  prog_limit: int  # Limit for the number of generated programs per switch
  diff_opt: bool = False  # Whether to enable differential testing over optimization levels
  cflag_pool: Optional[List[str]] = None  # Pool of compiler flags for swarm testing


@dataclass
class WorkerRunOptions:
  limit: int  # Maximum number of iterations for the worker
  stop: Event  # Synchronizing event to stop the worker
  gen_tmo: int = 3  # Timeout (seconds) for function/program generation
  test_tmo: int = 10  # Timeout (seconds) for testing the compiler with the generated program


class Worker:
  def __init__(self, wid: int, *, wconf: WorkerConf, msgq: Queue):
    self.wid = wid
    self.wconf = wconf
    self.msgq = msgq
    self.ice_dir = wconf.wdir / "intern_errors"
    self.hang_dir = wconf.wdir / "compiler_hangs"
    self.wrc_dir = wconf.wdir / "wrong_codes"
    self.log_file = wconf.wdir / "worker.log"
    self.log_file_fd = self.log_file.open(mode="w", encoding="utf-8")
    self.ice_dir.mkdir(parents=True, exist_ok=True)
    self.hang_dir.mkdir(parents=True, exist_ok=True)
    self.wrc_dir.mkdir(parents=True, exist_ok=True)
    self.iter = 0

  def close(self):
    if not self.log_file_fd.closed:
      self.log_file_fd.close()

  def run(self, ropts: WorkerRunOptions):
    fopts = FuncGenOptions(
      bin="./build/bin/rysmith",
      uuid=next_uuid(),
      sno=0,
      outdir=self.wconf.wdir,
      config=FGEN_SUGGESTED_CONFIGS[0],
      extra="--Xinject-ub-proba 0.1",
    )
    popts = ProgGenOptions(
      bin="./build/bin/rylink",
      uuid="<placeholder>",
      indir=self.wconf.wdir,
      limit=self.wconf.prog_limit,
      config=PGEN_SUGGESTED_CONFIGS[0],
      extra="--Xreplace-proba 0.4",
    )
    start_msg = (
      f"Worker started successfully: workdir={self.wconf.wdir}, "
      f"iter_limit={ropts.limit}, switch_limit={self.wconf.switch_limit}, "
      f"prog_limit={self.wconf.prog_limit}, "
      f"gen_tmo={ropts.gen_tmo}s, test_tmo={ropts.test_tmo}s"
    )
    self.log(start_msg)
    self.notify(start_msg)
    self.iter = 0
    while self.iter < ropts.limit and not ropts.stop.is_set():
      fopts.seed = rand_int()
      fopts.config = random.choice(FGEN_SUGGESTED_CONFIGS)
      if self.run_func(fopts, ropts.gen_tmo, ropts.test_tmo) is not None:
        fopts.sno += 1
      if fopts.sno == self.wconf.switch_limit:
        self.log("Switched to program generation", color="yellow")
        popts.uuid = next_uuid()
        popts.seed = rand_int()
        popts.config = random.choice(PGEN_SUGGESTED_CONFIGS)
        self.run_prog(popts, ropts.test_tmo)
        self.log("Removing useless functions and programs")
        self.remove_useless_funcs(fopts)
        self.remove_useless_progs(popts)
        self.log("Switched back to function generation", color="yellow")
        fopts.uuid = next_uuid()
        fopts.sno = 0
      self.iter += 1
    self.remove_useless_funcs(fopts)

  def run_func(self, opts: FuncGenOptions, gen_tmo: int, test_tmo: int) -> bool:
    self.log(
      f"Generating function: {', '.join([str(x[0]) + '=' + str(x[1]) for x in opts.to_dict().items()])}"
    )
    arts, errmsg = generate_function(opts, timeout=gen_tmo)
    if not arts:
      # TODO: Save it as it might be our bugs
      self.log(f"Failure: {errmsg}", color="yellow")
      return False  # No functions are generated
    self.log(f"Generated function: {arts.get_test_dir()}")
    self.log(f"Testing the compiler with the generated function: {arts.get_test_dir()}")
    binary = arts.get_test_dir() / "main.out"
    self.test(arts.get_test_dir(), binary=binary, timeout=test_tmo)
    return True  # Generated and tested

  def run_prog(self, opts: ProgGenOptions, test_tmo: int):
    self.log(
      f"Generating programs: {', '.join([str(x[0]) + '=' + str(x[1]) for x in opts.to_dict().items()])}"
    )
    all_arts, errmsg = generate_programs(opts)
    if not all_arts:
      self.log(f"Failure: {errmsg}", color="yellow")
      return  # All program generation failed
    self.log("Testing the compiler with the generated programs")
    for index, arts in enumerate(all_arts):
      self.log(f"{index}: Testing the compiler with the generated program: {arts.get_test_dir()}")
      binary = arts.get_test_dir() / "main.out"
      self.test(arts.get_test_dir(), binary=binary, timeout=test_tmo)

  def remove_useless_funcs(self, opts: FuncGenOptions):
    for i in range(opts.sno + 1):
      shutil.rmtree(
        configs.FunArts(opts.uuid, i, gen_dir=opts.outdir).get_test_dir(),
        ignore_errors=True,
      )

  def remove_useless_progs(self, opts: ProgGenOptions):
    for i in range(opts.limit + 1):
      shutil.rmtree(
        configs.ProgArts(opts.uuid, i, gen_dir=opts.indir).get_test_dir(),
        ignore_errors=True,
      )

  def test(self, test_dir: Path, *, binary: Path, timeout: int):
    extra_cc_opts = ""
    if self.wconf.cflag_pool:
      num_flags = random.randint(1, 5)
      picked = random.sample(self.wconf.cflag_pool, min(num_flags, len(self.wconf.cflag_pool)))
      # Only add the picked flags that are not already in the compiler command
      extra_cc_opts = " ".join([s for s in picked if s not in self.wconf.cc]).strip()
    self.log(f"Testing compiler: {self.wconf.cc} {extra_cc_opts}")
    if self.wconf.diff_opt:
      self.log("Using differential testing over optimization levels")
      test_res = test_compiler_diffopt(
        f"{self.wconf.cc} {extra_cc_opts}",
        test_dir=test_dir,
        out_file=binary,
        timeout=timeout,
      )
    else:
      test_res = test_compiler(
        f"{self.wconf.cc} {extra_cc_opts}",
        test_dir=test_dir,
        out_file=binary,
        timeout=timeout,
      )
    if test_res.is_internal_compiler_error():
      self.log(
        f"INTERNAL COMPILER ERROR (exitcode={test_res.exitcode}): {test_res.errmsg}",
        color="green",
      )
      self.store_bug(test_res, test_dir, self.ice_dir)
    elif test_res.is_compilation_timeout():
      self.log(f"COMPILER HANG (exitcode={test_res.exitcode}): {test_res.errmsg}", color="blue")
      self.store_bug(test_res, test_dir, self.hang_dir)
    elif test_res.is_wrong_code():
      self.log(f"WRONG CODE (exitcode={test_res.exitcode}): {test_res.errmsg}", color="green")
      self.store_bug(test_res, test_dir, self.wrc_dir)
    elif test_res.is_execution_timeout():
      self.log(f"The generated program timed out (skip): {test_res.errmsg}")
      shutil.rmtree(test_dir, ignore_errors=True)
    elif test_res.is_success():
      self.log("Compiler passed the test")
    elif test_res.is_invalid_compiler_command():
      self.log(
        f"INVALID COMPILER COMMAND (exitcode={test_res.exitcode}): {test_res.errmsg}",
        color="red",
      )
      raise RuntimeError(f"Invalid compiler command: {test_res.errmsg}")
    else:
      raise RuntimeError(
        f"Unknown test result: type={test_res.type.name}, exitcode={test_res.exitcode}, errmsg={test_res.errmsg}"
      )

  def store_bug(self, res: TestRes, test_dir: Path, bug_dir: Path):
    shutil.move(str(test_dir), str(bug_dir / test_dir.name))
    with (bug_dir / "result.jsonl").open("a") as fou:
      fou.write(json.dumps(res.to_dict(), ensure_ascii=False) + "\n")

  def notify(self, msg: str):
    self.msgq.put(f"Worker@{self.wid}: {msg}")

  def log(self, msg: str, *, color: Optional[str] = None, end: str = "\n"):
    log(
      f"Worker@{self.wid}",
      msg=f"[{self.iter}] " + msg,
      file=self.log_file_fd,
      color=color,
      end=end,
      flush=True,
    )


# -==========================================================
# Fuzzing loop and worker management
# -==========================================================


def run_worker(*, wid: int, wconf: WorkerConf, ropts: WorkerRunOptions, seed: int, msgq: Queue):
  wconf.wdir.mkdir(parents=False, exist_ok=False)
  random.seed(seed)
  worker = Worker(wid, wconf=wconf, msgq=msgq)
  try:
    worker.run(ropts)
    worker.notify("Worker exited normally")
  except KeyboardInterrupt:
    worker.notify("Worker interrupted by user's Ctrl-C")
    worker.log("Worker interrupted by user's Ctrl-C", color="red")
  except Exception as e:
    worker.notify(f"Worker interrupted by exception: {e}")
    worker.log(f"Worker interrupted by exception: {e}", color="red")
  finally:
    worker.close()


def run_msgq(msgq: Queue, stop: Event):
  while not stop.is_set():
    try:
      msg = msgq.get(timeout=15)  # seconds
    except QueueIsEmpty:
      continue
    mlog(msg)
  mlog("Waiting for remaining messages to come ...", color="yellow")
  time.sleep(5)  # Give some time for the last messages to be processed
  while not msgq.empty():
    msg = msgq.get(timeout=0.5)
    mlog(msg)


def run_fuzz_main(
  cc: str,
  *,
  outdir: Path,
  workers: int,
  ilimit: int,
  slimit: int,
  plimit: int,
  diff_opt: bool = False,
  cflag_pool: Optional[List[str]] = None,
):
  class SignalInterrupt(Exception):
    def __init__(self, sig):
      super().__init__(f"Process killed by user signal: {sig}")
      self.sig = sig

  # Register to catch SIGTERM signal. Note, SIGKILL cannot be caught.
  def abort_by_signals(sig, _):
    raise SignalInterrupt(sig)

  signal.signal(signal.SIGTERM, abort_by_signals)
  mlog("Registered signal handler for SIGTERM")

  mlog("Start fuzzing process ...")
  manager = Manager()
  stop = manager.Event()  # An event to notify all workers to stop
  msgq = manager.Queue(maxsize=workers + 1)  # A message queue to notify any events
  msgq_thread = Thread(target=run_msgq, args=(msgq, stop))
  msgq_thread.start()
  worker_pool = Pool(workers)
  results = {}
  for wid in range(workers):
    wdir = outdir / f"fuzz_proc_{wid:05}"  # Working directory for each worker
    seed = rand_int()  # Random seed for each worker
    mlog(
      f"Requesting worker: "
      f"wid={wid}, workdir={wdir}, seed={seed}, "
      f"iter_limit={ilimit}, prog_limit={plimit}, switch_limit={slimit}, "
      f"compiler=`{cc}`"
    )
    results[wid] = worker_pool.apply_async(
      run_worker,
      kwds={
        "wid": wid,
        "wconf": WorkerConf(
          cc=cc,
          wdir=wdir,
          switch_limit=slimit,
          prog_limit=plimit,
          diff_opt=diff_opt,
          cflag_pool=cflag_pool,
        ),
        "ropts": WorkerRunOptions(
          limit=ilimit // workers,
          stop=stop,
        ),
        "seed": seed,
        "msgq": msgq,
      },
    )
  worker_pool.close()

  mlog("Press Ctrl-C to stop the fuzzing process at any time ...", color="yellow")

  try:
    worker_pool.join()
    mlog("All workers have finished their tasks successfully.")
    stop.set()
  except SignalInterrupt as e:
    mlog(f"Fuzzing process was interrupted by user signal: {e.sig.name}", color="red")
    stop.set()
  except KeyboardInterrupt:
    mlog("Fuzzing process was interrupted by user's Ctrl-C", color="red")
    stop.set()
  except Exception as e:
    mlog(f"Fuzzing process was interrupted by exception: {e}", color="red")
    stop.set()
  finally:
    mlog("Fuzzing process is shutting down ...")
    worker_pool.join()
    msgq_thread.join()

  mlog(f"Exit. All results are stored inside {outdir}.", color="green")


def main():
  import os

  parser = ArgumentParser("fuzz", description="Tool for fuzzing a specific C compiler (gcc/llvm)")

  parser.add_argument(
    "-o",
    "--outdir",
    type=str,
    default="fuzzout",
    help="Directory to store the output of the fuzzing process (default: 'fuzzout')",
  )
  parser.add_argument(
    "-s",
    "--seed",
    type=int,
    default=-1,
    help="Seed for the randomness of the fuzzing process, negative meaning no seed (default: -1)",
  )
  parser.add_argument(
    "-j",
    "--workers",
    type=int,
    default=os.cpu_count(),
    help=f"Number of workers to use for fuzzing (default: {os.cpu_count()})",
  )
  parser.add_argument(
    "-l",
    "--limit",
    type=int,
    default=MAX_I32,
    help=f"The maximum number of fuzz iterations, non-positive meaning default (default: {MAX_I32})",
  )
  parser.add_argument(
    "--switch-limit",
    type=int,
    default=100,
    help="Switch to program generation every N functions (default: 100)",
  )
  parser.add_argument(
    "--prog-limit",
    type=int,
    default=2500,
    help="The maximum number of programs to generate per switch (default: 2500)",
  )
  parser.add_argument(
    "--diff-opt",
    action="store_true",
    default=False,
    help="Enable differential testing over different optimization levels (e.g., O0 vs O3)",
  )
  parser.add_argument(
    "--swarm-cflags",
    type=str,
    choices=["gcc", "clang"],
    default=None,
    help="Enable swarm testing with a pool of compiler flags for the specified compiler type",
  )
  parser.add_argument(
    "compiler",
    type=str,
    help="Command to compile the C program (e.g., 'gcc -fsanitize=address -O0')",
  )

  args = parser.parse_args()

  seed = args.seed
  if seed >= 0:
    mlog(f"Setting seed for random number generation: {seed}")
    random.seed(seed)
  else:
    seed = time.time_ns()
    random.seed(seed)
    mlog("No seed provided, using system randomness.")

  workers = args.workers
  if workers <= 0:
    mlog(f"Error: Invalid number of workers specified: {workers}.", color="red")
    sys.exit(1)
  if workers > os.cpu_count():
    mlog(
      f"Warning: Number of workers ({workers}) exceeds available CPU cores ({os.cpu_count()}).",
      color="yellow",
    )

  limit = args.limit
  if limit <= 0:
    limit = MAX_I32
  elif limit > MAX_I32:
    mlog(
      f"Warning: Limit ({limit}) exceeds maximum allowed value ({MAX_I32}). Setting to {MAX_I32}.",
      color="yellow",
    )
    limit = MAX_I32

  switch_limit = args.switch_limit
  if switch_limit <= 0:
    mlog(
      f"Error: Invalid switch limit specified: {switch_limit}. Must be a positive number",
      color="red",
    )
    sys.exit(1)
  if switch_limit > MAX_I32:
    mlog(
      f"Warning: Switch limit ({switch_limit}) exceeds maximum allowed value ({MAX_I32}). Setting to {MAX_I32}.",
      color="yellow",
    )
    switch_limit = MAX_I32

  prog_limit = args.prog_limit
  if prog_limit <= 0:
    mlog(
      f"Error: Invalid program limit specified: {prog_limit}. Must be a positive number",
      color="red",
    )
    sys.exit(1)
  if prog_limit > MAX_I32:
    mlog(
      f"Warning: Program limit ({prog_limit}) exceeds maximum allowed value ({MAX_I32}). Setting to {MAX_I32}.",
      color="yellow",
    )
    prog_limit = MAX_I32

  if switch_limit >= prog_limit:
    mlog("Warning: Program limit is recommended to be larger than switching limit.")

  outdir = Path(args.outdir).resolve().absolute()
  if outdir.exists():
    mlog(
      "Output directory already exists. Would you like to overwrite it? (Y/n)> ",
      color="yellow",
      end="",
      flush=True,
    )
    yes = input()
    if yes != "Y":
      mlog("The user chose not to overwrite the output directory. Exiting.", color="red")
      sys.exit(0)
    shutil.rmtree(args.outdir)
  mlog(f"Creating output directory: {outdir}")
  outdir.mkdir(parents=True, exist_ok=False)

  compiler = args.compiler
  test_input = outdir / "a.c"
  test_input.write_text("int main() { return 0; }", encoding="utf-8")
  test_output = outdir / "a.out"
  mlog("Testing if the compiler command is valid ...")
  test_res = test_compiler(compiler, test_dir=outdir, out_file=test_output, timeout=1)
  test_input.unlink()
  test_output.unlink(missing_ok=True)
  if not test_res.is_success():
    mlog(f"Invalid: {test_res.errmsg}", color="red")
    sys.exit(1)
  else:
    mlog("Valid")

  # Save the command line arguments to a JSON file
  (outdir / "command.json").write_text(json.dumps({**vars(args), "seed": seed}, indent=2))

  cflag_pool = None
  if args.swarm_cflags == "gcc":
    cflag_pool = GCC_CFLAGS_POOL
  elif args.swarm_cflags == "clang":
    cflag_pool = CLANG_CFLAGS_POOL

  # Start the fuzzing loop
  run_fuzz_main(
    compiler,
    workers=workers,
    ilimit=limit,
    slimit=switch_limit,
    plimit=prog_limit,
    outdir=outdir,
    cflag_pool=cflag_pool,
    diff_opt=args.diff_opt,
  )


if __name__ == "__main__":
  main()
