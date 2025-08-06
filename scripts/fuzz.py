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
from multiprocessing import Event, Manager, Pool
from pathlib import Path
from subprocess import TimeoutExpired, CalledProcessError, PIPE, STDOUT
from typing import Optional, Tuple, List
from uuid import uuid4 as uuidgen

import params
import utils


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
    utils.check_out(cmd, timeout=timeout)
    result = params.get_prog_file(opts.uuid, opts.sno, gen_dir=opts.outdir), None
  except CalledProcessError as e:
    result = None, (
      f"exit with {e.returncode}, message: {e.stdout or '<no output>'}"
    )
  except TimeoutExpired:
    result = None, f"generation timeout: exceeding {timeout}s"
  except Exception as e:
    result = None, f"unexpected error: {e}"
  if result[0] is None:
    for art in params.get_artifacts(opts.uuid, opts.sno, gen_dir=opts.outdir).values():
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
    utils.check_out(cmd)
    return None
  except CalledProcessError as e:
    return f"exit with {e.returncode}, message: {e.stdout or '<no output>'}"
  except Exception as e:
    return f"unexpected error: {e}"


# -==========================================================
# Logging and utility functions
# -==========================================================

def log(title: str, msg: str, *, color: Optional[str] = None, end: str = '\n', flush: bool = False, file=sys.stderr):
  disable = '\033[0m'
  enable = {
    'red': '\033[91m',
    'green': '\033[92m',
    'yellow': '\033[93m',
    'blue': '\033[94m',
    'magenta': '\033[95m',
    'cyan': '\033[96m',
    'white': '\033[97m',
  }.get(color, '')
  print(f"{enable}[{datetime.datetime.now().strftime('%Y-%m-%d::%H:%M:%S')}::{title}] {msg}{disable}", end=end,
        flush=flush, file=file)


def mlog(msg, *, color: Optional[str] = None, end: str = '\n', flush: bool = False):
  log("Main", msg=msg, color=color, end=end, flush=flush)


# -==========================================================
# Compiler testing and result handling
# -==========================================================


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

  def to_dict(self) -> dict:
    return {
      'cmd': self.cmd,
      'type': self.type.value,
      'exitcode': self.exitcode,
      'errmsg': self.errmsg or '<no-error-message>',
      'timestamp': datetime.datetime.now().isoformat()
    }


def test_compiler(cc: str, cfile: Path, ofile: Path, timeout: int) -> TestRes:
  comp_cmd = f"{cc} -o {ofile} {cfile}"
  try:
    proc = utils.run_proc(shlex.split(comp_cmd), stdout=PIPE, stderr=STDOUT, timeout=timeout * 10, check=False)
  except TimeoutExpired as e:
    return TestRes(comp_cmd, TestRes.Type.CTO, exitcode=-1, errmsg=str(e))
  if proc.returncode != 0:
    return TestRes(comp_cmd, TestRes.Type.ICE, exitcode=-proc.returncode, errmsg=proc.stdout)
  exec_cmd = f"{ofile}"
  full_cmd = f"{comp_cmd}; {exec_cmd}"
  try:
    proc = utils.run_proc(shlex.split(exec_cmd), stdout=PIPE, stderr=STDOUT, timeout=timeout * 20, check=False)
  except TimeoutExpired as e:
    return TestRes(full_cmd, TestRes.Type.ETO, exitcode=-1, errmsg=str(e))
  if proc.returncode != 0:
    return TestRes(full_cmd, TestRes.Type.WRC, exitcode=proc.returncode, errmsg=proc.stdout)
  return TestRes(full_cmd, TestRes.Type.SUC, exitcode=0, errmsg=None)


# -==========================================================
# Fuzzing worker to generate programs and test compilers
# -==========================================================

class Worker:
  def __init__(self, wid: int, *, cc: str, wdir: Path, switch_limit: int, prog_limit: int):
    self.wid = wid
    self.cc = cc
    self.wdir = wdir
    self.iter = 0
    self.ice_dir = wdir / 'intern_error'
    self.hang_dir = wdir / 'prog_hang'
    self.wrc_dir = wdir / 'wrong_code'
    self.log_file = wdir / 'worker.log'
    self.log_file_fd = self.log_file.open(mode='w', encoding='utf-8')
    self.ice_dir.mkdir(parents=True, exist_ok=True)
    self.hang_dir.mkdir(parents=True, exist_ok=True)
    self.wrc_dir.mkdir(parents=True, exist_ok=True)
    self.switch_limit = switch_limit
    self.prog_limit = prog_limit

  def close(self):
    if not self.log_file_fd.closed:
      self.log_file_fd.close()

  def run(self, *, iter_limit: int, stop: Event, gen_tmo: int, test_tmo: int):
    switch_limit = self.switch_limit  # Switch to program generation every 100 functions
    prog_limit = self.prog_limit  # Limit for the number of generated program per switch
    fopts = FuncGenOptions(
      bin="./build/bin/fgen",
      uuid=self.uuid(),
      sno=0,
      outdir=self.wdir,
      extra='--Xinject-ub-proba 0.1'
    )
    popts = ProgGenOptions(
      bin="./build/bin/pgen",
      uuid='<placeholder>',
      indir=self.wdir,
      limit=prog_limit,
      extra='--Xreplace-proba 0.4'
    )
    self.log(
      f"Worker started: workdir={self.wdir}, "
      f"iter_limit={iter_limit}, switch_limit={switch_limit}, prog_limit={prog_limit}, "
      f"gen_tmo={gen_tmo}s, test_tmo={test_tmo}s"
    )
    self.iter = 0
    while self.iter < iter_limit and not stop.is_set():
      fopts.seed = random.randint(0, 2147483647)
      if self.run_func(fopts, gen_tmo, test_tmo):
        fopts.sno += 1
      if fopts.sno == switch_limit:
        self.log(f"Switched to program generation", color="yellow")
        popts.uuid = self.uuid()
        popts.seed = random.randint(0, 2147483647)
        self.run_prog(popts, test_tmo)
        self.log(f"Removing useless functions and programs")
        self.remove_useless_funcs(fopts)
        self.remove_useless_progs(popts)
        self.log(f"Switched back to function generation", color="yellow")
        fopts.uuid = self.uuid()
        fopts.sno = 0
      self.iter += 1

  def run_func(self, opts: FuncGenOptions, gen_tmo: int, test_tmo: int) -> bool:
    self.log(f"Generating function: uuid={opts.uuid}, number={opts.sno}, seed={opts.seed}")
    prog, errmsg = generate_function(opts, timeout=gen_tmo)
    if not prog:
      # TODO: Save it as it might be our error
      self.log(f"Failure: {errmsg}", color="yellow")
      return False
    self.log(f"Generated function: {prog}")
    self.log(f"Test the compiler with the generated function: {prog}")
    with prog.open('a') as fout:
      fout.write(f"\n\n// Options: {json.dumps(opts.to_dict())}")
    bina = (prog.parent / (prog.name + '.out'))
    artifacts = list(params.get_artifacts(opts.uuid, opts.sno, gen_dir=opts.outdir).values()) + [bina]
    self.test(prog, bina, artifacts=artifacts, timeout=test_tmo)
    return True

  def run_prog(self, opts: ProgGenOptions, test_tmo: int):
    self.log(f"Generating programs: uuid={opts.uuid}, seed={opts.seed}, limit={opts.limit}")
    errmsg = generate_programs(opts)
    if errmsg:
      self.log(f"Failure: {errmsg}", color="yellow")
      return
    self.log(f"Testing the compiler with the generated programs")
    for prog in params.get_progs_dir(opts.indir).iterdir():
      if not prog.name.startswith(opts.uuid) or prog.suffix != '.c':
        continue
      self.log(f"Test the compiler with the generated program: {prog}")
      with prog.open('a') as fout:
        fout.write(f"\n\n// Options: {json.dumps(opts.to_dict())}")
      bina = (prog.parent / (prog.name + '.out'))
      artifacts = [prog, bina]
      self.test(prog, bina, artifacts=artifacts, timeout=test_tmo)

  def remove_useless_funcs(self, opts: FuncGenOptions):
    for i in range(opts.sno + 1):
      self.remove_all(params.get_artifacts(opts.uuid, i, gen_dir=opts.outdir).values())

  def remove_useless_progs(self, opts: ProgGenOptions):
    for prog in params.get_progs_dir(opts.indir).iterdir():
      if not prog.name.startswith(opts.uuid) or prog.suffix != '.c':
        continue
      prog.unlink(missing_ok=True)

  def test(self, prog: Path, bina: Path, *, artifacts: List[Path], timeout: int):
    test_res = test_compiler(self.cc, cfile=prog, ofile=bina, timeout=timeout)
    if test_res.is_internal_compiler_error():
      self.log(f"INTERNAL COMPILER ERROR (exitcode={test_res.exitcode}): {test_res.errmsg}", color="green")
      self.store_bug(test_res, artifacts, self.ice_dir)
    elif test_res.is_compilation_timeout():
      self.log(f"COMPILER HANG (exitcode={test_res.exitcode}): {test_res.errmsg}", color="blue")
      self.store_bug(test_res, artifacts, self.hang_dir)
    elif test_res.is_wrong_code():
      self.log(f"WRONG CODE (exitcode={test_res.exitcode}): {test_res.errmsg}", color="green")
      self.store_bug(test_res, artifacts, self.wrc_dir)
    elif test_res.is_execution_timeout():
      self.log(f"The generated program timed out: {test_res.errmsg}")
      self.remove_all(artifacts)
    elif test_res.is_success():
      self.log("Compiler passed the test")
      bina.unlink(missing_ok=True)  # Remove the binary file
    else:
      raise RuntimeError(
        f"Unknown test result: type={test_res.type.name}, exitcode={test_res.exitcode}, errmsg={test_res.errmsg}")

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

  def log(self, msg: str, *, color: Optional[str] = None, end: str = '\n'):
    log(
      f"Worker@{self.wid}",
      msg=f"[{self.iter}] " + msg,
      file=self.log_file_fd,
      color=color,
      end=end,
      flush=True
    )

  def uuid(self):
    return str(uuidgen()).replace('-', '_')


# -==========================================================
# Fuzzing loop and worker management
# -==========================================================


def run_worker(*, cc: str, wid: int, wdir: Path, seed: int, gen_tmo: int, test_tmo: int, ilimit: int, slimit: int,
               plimit: int, stop: Event):
  wdir.mkdir(parents=False, exist_ok=False)
  random.seed(seed)
  worker = Worker(wid, cc=cc, wdir=wdir, switch_limit=slimit, prog_limit=plimit)
  try:
    worker.run(iter_limit=ilimit, stop=stop, gen_tmo=gen_tmo, test_tmo=test_tmo)
  except KeyboardInterrupt:
    worker.log(f"Worker interrupted by user's Ctrl-C", color="red")
  except Exception as e:
    worker.log(f"Worker interrupted by exception: {e}", color="red")
  finally:
    worker.close()


def run_fuzz_main(cc: str, *, outdir: Path, workers: int, ilimit: int, slimit: int, plimit: int):
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
  stop = manager.Event()
  pool = Pool(workers)
  results = {}
  gen_tmo = 3  # seconds for function/program generation
  test_tmo = 10  # seconds for testing the compiler with the generated program
  for wid in range(workers):
    wdir = outdir / f'fuzz_proc_{wid:05}'  # Working directory for each worker
    seed = random.randint(0, 2147483647)  # Random seed for each worker
    mlog(
      f"Request worker: "
      f"wid={wid}, workdir={wdir}, seed={seed}, "
      f"gen_tmo={gen_tmo}s, test_tmo={test_tmo}s, "
      f"iter_limit={ilimit if ilimit != 0 else 'infinite'} "
      f"compiler=`{cc}`"
    )
    results[wid] = pool.apply_async(
      run_worker,
      kwds={
        'cc': cc,
        'wid': wid,
        'wdir': wdir,
        'seed': seed,
        'gen_tmo': gen_tmo,
        'test_tmo': test_tmo,
        'ilimit': ilimit // workers,
        'plimit': plimit,
        'slimit': slimit,
        'stop': stop,
      }
    )
  pool.close()

  mlog("Press Ctrl-C to stop the fuzzing process at any time ...")

  try:
    pool.join()
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
    pool.join()
    mlog("Exit")


def main():
  import os

  parser = ArgumentParser(
    "fuzz", description="Tool for fuzzing a specific C compiler (gcc/llvm)"
  )

  parser.add_argument(
    "-o", "--outdir",
    type=str,
    default="output",
    help="Directory to store the output of the fuzzing process (default: 'output')",
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
    default=2147483647,
    help="The maximum number of fuzz iterations, non-positive meaning default (default: 2147483647)",
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
    "compiler", type=str, help="Command to compile the C program (e.g., 'gcc -fsanitize=address -O0')"
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
    limit = 2147483647
  elif limit > 2147483647:
    mlog(f"Warning: Limit ({limit}) exceeds maximum allowed value (2147483647). Setting to 2147483647.",
         color='yellow')
    limit = 2147483647

  switch_limit = args.switch_limit
  if switch_limit <= 0:
    mlog(f"Error: Invalid switch limit specified: {switch_limit}. Must be a positive number", color='red')
    sys.exit(1)
  if switch_limit > 2147483647:
    mlog(
      f"Warning: Switch limit ({switch_limit}) exceeds maximum allowed value (2147483647). Setting to 2147483647.",
      color='yellow')
    switch_limit = 2147483647

  prog_limit = args.prog_limit
  if prog_limit <= 0:
    mlog(f"Error: Invalid program limit specified: {prog_limit}. Must be a positive number", color='red')
    sys.exit(1)
  if prog_limit > 2147483647:
    mlog(
      f"Warning: Program limit ({prog_limit}) exceeds maximum allowed value (2147483647). Setting to 2147483647.",
      color='yellow')
    prog_limit = 2147483647

  if switch_limit >= prog_limit:
    mlog("Warning: Program limit is recommended to be larger than switching limit.")

  outdir = Path(args.outdir).resolve().absolute()
  if outdir.exists():
    mlog("Output directory already exists. Would you like to overwrite it? (Y/n)> ", color='yellow', end='',
         flush=True)
    yes = input()
    if yes != "Y":
      mlog("The user chose not to overwrite the output directory. Exiting.", color='red')
      sys.exit(1)
    shutil.rmtree(args.outdir)
  mlog(f"Creating output directory: {outdir}")
  outdir.mkdir(parents=True, exist_ok=False)

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
    outdir=outdir
  )


if __name__ == "__main__":
  main()
