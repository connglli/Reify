#  MIT License
#
#  Copyright (c) 2025
#
#  Kavya Chopra (chopra.kavya04@gmail.com)
#  Cong Li (cong.li@inf.ethz.ch)
#  Florian Bruno (schwinix@proton.me)
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

from __future__ import annotations

import copy
import datetime
import json
import random
import shlex
import shutil
import sys
from argparse import ArgumentParser
from dataclasses import dataclass, replace
from enum import Enum
from pathlib import Path
from typing import Dict, Optional, TextIO, Tuple

import cmdline


class Logger:
  verbose: bool
  out_file: TextIO = sys.stderr

  def set_verbose(self: Logger, verbose: bool):
    self.verbose = verbose

  def set_out(self: Logger, out_file: TextIO):
    self.out_file = out_file

  def __print(
    self: Logger,
    title: str,
    msg: str,
    *,
    color: Optional[str] = None,
    end: str = "\n",
    flush: bool = False,
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
      f"{enable}[{datetime.datetime.now().strftime('%Y-%m-%d::%H:%M:%S')}::{title}]{disable} {msg}",
      flush=flush,
      file=self.out_file,
    )

  def log_error(self: Logger, msg: str):
    self.__print(title="Error", msg=msg, color="red", flush=True)

  def log_warning(self: Logger, msg: str):
    self.__print(title="Warning", msg=msg, color="yellow")

  def log_info(self: Logger, msg: str):
    if not self.verbose:
      return
    self.__print(title="Info", msg=msg, color="blue")


@dataclass
class ProgGenOptions:
  bin: str  # Path to the rylink executable
  uuid: str  # Primary ID for the newly generated program
  indir: Path  # Directory to read the input function files
  limit: int  # The maximum number of programs to generate (0 means unlimited)
  sno: int  # Target Sample Number
  seed: int  # Seed for the random number generator
  rule_info: bool = False  # Whenever to output the rule info json
  reduce_mode: Optional[Path] = None  # path to the reduce mode file
  rule: Optional[int] = None  # Number of Rules applied in each block
  extra: Optional[str] = None  # Extra options to control the program generation process


class ReduceInfo:
  sno: int
  info: list[Tuple[str, str, int]]
  __bitvec: int

  def __init__(self: ReduceInfo, sno: int):
    self.info = []
    self.__bitvec = 0
    self.sno = sno

  def items(self: ReduceInfo):
    return self.info

  def add(self: ReduceInfo, function_name: str, header_block: str, rule_count: int):
    self.__bitvec |= 1 << len(self.info)
    self.info.append((function_name, header_block, rule_count))

  def copy(self: ReduceInfo) -> ReduceInfo:
    return copy.deepcopy(self)

  def get_empty(self: ReduceInfo) -> ReduceInfo:
    empty: ReduceInfo = self.copy()
    for i in range(len(empty.info)):
      empty.info[i] = (empty.info[i][0], empty.info[i][1], 0)
    empty.__bitvec = 0
    return empty

  @staticmethod
  def from_json(file: Path) -> ReduceInfo:
    reduce_info_json: Dict = json.load(file.open())
    reduce_info: ReduceInfo = ReduceInfo(reduce_info_json["sno"])
    for function in reduce_info_json["functions"]:
      function_name: str = function["name"]
      for block in function["blocks"]:
        header_label: str = block["headerLabel"]
        reduce_info.add(function_name, header_label, block["targetRuleCount"])
    return reduce_info

  def to_json(self: ReduceInfo, path: Path):
    reduce_info_json: Dict = {}
    reduce_info_json["functions"] = []
    seen_functions: Dict[str, int] = {}
    count: int = 0
    reduce_info_json["sno"] = self.sno
    for [function, header_block, rule_count] in self.info:
      if seen_functions.__contains__(function):
        reduce_info_json["functions"][seen_functions[function]]["blocks"].append(
          {"headerLabel": header_block, "targetRuleCount": rule_count}
        )
      else:
        seen_functions[function] = count
        count += 1
        reduce_info_json["functions"].append(
          {
            "name": function,
            "blocks": [{"headerLabel": header_block, "targetRuleCount": rule_count}],
          }
        )
    json.dump(reduce_info_json, open(path, "w"))

  # Bitvec is unique for all ReduceInfos r1, r2 s.t. r1 != r2, see __eq__
  # and conversly ofcourse r1 == r2 => __hash__(r1) == __hash__(r2)__
  def __hash__(self: ReduceInfo):
    return self.__bitvec

  def __getitem__(self: ReduceInfo, key: Tuple[str, str]):
    [function_name, header_block] = key
    for item in self.info:
      if function_name == item[0] and header_block == item[1]:
        return item[2]

  def __setitem__(self: ReduceInfo, key: Tuple[str, str], rule_count: int):
    [function_name, header_block] = key
    for i in range(len(self.info)):
      if function_name == self.info[i][0] and header_block == self.info[i][1]:
        self.info[i] = (self.info[i][0], self.info[i][1], rule_count)
        if rule_count == 0:
          self.__bitvec &= ~(1 << i)
        else:
          self.__bitvec |= 1 << i
        break
    else:
      self.add(function_name, header_block, rule_count)
      self.__bitvec |= 1 << len(self.info)
      self.info.append((function_name, header_block, rule_count))

  def __delitem__(self: ReduceInfo, _key: Tuple[str, str]):
    assert True, "Cannot Delete from RuleInfo"

  # This is a shallow equallity e.g. only over if a block has ruleCount == 0 or not
  def __eq__(self: ReduceInfo, other_obj: object):
    if not isinstance(other_obj, ReduceInfo):
      return False
    other: ReduceInfo = other_obj
    return self.__bitvec == other.__bitvec


def next_uuid():
  keywords = "0123456789abcdefghijklmnopqrstuvwxyz"
  return "".join(random.choices(keywords, k=6))


def make_file_name(prefix: str):
  import os

  return prefix + str(os.getpid())


def run_rylink(popts: ProgGenOptions):
  cmd: list[str] = [
    popts.bin,
    "-i",
    str(popts.indir),
    "-l",
    str(popts.limit),
    "-s",
    str(popts.seed),
    "-n",
    str(popts.sno),
  ]
  cmd += [popts.uuid]
  if popts.rule:
    cmd += str(popts.rule)
  if popts.rule_info:
    cmd += ["--Xrule-info"]
  if popts.reduce_mode:
    cmd += ["--Xreduce-mode", str(popts.reduce_mode)]
  if popts.extra:
    cmd += shlex.split(popts.extra)

  cmdline.check_run(cmd)


def get_reduce_info(popts: ProgGenOptions, outdir: Path, sno: int) -> ReduceInfo:
  reduce_info_opts: ProgGenOptions = replace(popts, rule_info=True, reduce_mode=None)
  try:
    run_rylink(reduce_info_opts)
  except Exception:
    logger.log_error("Failed to get Reduce Info")
    exit(1)

  reduce_info_file: Path = popts.indir / ("prog_" + popts.uuid + "_" + str(sno)) / "ruleinfo.jsonl"
  reduce_info = ReduceInfo.from_json(reduce_info_file)

  prog_dir: str = "prog_" + popts.uuid + "_" + str(popts.sno)
  shutil.rmtree(str(popts.indir / prog_dir))

  return reduce_info


def get_final_reduction(popts: ProgGenOptions, outdir: Path, reduce_info: ReduceInfo):
  reduce_mode_path: Path = Path("/tmp/" + make_file_name("reduce_file.jsonl"))
  reduce_info.to_json(reduce_mode_path)

  reduce_info_opts: ProgGenOptions = replace(popts, rule_info=True, reduce_mode=reduce_mode_path)
  try:
    run_rylink(reduce_info_opts)
  except Exception:
    logger.log_error("Failed to get final Reduction")
    exit(1)

  prog_dir: str = "prog_" + popts.uuid + "_" + str(popts.sno)
  shutil.move(str(popts.indir / prog_dir), str(outdir / prog_dir))


class TestResult(Enum):
  BUGGY = True
  FIXED = False


@dataclass
class TestOpts:
  popts: ProgGenOptions
  outdir: Path
  reduce_info: ReduceInfo
  test: str
  args: Optional[str] = None


def test(opts: TestOpts) -> TestResult:
  reduce_mode_path: Path = Path("/tmp/" + make_file_name("reduce_file.jsonl"))
  opts.reduce_info.to_json(reduce_mode_path)

  reduce_mode_opts: ProgGenOptions = replace(
    opts.popts, rule_info=False, reduce_mode=str(reduce_mode_path)
  )
  try:
    run_rylink(reduce_mode_opts)
  except Exception:
    logger.log_error("Failed to run Test")
    exit(1)

  prog_dir_name: str = "prog_" + opts.popts.uuid + "_" + str(opts.popts.sno)
  prog_path: Path = opts.popts.indir / prog_dir_name
  cmd: list[str] = [opts.test, str(prog_path)]
  if opts.args:
    cmd += shlex.split(opts.args)

  ret: int = cmdline.get_ret(cmd)
  shutil.rmtree(prog_path)

  return TestResult(ret == 0)


# DeltaDebug Helpers:


@dataclass(eq=True, frozen=True)
class AtomicDelta:
  function_name: str
  header_label: str
  rule_count: int

  def apply(self: AtomicDelta, reduce_info: ReduceInfo):
    reduce_info[self.function_name, self.header_label] = self.rule_count


Delta = set[AtomicDelta]


def apply_delta(delta: Delta, reduce_info: ReduceInfo) -> ReduceInfo:
  res: ReduceInfo = reduce_info.copy()
  for c in delta:
    c.apply(res)
  return res


def split_delta(delta: Delta, n: int) -> list[Delta]:
  deltas: list[set[AtomicDelta]] = []
  curr_delta: list[AtomicDelta] = list(delta)
  start = 0
  for i in range(n):
    new_delta: list[AtomicDelta] = curr_delta[start : int(start + (len(delta) - start) / (n - i))]
    deltas.append(Delta(set(new_delta)))
    start = start + len(new_delta)
  return deltas


def get_delta_from_reduce_info(reduce_info: ReduceInfo) -> Delta:
  d: set[AtomicDelta] = set()
  for [function_name, header_label, rule_count] in reduce_info.items():
    if rule_count > 0:
      d.add(AtomicDelta(function_name, header_label, rule_count))
  return d


# TODO: This could be speed up with caching
# Apply Delta Debugging to the program generation. An Atomic Change here is "for function f and block b apply all transformations"
# source: https://www.cs.cornell.edu/courses/cs5150/2025sp/lecture/lec21-slides-delta-debugging.pdf
# 1. Start with n = 2 and Δ as test set
# 2. Test each Δ1, Δ2, …, Δn and each ∇1, ∇2, …, ∇n
# 3. There are three possible outcomes:
#   a. Some Δi causes failure: Go to step (1) with Δ = Δi and n = 2
#   b. Some ∇i causes failure: Go to step (1) with Δ = ∇i and n = n - 1
#   c. No test causes failure:
# If granularity can be refined: Go to step (1) with Δ = Δ and n = n * 2
# Otherwise: Done, found the 1-minimal subset
def delta_debug(opts: TestOpts) -> ReduceInfo:
  empty_reduce_info: ReduceInfo = opts.reduce_info.get_empty()
  entire_delta: Delta = get_delta_from_reduce_info(opts.reduce_info)
  assert (
    apply_delta(entire_delta, empty_reduce_info) == opts.reduce_info
  ), "entireDelta is not the entire difference between empty- and initReduceInfo"

  result_cache: Dict[ReduceInfo, TestResult] = {}

  # 1.
  n: int = 2
  current_delta: Delta = entire_delta
  logger.log_info(f"Start DD with n = {n} and Δ = {len(current_delta)} AtomicDeltas")

  while True:
    logger.log_info(f"New DD Iter with n = {n} and Δ = {len(current_delta)} AtomicDeltas remaining")

    if len(current_delta) == 1:
      break

    # 2.
    deltas: list[Delta] = split_delta(current_delta, n)
    compliments: list[Delta] = list(map(lambda d: current_delta.difference(d), deltas))
    logger.log_info(
      f"Splitting Δ to Δi: {list(map(lambda d: len(d), deltas))} and ∇i: {list(map(lambda d: len(d), compliments))}"
    )
    found_buggy: bool = False

    for delta in deltas:
      if len(delta) == 0:
        continue
      reduce_info = apply_delta(delta, empty_reduce_info)
      if reduce_info not in result_cache:
        result_cache[reduce_info] = test(replace(opts, reduce_info=reduce_info))
      if result_cache[reduce_info] == TestResult.BUGGY:
        # 3.a.
        current_delta = delta
        n = 2
        found_buggy = True
        break
    if found_buggy:
      continue

    for delta in compliments:
      if len(delta) == 0:
        continue
      reduce_info = apply_delta(delta, empty_reduce_info)
      if reduce_info not in result_cache:
        result_cache[reduce_info] = test(replace(opts, reduce_info=reduce_info))
      if result_cache[reduce_info] == TestResult.BUGGY:
        # 3.b.
        current_delta = delta
        n = n - 1
        found_buggy = True
        break
    if found_buggy:
      continue

    # 3.c
    if all(map(lambda d: len(d) <= 1, deltas)) and all(map(lambda d: len(d) <= 1, compliments)):
      break
    n = n * 2

  logger.log_info(f"Finished DD with Δ = {len(current_delta)}")
  return apply_delta(current_delta, empty_reduce_info)


# Applyu Bisection to the program generation. For each function and each block independintly bisect rule application
def bisect(topts: TestOpts) -> ReduceInfo:
  init_reduce_info: ReduceInfo = topts.reduce_info

  curr_reduce_info: ReduceInfo = init_reduce_info.copy()
  for function_name, header_label, rule_count in init_reduce_info.items():
    if rule_count <= 0:
      continue

    curr_bisect: int = rule_count // 2
    last_buggy_size = rule_count
    while curr_bisect != last_buggy_size:
      logger.log_info(
        f"Bisecting [{function_name}, {header_label}] To: {curr_bisect}, Candidate {last_buggy_size}"
      )
      curr_bisect_size = max(1, (last_buggy_size - curr_bisect) // 2)
      curr_reduce_info[(function_name, header_label)] = curr_bisect
      test_result: TestResult = test(replace(topts, reduce_info=curr_reduce_info))
      if test_result == TestResult.BUGGY:
        last_buggy_size = curr_bisect
        curr_bisect -= curr_bisect_size
      else:
        curr_bisect += curr_bisect_size

    logger.log_info(f"Bisected: [{function_name}, {header_label}]: to {last_buggy_size}")
    curr_reduce_info[(function_name, header_label)] = last_buggy_size

  return curr_reduce_info


def main():
  parser: ArgumentParser = ArgumentParser(
    "ryreduce", description="Tool for reducing whole programs generated by RyLink, based on a seed"
  )

  parser.add_argument(
    "test",
    type=str,
    help="Likeness test should take the program folder as the first argument and return 0 if the Bug occures and any other value if not",
  )

  parser.add_argument(
    "-a",
    "--args",
    type=str,
    default="",
    help="Additional Arguments for the likeness test.",
  )

  parser.add_argument(
    "-b",
    "--binary",
    type=str,
    default="./build/bin/rylink",
    help="Path to the RyLink Binary (default: ./build/bin/rylink)",
  )

  parser.add_argument(
    "-s",
    "--seed",
    type=int,
    default=-1,
    help="Seed of the target RyLink program",
  )

  parser.add_argument(
    "-n",
    "--sno",
    type=int,
    default=0,
    help="Sample number of the target RyLink program",
  )

  parser.add_argument(
    "-l",
    "--limit",
    type=int,
    default=1,
    help="Original Limit value RyLink ran with (default: 1)",
  )

  parser.add_argument(
    "-i",
    "--input",
    type=str,
    default="generated",
    help="Directory to read the input function files (default: generated)",
  )

  parser.add_argument(
    "-r",
    "--rule",
    type=int,
    default=-1,
    help="Number of rule applications per generated block (default: used RyLinks default (100))",
  )

  parser.add_argument(
    "-e",
    "--extra",
    type=str,
    default="",
    help="Extra flags for RyLink",
  )

  parser.add_argument(
    "-o",
    "--outdir",
    type=str,
    default="reduce",
    help="Directory to store the output of the reduction (default: 'reduce')",
  )

  parser.add_argument(
    "-v",
    "--verbose",
    action="store_true",
    default=False,
    help="Enables Info level Logging (default: False)",
  )

  parser.add_argument(
    "--logger",
    type=str,
    default="stderr",
    help="Sets to logger output file. May be stdout, stderr or any valid filepath (default: stderr)",
  )

  args = parser.parse_args()
  assert args.seed >= 0, "A seed must be provided"

  if args.verbose:
    logger.set_verbose(True)

  if args.logger == "stdout":
    logger.set_out(sys.stdout)
  elif args.logger == "stderr":
    logger.set_out(sys.stderr)
  else:
    logger.set_out(Path(args.logger).open())

  if args.rule >= 0:
    rule: Optional[int] = args.rule
  else:
    rule: Optional[int] = None

  if args.extra != "":
    extra: Optional[str] = args.extra
  else:
    extra: Optional[str] = None

  if args.args != "":
    test_args: Optional[str] = args.args
  else:
    test_args: Optional[str] = None

  indir: Path = Path(args.input).resolve().absolute()
  popts: ProgGenOptions = ProgGenOptions(
    bin=args.binary,
    uuid=next_uuid(),
    indir=indir,
    limit=args.limit,
    sno=args.sno,
    seed=args.seed,
    rule=rule,
    extra=extra,
  )

  logger.log_info(f"========== Working UUID {popts.uuid} ==========")

  outdir: Path = Path(args.outdir).resolve().absolute()

  logger.log_info("===== Getting RuleInfo output =====")
  reduce_info: ReduceInfo = get_reduce_info(popts, outdir, args.sno)
  logger.log_info("Success")

  # validate likeness test
  logger.log_info("===== Validating likeness test =====")
  topts: TestOpts = TestOpts(popts, outdir, reduce_info, args.test, test_args)
  if test(topts) != TestResult.BUGGY:
    logger.log_warning("Likeness Test returns 1 on unreduced Program")
    get_final_reduction(popts, outdir, reduce_info)
    return 1
  logger.log_info("Passed")

  # validate bug freeness without any rule application
  logger.log_info("===== Checking Empty Reduce Info =====")
  empty_reduce_info: ReduceInfo = reduce_info.get_empty()
  if test(replace(topts, reduce_info=empty_reduce_info)) == TestResult.BUGGY:
    logger.log_warning("Likeness Test returns 0 on fully reduced Program")
    get_final_reduction(popts, outdir, empty_reduce_info)
    return 0
  logger.log_info("Passed")

  logger.log_info("===== Delta Debugging: =====")
  reduce_info = delta_debug(topts)
  logger.log_info("===== Bisecting: =====")
  reduce_info = bisect(replace(topts, reduce_info=reduce_info))
  get_final_reduction(popts, outdir, reduce_info)


logger: Logger = Logger()
if __name__ == "__main__":
  main()
