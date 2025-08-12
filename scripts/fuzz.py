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
import shlex
import shutil
import signal
import sys
import time
from argparse import ArgumentParser
from dataclasses import dataclass
from multiprocessing import Manager, Pool, Queue
from multiprocessing.synchronize import Event
from pathlib import Path
from queue import Empty as QueueIsEmpty
from subprocess import TimeoutExpired, CalledProcessError, PIPE, STDOUT
from threading import Thread
from typing import Optional, Tuple, List
from uuid import uuid4 as uuidgen

import cmdline
import configs


# -==========================================================
# Function and program generation
# -==========================================================


@dataclass
class FuncGenOptions:
  bin: str  # Path to the fgen executable
  uuid: str  # Primary ID for the newly generated function
  sno: int  # Secondary ID for the newly generated function
  outdir: Path  # Directory to store the generated function files
  verbose: bool = True  # Whether to print verbose output
  main: bool = True  # Whether to include a main function in the generated program
  sexp: bool = True  # Whether to include S-expression output
  allops: bool = True  # Whether to consider all possible operations in the generated program
  injubs: bool = True  # Whether to inject undefined behaviors for those unexecuted blocks
  seed: Optional[int] = None  # Seed for the random number generator (None means no seed)
  extra: Optional[str] = None  # Extra options to control the function generation process

  def to_dict(self) -> dict:
    return {
      'bin': self.bin,
      'uuid': self.uuid,
      'sno': self.sno,
      'outdir': str(self.outdir),
      'verbose': self.verbose,
      'main': self.main,
      'sexp': self.sexp,
      'allops': self.allops,
      'injubs': self.injubs,
      'seed': self.seed,
      'extra': self.extra
    }


def generate_function(opts: FuncGenOptions, timeout: int) -> Tuple[Optional[Path], Optional[str]]:
  try:
    cmd = [opts.bin]
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
    result = configs.get_prog_file(opts.uuid, opts.sno, gen_dir=opts.outdir), None
  except CalledProcessError as e:
    result = None, (
      f"exit with {e.returncode}, message: {e.stdout or '<no output>'}"
    )
  except TimeoutExpired:
    result = None, f"generation timeout: exceeding {timeout}s"
  except Exception as e:
    result = None, f"unexpected error: {e}"
  if result[0] is None:
    for art in configs.get_artifacts(opts.uuid, opts.sno, gen_dir=opts.outdir).values():
      art.unlink(missing_ok=True)
  return result


@dataclass
class ProgGenOptions:
  bin: str  # Path to the pgen executable
  uuid: str  # Primary ID for the newly generated program
  indir: Path  # Directory to read the input function files
  limit: int  # The maximum number of programs to generate (0 means unlimited)
  seed: Optional[int] = None  # Seed for the random number generator (None means no seed)
  debug = False  # Whether to print debug information
  extra: Optional[str] = None  # Extra options to control the program generation process

  def to_dict(self) -> dict:
    return {
      'bin': self.bin,
      'uuid': self.uuid,
      'indir': str(self.indir),
      'limit': self.limit,
      'seed': self.seed,
      'debug': self.debug,
      'extra': self.extra
    }


def generate_programs(opts: ProgGenOptions) -> Optional[str]:
  try:
    cmd = [opts.bin, "-i", str(opts.indir), "-l", str(opts.limit)]
    if opts.seed:
      cmd += ["-s", str(opts.seed)]
    if opts.debug:
      cmd += ["--debug"]
    if opts.extra:
      cmd += shlex.split(opts.extra)
    cmd += [opts.uuid]
    cmdline.check_out(cmd)
    return None
  except CalledProcessError as e:
    return f"exit with {e.returncode}, message: {e.stdout or '<no output>'}"
  except Exception as e:
    return f"unexpected error: {e}"


# -==========================================================
# Logging and utility functions
# -==========================================================


MAX_I32 = 2147483647


def rand_int(a: int = 0, b=MAX_I32) -> int:
  return random.randint(a, b)


def next_uuid():
  return str(uuidgen()).replace('-', '_')


def log(title: str, msg: str, *, color: Optional[str] = None, end: str = '\n', flush: bool = False, file=sys.stderr):
  enable = {
    'red': '\033[91m',
    'green': '\033[92m',
    'yellow': '\033[93m',
    'blue': '\033[94m',
    'magenta': '\033[95m',
    'cyan': '\033[96m',
    'white': '\033[97m',
  }.get(color, '')
  disable = '\033[0m' if enable else ''
  print(f"{enable}[{datetime.datetime.now().strftime('%Y-%m-%d::%H:%M:%S')}::{title}] {msg}{disable}", end=end,
        flush=flush, file=file)


def mlog(msg, *, color: Optional[str] = None, end: str = '\n', flush: bool = False):
  log("Main", msg=msg, color=color, end=end, flush=flush)


# -==========================================================
# Compiler testing and result handling
# -==========================================================


EXITCODE_TIMEOUT = 100001


@dataclass
class TestRes:
  @enum.unique
  class Type(enum.Enum):
    SUC = 'success'
    ICE = 'internal_compiler_error'
    WRC = 'wrong_code'
    CTO = 'compilation_timeout'
    ETO = 'execution_timeout'

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
      'cmd': self.cmd,
      'type': self.type.value,
      'exitcode': self.exitcode,
      'exitcode_str': self.exitcode_str(),
      'error_msg': self.errmsg or '<no-error-message>',
      'timestamp': datetime.datetime.now().isoformat()
    }


def test_compiler(cc: str, cfile: Path, ofile: Path, timeout: int) -> TestRes:
  comp_cmd = f"{cc} -o {ofile} {cfile}"
  try:
    proc = cmdline.run_proc(shlex.split(comp_cmd), stdout=PIPE, stderr=STDOUT, timeout=timeout * 10, check=False)
  except TimeoutExpired as e:
    return TestRes(comp_cmd, TestRes.Type.CTO, exitcode=EXITCODE_TIMEOUT, errmsg=str(e))
  if proc.returncode != 0:
    return TestRes(comp_cmd, TestRes.Type.ICE, exitcode=-proc.returncode, errmsg=proc.stdout)
  exec_cmd = f"{ofile}"
  full_cmd = f"{comp_cmd}; {exec_cmd}"
  try:
    proc = cmdline.run_proc(shlex.split(exec_cmd), stdout=PIPE, stderr=STDOUT, timeout=timeout * 20, check=False)
  except TimeoutExpired as e:
    return TestRes(full_cmd, TestRes.Type.ETO, exitcode=EXITCODE_TIMEOUT, errmsg=str(e))
  if proc.returncode != 0:
    return TestRes(full_cmd, TestRes.Type.WRC, exitcode=proc.returncode, errmsg=proc.stdout)
  return TestRes(full_cmd, TestRes.Type.SUC, exitcode=0, errmsg=None)


# -==========================================================
# Creal and its utility functions
# -==========================================================


@dataclass
class CrealOptions:
  bin: str  # Path to the Creal executable
  dst: Path  # Directory to store the generated mutants
  mutants: int = 3  # The number of mutants to generate per function (default: 3)
  prob: int = 80  # Probability of synthesizing a new function (0-100, default: 80)
  src: Optional[Path] = None  # Path to the source file for generating mutants (None means Csmith)
  cdb: Optional[Path] = None  # Path to the function database for generating mutants (None means default database)
  seed: Optional[int] = None  # Seed for the random number generator (None means no seed)

  def to_dict(self):
    return {
      'bin': self.bin,
      'dst': str(self.dst),
      'mutants': self.mutants,
      'prob': self.prob,
      'src': str(self.src) if self.src else None,
      'cdb': str(self.cdb) if self.cdb else None,
      'seed': self.seed if self.seed else None,
    }


@dataclass
class CrealEnv:
  creal_home: Path  # Path to the Creal home directory
  csmith_home: Path  # Path to the Csmith home directory
  ccomp_home: Path  # Path to the CompCert home directory

  def creal_bin(self) -> Path:
    return self.creal_home / 'creal.py'

  def creal_gm_bin(self) -> Path:
    return self.creal_home / 'generate_mutants.py'

  def csmith_bin(self) -> Path:
    return self.csmith_home / 'bin' / 'csmith'

  def csmith_incl_dir(self) -> Path:
    return self.csmith_home / 'include'

  def ccomp_bin(self) -> Path:
    return self.ccomp_home / 'bin' / 'ccomp'


def is_creal_mutant(m: Path) -> bool:
  return m.is_file() and m.suffix == ".c" and "_syn" in m.stem


def run_creal(opts: CrealOptions, timeout: int) -> Tuple[Optional[List[Path]], Optional[str]]:
  cmd = [opts.bin, "--dst", str(opts.dst), "--syn-prob", str(opts.prob), "--num-mutants", str(opts.mutants)]
  if opts.src:
    cmd += ["--seed", str(opts.src)]
  if opts.cdb:
    cmd += ["--func-db", str(opts.cdb)]
  if opts.seed:
    cmd += ["--rand-seed", str(opts.seed)]
  try:
    cmdline.check_out(cmd, timeout=timeout)
    return [m for m in opts.dst.iterdir() if is_creal_mutant(m)], None
  except TimeoutExpired as e:
    return None, f"Creal generation timed out: {e}"
  except CalledProcessError as e:
    return None, f"Creal generation failed with exit code {e.returncode}: {e.stdout or '<no output>'}"
  except Exception as e:
    return None, f"Unexpected error during Creal generation: {e}"



CREALIZE_CHK_SIGN = "exit(101)";
CREALIZE_TEMPLATE = """
int {chkchk_name}(int size, int args[]) {{
  int outs[{num_maps}][{num_params}] = {{
    {outputs}
  }};
  int chks[{num_maps}] = {{
    {checksums}
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
  {check_signal}; // No checksum found, signal an error
  return -2147483648; // Should never reach here
}}

{func_code}
"""


def crealize(cdb_file: Path, func_file: Path, map_file: Path):
  func_name = func_file.stem
  func_code = func_file.read_text()
  func_maps = configs.parse_mapping(map_file)
  num_maps = len(func_maps)
  if num_maps == 0:
    return False
  num_params = len(func_maps[0][0])

  # Change the checksum function into a checksum check function
  # and add the new checksum check function to the code.
  chkchk_name = func_name + "_checksum"
  func_code = func_code.replace(configs.CHKSUM_FUNC, chkchk_name)
  func_code = CREALIZE_TEMPLATE.format(
    chkchk_name=chkchk_name,
    num_maps=num_maps,
    num_params=num_params,
    outputs=",\n".join(["    {" + ",".join([str(x) for x in m[1]]) + "}" for m in func_maps]),
    checksums=",".join([str(m[2]) for m in func_maps]),
    check_signal=CREALIZE_CHK_SIGN,
    func_code=func_code,
  )

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
      "include_headers": ["stdlib.h"],  # for exit()
      "include_sources": [],
    }) + "\n")

  return True


# -==========================================================
# CompCert and its utility functions
# -==========================================================


def verify_ubfree(
    ccomp: str, prog: str, *, timeout: int, extra: Optional[List[str]] = None
) -> Tuple[bool, Optional[str]]:
  try:
    extra = extra if extra else []
    out = cmdline.get_out([ccomp, '-fall'] + extra + ['-interp', prog], timeout=timeout)
    if "ERROR: Undefined behavior" in out:
      return False, out
    else:
      return True, out
  except TimeoutExpired as e:
    return False, f"CompCert verification timed out: {e}"


# -==========================================================
# Fuzzing worker to generate programs and test compilers
# -==========================================================


@dataclass
class WorkerConf:
  cc: str  # Compiler command to use (e.g., 'gcc -O3 -fno-tree-slsr', 'clang')
  wdir: Path  # Working directory for the worker
  switch_limit: int  # Number of functions before switching to program generation
  prog_limit: int  # Limit for the number of generated programs per switch
  creal_env: Optional[CrealEnv] = (
    None  # Creal environment for generating mutants (None means disabled)
  )
  creal_limit: int = (
    500  # Limit for the total number of mutants generated by Creal per switch
  )
  creal_limit_per_func: int = 5  # Limit for the number of mutants generated by Creal per function


@dataclass
class WorkerRunOptions:
  limit: int  # Maximum number of iterations for the worker
  stop: Event  # Synchronizing event to stop the worker
  gen_tmo: int = 3  # Timeout (seconds) for function/program generation
  test_tmo: int = (
    10  # Timeout (seconds) for testing the compiler with the generated program
  )
  creal_tmo: int = 15  # Timeout (seconds) for Creal generation


class Worker:
  def __init__(self, wid: int, *, wconf: WorkerConf, msgq: Queue):
    self.wid = wid
    self.wconf = wconf
    self.msgq = msgq
    self.ice_dir = wconf.wdir / 'intern_errors'
    self.hang_dir = wconf.wdir / 'prog_hangs'
    self.wrc_dir = wconf.wdir / 'wrong_codes'
    self.log_file = wconf.wdir / 'worker.log'
    self.log_file_fd = self.log_file.open(mode='w', encoding='utf-8')
    self.ice_dir.mkdir(parents=True, exist_ok=True)
    self.hang_dir.mkdir(parents=True, exist_ok=True)
    self.wrc_dir.mkdir(parents=True, exist_ok=True)
    if wconf.creal_env:
      self.creal_dir = wconf.wdir / 'realsmiths'
      self.creal_dir.mkdir(parents=True, exist_ok=True)
    else:
      self.creal_dir = None
    self.iter = 0

  def close(self):
    if not self.log_file_fd.closed:
      self.log_file_fd.close()

  def run(self, ropts: WorkerRunOptions):
    fopts = FuncGenOptions(
      bin="./build/bin/fgen",
      uuid=next_uuid(),
      sno=0,
      outdir=self.wconf.wdir,
      extra='--Xinject-ub-proba 0.1'
    )
    popts = ProgGenOptions(
      bin="./build/bin/pgen",
      uuid='<placeholder>',
      indir=self.wconf.wdir,
      limit=self.wconf.prog_limit,
      extra='--Xreplace-proba 0.4'
    )
    copts = CrealOptions(
      bin="<placeholder>",
      dst=self.creal_dir,
      mutants=self.wconf.creal_limit_per_func
    )
    start_msg = (
      f"Worker started successfully: workdir={self.wconf.wdir}, "
      f"iter_limit={ropts.limit}, switch_limit={self.wconf.switch_limit}, "
      f"prog_limit={self.wconf.prog_limit}, creal_limit={self.wconf.creal_limit}, "
      f"gen_tmo={ropts.gen_tmo}s, test_tmo={ropts.test_tmo}s, creal_tmo={ropts.creal_tmo}s"
    )
    self.log(start_msg)
    self.notify(start_msg)
    self.iter = 0
    while self.iter < ropts.limit and not ropts.stop.is_set():
      fopts.seed = rand_int()
      prog = self.run_func(fopts, ropts.gen_tmo, ropts.test_tmo)
      if prog is not None:
        fopts.sno += 1
        if self.wconf.creal_env:
          copts.bin = str(self.wconf.creal_env.creal_gm_bin())
          copts.src = prog  # We'll mutate our generated function
          copts.cdb = None  # We'll use Creal's default database
          copts.seed = rand_int()
          self.run_creal(copts, ropts.creal_tmo)
      if fopts.sno == self.wconf.switch_limit:
        self.log(f"Switched to program generation", color="yellow")
        popts.uuid = next_uuid()
        popts.seed = rand_int()
        self.run_prog(popts, ropts.test_tmo)
        if self.wconf.creal_env:
          copts.bin = str(self.wconf.creal_env.creal_bin())
          copts.src = None  # We'll use Csmith for seed generation
          copts.cdb = self.build_crealdb(fopts)  # We'll use our database
          for _ in range(self.wconf.creal_limit // self.wconf.creal_limit_per_func):
            copts.seed = rand_int()
            self.run_creal(copts, ropts.creal_tmo)
        self.log(f"Removing useless functions and programs and mutants")
        self.remove_useless_funcs(fopts)
        self.remove_useless_progs(popts)
        if self.wconf.creal_env:
          self.remove_useless_mutants(copts)
        self.log(f"Switched back to function generation", color="yellow")
        fopts.uuid = next_uuid()
        fopts.sno = 0
      self.iter += 1

  def run_func(self, opts: FuncGenOptions, gen_tmo: int, test_tmo: int) -> Optional[Path]:
    self.log(f"Generating function: {", ".join([str(x[0]) + "=" + str(x[1]) for x in opts.to_dict().items()])}")
    prog, errmsg = generate_function(opts, timeout=gen_tmo)
    if not prog:
      # TODO: Save it as it might be our bugs
      self.log(f"Failure: {errmsg}", color="yellow")
      return None
    self.log(f"Generated function: {prog}")
    self.log(f"Testing the compiler with the generated function: {prog}")
    with prog.open('a') as fout:
      fout.write(f"\n\n// Fgen Options: {json.dumps(opts.to_dict())}")
    bina = (prog.parent / (prog.name + '.out'))
    artifacts = list(configs.get_artifacts(opts.uuid, opts.sno, gen_dir=opts.outdir).values()) + [bina]
    self.test(prog, bina, artifacts=artifacts, timeout=test_tmo)
    return prog

  def run_prog(self, opts: ProgGenOptions, test_tmo: int):
    self.log(f"Generating programs: {", ".join([str(x[0]) + "=" + str(x[1]) for x in opts.to_dict().items()])}")
    errmsg = generate_programs(opts)
    if errmsg:
      self.log(f"Failure: {errmsg}", color="yellow")
      return
    self.log(f"Testing the compiler with the generated programs")
    index = 0
    for prog in configs.get_progs_dir(opts.indir).iterdir():
      if not prog.name.startswith(opts.uuid) or prog.suffix != '.c':
        continue
      self.log(f"{index}: Testing the compiler with the generated program: {prog}")
      with prog.open('a') as fout:
        fout.write(f"\n\n// Pgen Options: {json.dumps(opts.to_dict())}")
      bina = (prog.parent / (prog.name + '.out'))
      artifacts = [prog, bina]
      self.test(prog, bina, artifacts=artifacts, timeout=test_tmo)
      index += 1

  def build_crealdb(self, fopts: FuncGenOptions) -> Optional[Path]:
    self.log("Building CrealDB from the generated functions")
    tmp_file = self.wconf.wdir / 'crealdb.tmp.jsonl'
    cdb_file = self.wconf.wdir / 'crealdb.json'
    for i in range(fopts.sno + 1):
      arts = configs.get_artifacts(fopts.uuid, i, gen_dir=fopts.outdir)
      func, map = arts['func'], arts['map']
      if not func or not map:
        continue  # Skip if the function or map file does not exist
      if not func.exists() or not map.exists():
        continue  # Skip if the function or map file does not exist
      crealize(tmp_file, func, map)
    with tmp_file.open() as fin:
      items = []
      for line in fin:
        if not line:
          continue
        items.append(json.loads(line.strip()))
    with cdb_file.open("w") as fout:
      json.dump(items, fout, indent=2)
    tmp_file.unlink(missing_ok=True)  # Remove the temporary file
    self.log(f"CrealDB saved to {cdb_file}")
    return cdb_file

  def run_creal(self, opts: CrealOptions, timeout: int):
    self.log(
      f"Generating Creal mutants: {", ".join([str(x[0]) + "=" + str(x[1]) for x in opts.to_dict().items()])}"
    )
    mutants, errmsg = run_creal(opts, timeout=timeout)
    if errmsg:
      self.log(f"Failure: {errmsg}", color="yellow")
      return
    self.log(f"Succeeded in generating {len(mutants)} mutants")
    for i, mut in enumerate(mutants):
      self.log(f"{i}: Testing the compiler with the Creal-generated mutant: " + str(mut))
      with mut.open('a') as fou:
        fou.write(f"\n\n// Creal Options: {json.dumps(opts.to_dict())}")
      bina = (mut.parent / (mut.name + '.out'))
      self.test(
        mut, bina, artifacts=[mut, bina], timeout=timeout,
        extra_cc_opts=f"-I{self.wconf.creal_env.csmith_incl_dir()}"
      )
      if mut.exists():  # No bugs found
        mut.unlink(missing_ok=False)  # Remove the mutant file after testing
        bina.unlink(missing_ok=True)

  def remove_useless_funcs(self, opts: FuncGenOptions):
    for i in range(opts.sno + 1):
      self.remove_all(configs.get_artifacts(opts.uuid, i, gen_dir=opts.outdir).values())

  def remove_useless_progs(self, opts: ProgGenOptions):
    for prog in configs.get_progs_dir(opts.indir).iterdir():
      if not prog.name.startswith(opts.uuid) or prog.suffix != '.c':
        continue
      prog.unlink(missing_ok=True)

  def remove_useless_mutants(self, opts: CrealOptions):
    for prog in opts.dst.iterdir():
      if prog.is_file():
        prog.unlink(missing_ok=True)
    if opts.cdb:
      opts.cdb.unlink(missing_ok=True)

  def test(self, prog: Path, bina: Path, *, artifacts: List[Path], timeout: int, extra_cc_opts: str = ''):
    test_res = test_compiler(f"{self.wconf.cc} {extra_cc_opts}", cfile=prog, ofile=bina, timeout=timeout)
    if test_res.is_internal_compiler_error():
      self.log(f"INTERNAL COMPILER ERROR (exitcode={test_res.exitcode}): {test_res.errmsg}", color="green")
      self.store_bug(test_res, artifacts, self.ice_dir)
    elif test_res.is_compilation_timeout():
      self.log(f"COMPILER HANG (exitcode={test_res.exitcode}): {test_res.errmsg}", color="blue")
      self.store_bug(test_res, artifacts, self.hang_dir)
    elif test_res.is_wrong_code():
      # There're cases that Creal-generated mutants are not UB-free
      if self.wconf.creal_env and is_creal_mutant(prog):
        ccomp = self.wconf.creal_env.ccomp_bin()
        if str(self.wconf.creal_env.csmith_incl_dir()) in extra_cc_opts:
          extra_ccomp_opts = [f"-I{self.wconf.creal_env.csmith_incl_dir()}"]
        else:
          extra_ccomp_opts = []
        self.log(f"Verifying Creal-generated mutant towards UBs: ccomp={ccomp}, prog={prog}, extra={' '.join(extra_ccomp_opts)}")
        ubfree, errmsg = verify_ubfree(str(ccomp), str(prog), extra=extra_ccomp_opts, timeout=timeout * 2)
        if not ubfree and CREALIZE_CHK_SIGN not in errmsg:  # This is the wrong code signal generated by us
          self.log(f"UBs detected in the mutant (skipping it): {errmsg}", color="yellow")
          self.remove_all(artifacts)
          return
      self.log(f"WRONG CODE (exitcode={test_res.exitcode}): {test_res.errmsg}", color="green")
      self.store_bug(test_res, artifacts, self.wrc_dir)
    elif test_res.is_execution_timeout():
      self.log(f"The generated program timed out (skipping it): {test_res.errmsg}")
      self.remove_all(artifacts)
    elif test_res.is_success():
      self.log("Compiler passed the test")
      bina.unlink(missing_ok=True)  # Remove the binary file
    else:
      raise RuntimeError(
        f"Unknown test result: type={test_res.type.name}, exitcode={test_res.exitcode}, errmsg={test_res.errmsg}"
      )

  def store_bug(self, res: TestRes, artifacts: List[Path], bug_dir: Path):
    for art in artifacts:
      if art.exists():
        art.rename(bug_dir / art.name)
    with (bug_dir / 'result.jsonl').open('a') as fou:
      fou.write(json.dumps(res.to_dict(), ensure_ascii=False) + '\n')

  def remove_all(self, artifacts: List[Path]):
    for art in artifacts:
      if art.exists():
        art.unlink(missing_ok=True)

  def notify(self, msg: str):
    self.msgq.put(f"Worker@{self.wid}: {msg}")

  def log(self, msg: str, *, color: Optional[str] = None, end: str = '\n'):
    log(
      f"Worker@{self.wid}",
      msg=f"[{self.iter}] " + msg,
      file=self.log_file_fd,
      color=color,
      end=end,
      flush=True
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
    worker.notify(f"Worker exited normally")
  except KeyboardInterrupt:
    worker.notify(f"Worker interrupted by user's Ctrl-C")
    worker.log(f"Worker interrupted by user's Ctrl-C", color="red")
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
  mlog(f"Waiting for remaining messages to come ...", color="yellow")
  time.sleep(5)  # Give some time for the last messages to be processed
  while not msgq.empty():
    msg = msgq.get(timeout=.5)
    mlog(msg)


def run_fuzz_main(cc: str, *, outdir: Path, workers: int, ilimit: int, slimit: int, plimit: int,
                  creal: Optional[CrealEnv], crlimit: int):
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
      f"creal.creal={creal.creal_home if creal else 'disabled'}, "
      f"creal.csmith={creal.csmith_home if creal else 'disabled'}, "
      f"creal_limit={crlimit if creal else 'disabled'}, "
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
          creal_env=creal,
          creal_limit=crlimit,
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
    mlog(f"Fuzzing process was interrupted by user signal: {e.sig.name}", color='red')
    stop.set()
  except KeyboardInterrupt:
    mlog(f"Fuzzing process was interrupted by user's Ctrl-C", color='red')
    stop.set()
  except Exception as e:
    mlog(f"Fuzzing process was interrupted by exception: {e}", color='red')
    stop.set()
  finally:
    mlog("Fuzzing process is shutting down ...")
    worker_pool.join()
    msgq_thread.join()

  mlog(f"Exit. All results are stored inside {outdir}.", color='green')


def main():
  import os

  parser = ArgumentParser(
    "fuzz", description="Tool for fuzzing a specific C compiler (gcc/llvm)"
  )

  parser.add_argument(
    "-o", "--outdir",
    type=str,
    default="fuzzout",
    help="Directory to store the output of the fuzzing process (default: 'fuzzout')",
  )
  parser.add_argument(
    "-s", "--seed",
    type=int,
    default=-1,
    help="Seed for the randomness of the fuzzing process, negative meaning no seed (default: -1)",
  )
  parser.add_argument(
    "-j", "--workers",
    type=int,
    default=os.cpu_count(),
    help=f"Number of workers to use for fuzzing (default: {os.cpu_count()})",
  )
  parser.add_argument(
    "-l", "--limit",
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
    "--creal",
    type=str,
    default=None,
    help="Path to Creal's home directory; setting this option means enable Creal during fuzzing (default: None, not used)",
  )
  parser.add_argument(
    "--csmith",
    type=str,
    default=None,
    help="Path to Csmith's home directory; this option is required when Creal is enabled (default: None, not used)",
  )
  parser.add_argument(
    "--ccomp",
    type=str,
    default=None,
    help="Path to CompCert's home directory; this option is required when Creal is enabled (default: None, not used)",
  )
  parser.add_argument(
    "--creal-limit",
    type=int,
    default=500,
    help="The maximum number of mutants to generate by Creal per switch (default: 500)",
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
    mlog(f"Error: Invalid number of workers specified: {workers}.", color='red')
    sys.exit(1)
  if workers > os.cpu_count():
    mlog(f"Warning: Number of workers ({workers}) exceeds available CPU cores ({os.cpu_count()}).", color='yellow')

  limit = args.limit
  if limit < 0:
    limit = MAX_I32
  elif limit > MAX_I32:
    mlog(f"Warning: Limit ({limit}) exceeds maximum allowed value ({MAX_I32}). Setting to {MAX_I32}.",
         color='yellow')
    limit = MAX_I32

  switch_limit = args.switch_limit
  if switch_limit <= 0:
    mlog(f"Error: Invalid switch limit specified: {switch_limit}. Must be a positive number", color='red')
    sys.exit(1)
  if switch_limit > MAX_I32:
    mlog(
      f"Warning: Switch limit ({switch_limit}) exceeds maximum allowed value ({MAX_I32}). Setting to {MAX_I32}.",
      color="yellow",
    )
    switch_limit = MAX_I32

  prog_limit = args.prog_limit
  if prog_limit <= 0:
    mlog(f"Error: Invalid program limit specified: {prog_limit}. Must be a positive number", color='red')
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
    mlog("Output directory already exists. Would you like to overwrite it? (Y/n)> ", color='yellow', end='',
         flush=True)
    yes = input()
    if yes != "Y":
      mlog("The user chose not to overwrite the output directory. Exiting.", color='red')
      sys.exit(0)
    shutil.rmtree(args.outdir)
  mlog(f"Creating output directory: {outdir}")
  outdir.mkdir(parents=True, exist_ok=False)

  creal_env = None
  if args.creal:
    if not args.csmith:
      mlog(f"Error: Csmith's home directory is required when Creal is enabled.", color='red')
      sys.exit(1)
    if not args.ccomp:
      mlog(f"Error: CompCert's home directory is required when Creal is enabled.", color='red')
      sys.exit(1)
    creal = Path(args.creal).resolve().absolute()
    csmith = Path(args.csmith).resolve().absolute()
    ccomp = Path(args.ccomp).resolve().absolute()
    creal_env = CrealEnv(creal, csmith, ccomp)
    if not creal.exists():
      mlog(f"Error: The given Creal's home directory does not exist: {creal}", color='red')
      sys.exit(1)
    if not creal.is_dir():
      mlog(f"Error: The given Creal's home directory is not a directory: {creal}", color='red')
      sys.exit(1)
    if not creal_env.creal_bin().exists():
      mlog(f"Error: Creal's home directory does not contain '{creal_env.creal_bin().name}': {creal}", color='red')
      sys.exit(1)
    if not creal_env.creal_gm_bin().exists():
      mlog(f"Error: Creal's home directory does not contain '{creal_env.creal_gm_bin().name}': {creal}", color='red')
      sys.exit(1)
    if not csmith.exists():
      mlog(f"Error: The given Csmith's home directory does not exist: {csmith}", color='red')
      sys.exit(1)
    if not csmith.is_dir():
      mlog(f"Error: The given Csmith's home directory is not a directory: {csmith}", color='red')
      sys.exit(1)
    if not creal_env.csmith_bin().exists():
      mlog(f"Error: Csmith's home directory does not contain '{creal_env.csmith_bin().name}'", color='red')
      sys.exit(1)
    if not creal_env.csmith_incl_dir().exists():
      mlog(f"Error: Csmith's home directory does not contain '{creal_env.csmith_incl_dir().name}'", color='red')
      sys.exit(1)
    if not ccomp.exists():
      mlog(f"Error: The given CompCert's home directory does not exist: {ccomp}", color='red')
      sys.exit(1)
    if not ccomp.is_dir():
      mlog(f"Error: The given CompCert's home directory is not a directory: {ccomp}", color='red')
      sys.exit(1)
    if not creal_env.ccomp_bin().exists():
      mlog(f"Error: CompCert's home directory does not contain '{creal_env.ccomp_bin().name}'", color='red')
      sys.exit(1)
    mlog(f"Testing if Creal is valid ...")
    test_out = outdir / "realsmith"
    _, errmsg = run_creal(
      CrealOptions(
        bin=str(creal_env.creal_bin()),
        dst=test_out,
        mutants=1,
      ),
      timeout=60,
    )
    if errmsg is not None:
      mlog(f"Invalid: {errmsg}", color="red")
      sys.exit(1)
    mlog("Valid")
    shutil.rmtree(test_out, ignore_errors=True)

  creal_limit = args.creal_limit
  if creal_env:
    if creal_limit <= 0:
      mlog(f"Error: Invalid Creal limit specified: {creal_limit}. Must be a positive number", color='red')
      sys.exit(1)
    if creal_limit > MAX_I32:
      mlog(
        f"Warning: Creal limit ({creal_limit}) exceeds maximum allowed value ({MAX_I32}). Setting to {MAX_I32}.",
        color="yellow",
      )
      creal_limit = MAX_I32

  compiler = args.compiler
  test_input = outdir / 'a.c'
  test_input.write_text("int main() { return 0; }", encoding='utf-8')
  test_output = outdir / 'a.out'
  mlog(f"Testing if the compiler command is valid ...")
  test_res = test_compiler(compiler, cfile=test_input, ofile=test_output, timeout=1)
  test_input.unlink()
  test_output.unlink()
  if not test_res.is_success():
    mlog(f"Invalid: {test_res.errmsg}", color='red')
    sys.exit(1)
  else:
    mlog("Valid")

  # Save the command line arguments to a JSON file
  (outdir / 'command.json').write_text(json.dumps({
    **vars(args),
    'seed': seed
  }, indent=2))

  # Start the fuzzing loop
  run_fuzz_main(
    compiler,
    workers=workers,
    ilimit=limit,
    slimit=switch_limit,
    plimit=prog_limit,
    outdir=outdir,
    creal=creal_env,
    crlimit=creal_limit,
  )


if __name__ == "__main__":
  main()
