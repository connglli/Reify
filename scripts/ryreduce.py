import json
import random
import datetime
import shutil
import sys
import copy
import cmdline
import shlex
from argparse import ArgumentParser
from pathlib import Path
from dataclasses import dataclass, replace
from typing import Optional, TextIO, Tuple, Dict
from enum import Enum

class Logger:
  verbose: bool
  outFile: TextIO = sys.stderr

  
  def setVerbose(self: Logger, verbose: bool): self.verbose = verbose
  def setOut(self: Logger, outFile: TextIO): self.outFile = outFile

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
      file=self.outFile,
    )

  def logError(self: Logger, msg: str):
    self.__print(title="Error", msg=msg, color="red", flush=True)

  def logWarning(self: Logger, msg: str):
    self.__print(title="Warning", msg=msg, color="yellow")

  def logInfo(self: Logger, msg: str):
    if (not self.verbose): return
    self.__print(title="Info", msg=msg, color="blue")

@dataclass
class ProgGenOptions:
  bin: str  # Path to the rylink executable
  uuid: str  # Primary ID for the newly generated program
  indir: Path  # Directory to read the input function files
  limit: int  # The maximum number of programs to generate (0 means unlimited)
  sno: int # Target Sample Number
  seed: int  # Seed for the random number generator
  ruleInfo: bool = False  # Whenever to output the rule info json
  reduceMode: Optional[Path] = None # path to the reduce mode file
  rule: Optional[int] = None # Number of Rules applied in each block
  extra: Optional[str] = None  # Extra options to control the program generation process

class ReduceInfo:
  sno: int
  info: list[Tuple[str, str, int]]
  __bitvec: int

  def __init__(self: ReduceInfo, sno: int):
    self.info = []
    self.__bitvec = 0
    self.sno = sno

  def items(self: ReduceInfo): return self.info

  def add(self: ReduceInfo, functionName: str, headerBlock: str, ruleCount: int):
    self.__bitvec |= (1 << len(self.info))
    self.info.append((functionName, headerBlock, ruleCount))

  def copy(self: ReduceInfo) -> ReduceInfo:
    return copy.deepcopy(self)

  def getEmpty(self: ReduceInfo) -> ReduceInfo:
    empty: ReduceInfo = self.copy()
    for i in range(len(empty.info)):
      empty.info[i] = (empty.info[i][0], empty.info[i][1], 0)
    empty.__bitvec = 0
    return empty

  @staticmethod
  def fromJson(file: Path) -> ReduceInfo:
    reduceInfoJson: Dict = json.load(file.open())
    reduceInfo: ReduceInfo = ReduceInfo(reduceInfoJson["sno"]);
    for function in reduceInfoJson["functions"]:
      functionName: str = function["name"]
      for block in function["blocks"]:
        headerLabel: str = block["headerLabel"]
        reduceInfo.add(functionName, headerLabel, block["targetRuleCount"])
    return reduceInfo

  def toJson(self: ReduceInfo, path: Path):
    reduceInfoJson: Dict = {}
    reduceInfoJson["functions"] = []
    seenFunctions: Dict[str, int] = {}
    count: int = 0
    reduceInfoJson["sno"] = self.sno
    for [function, headerBlock, ruleCount] in self.info:
      if (seenFunctions.__contains__(function)):
        reduceInfoJson["functions"][seenFunctions[function]]["blocks"].append({ "headerLabel": headerBlock, "targetRuleCount": ruleCount })
      else:
        seenFunctions[function] = count
        count += 1
        reduceInfoJson["functions"].append({ "name": function, "blocks": [{ "headerLabel": headerBlock, "targetRuleCount": ruleCount}]})
    json.dump(reduceInfoJson, open(path, "w"));

  # Bitvec is unique for all ReduceInfos r1, r2 s.t. r1 != r2, see __eq__
  # and conversly ofcourse r1 == r2 => __hash__(r1) == __hash__(r2)__
  def __hash__(self: ReduceInfo):
    return self.__bitvec

  def __getitem__(self: ReduceInfo, key: Tuple[str, str]):
    [functionName, headerBlock] = key
    for item in self.info:
      if (functionName == item[0] and headerBlock == item[1]):
        return item[2]

  def __setitem__(self: ReduceInfo, key: Tuple[str, str], ruleCount: int):
    [functionName, headerBlock] = key
    for i in range(len(self.info)):
      if (functionName == self.info[i][0] and headerBlock == self.info[i][1]):
        self.info[i] = (self.info[i][0], self.info[i][1], ruleCount)
        if (ruleCount == 0):
          self.__bitvec &= ~(1 << i)
        else:
          self.__bitvec |= 1 << i
        break;
    else: 
      self.add(functionName, headerBlock, ruleCount)
      self.__bitvec |= (1 << len(self.info))
      self.info.append((functionName, headerBlock, ruleCount))

  def __delitem__(self: ReduceInfo, _key: Tuple[str, str]):
    assert True, "Cannot Delete from RuleInfo"

  # This is a shallow equallity e.g. only over if a block has ruleCount == 0 or not
  def __eq__(self: ReduceInfo, otherObj: object):
    if not isinstance(otherObj, ReduceInfo): return False
    other: ReduceInfo = otherObj
    return self.__bitvec == other.__bitvec

def nextUuid():
  keywords = "0123456789abcdefghijklmnopqrstuvwxyz"
  return "".join(random.choices(keywords, k=6))

def makeFileName(prefix: str):
  import os
  return prefix + str(os.getpid())

def runRyLink(popts: ProgGenOptions):
  cmd: list[str] = [
    popts.bin,
    "-i", str(popts.indir),
    "-l", str(popts.limit),
    "-s", str(popts.seed),
    "-n", str(popts.sno),
  ]
  cmd += [popts.uuid]
  if popts.rule:
    cmd += str(popts.rule)
  if popts.ruleInfo:
    cmd += ["--Xrule-info"]
  if popts.reduceMode:
    cmd += ["--Xreduce-mode", str(popts.reduceMode)]
  if popts.extra:
    cmd += shlex.split(popts.extra)

  cmdline.check_run(cmd)

def getReduceInfo(popts: ProgGenOptions, outdir: Path, sno: int) -> ReduceInfo:
  reduceInfoOpts: ProgGenOptions = replace(popts, ruleInfo=True, reduceMode=None);

  try: runRyLink(reduceInfoOpts)
  except: 
    logger.logError("Failed to get Reduce Info")
    exit(1)


  reduceInfoFile : Path = popts.indir / ("prog_" + popts.uuid + "_" + str(sno)) / "ruleinfo.jsonl"
  reduceInfo = ReduceInfo.fromJson(reduceInfoFile)

  progDir: str = ("prog_" + popts.uuid + "_" + str(popts.sno));
  shutil.rmtree(str(popts.indir / progDir))

  return reduceInfo

def getFinalReduction(popts : ProgGenOptions, outdir: Path, reduceInfo: ReduceInfo):
  reduceModePath: Path = Path("/tmp/" + makeFileName("reduce_file.jsonl"));
  reduceInfo.toJson(reduceModePath)

  reduceInfoOpts: ProgGenOptions = replace(popts, ruleInfo=True, reduceMode=reduceModePath);
  try: runRyLink(reduceInfoOpts)
  except: 
    logger.logError("Failed to get final Reduction")
    exit(1)

  progDir: str = ("prog_" + popts.uuid + "_" + str(popts.sno));
  shutil.move(str(popts.indir / progDir), str(outdir / progDir))

class TestResult(Enum):
  BUGGY = True
  FIXED = False

@dataclass
class TestOpts:
  popts: ProgGenOptions;
  outdir: Path;
  reduceInfo: ReduceInfo;
  test: str;
  args: Optional[str] = None;

def Test(opts: TestOpts) -> TestResult:
  reduceModePath: Path = Path("/tmp/" + makeFileName("reduce_file.jsonl"));
  opts.reduceInfo.toJson(reduceModePath)

  reduceModeOpts: ProgGenOptions = replace(opts.popts, ruleInfo=False, reduceMode=str(reduceModePath))
  try: runRyLink(reduceModeOpts)
  except: 
    logger.logError("Failed to run Test")
    exit(1)

  progDirName: str = "prog_" + opts.popts.uuid + "_" + str(opts.popts.sno)
  progPath: Path = opts.popts.indir / progDirName;
  cmd: list[str] = [opts.test, str(progPath)]
  if opts.args:
    cmd += shlex.split(opts.args)
  
  ret: int = cmdline.get_ret(cmd);

  shutil.rmtree(progPath)

  return TestResult(ret == 0)

# DeltaDebug Helpers:

@dataclass(eq=True, frozen=True)
class AtomicDelta:
  functionName: str;
  headerLabel: str;
  ruleCount: int;

  def apply(self: AtomicDelta, reduceInfo: ReduceInfo):
    reduceInfo[self.functionName, self.headerLabel] = self.ruleCount

Delta = set[AtomicDelta]

def applyDelta(delta: Delta, reduceInfo: ReduceInfo) -> ReduceInfo:
  res : ReduceInfo = reduceInfo.copy()
  for c in delta:
      c.apply(res)
  return res;

def splitDelta(delta: Delta, n: int) -> list[Delta]:
  deltas: list[set[AtomicDelta]] = []
  currDelta: list[AtomicDelta] = list(delta)
  start = 0
  for i in range(n):
    newDelta: list[AtomicDelta] = currDelta[start:int(start + (len(delta) - start) / (n - i))]
    deltas.append(Delta(set(newDelta)))
    start = start + len(newDelta)
  return deltas

def getDeltaFromReduceInfo(reduceInfo: ReduceInfo) -> Delta:
  d: set[AtomicDelta] = set()
  for [functionName, headerLabel, ruleCount] in reduceInfo.items():
    if (ruleCount > 0):
      d.add(AtomicDelta(functionName, headerLabel, ruleCount))
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
def DeltaDebug(
  opts: TestOpts
) -> ReduceInfo:
  emptyReduceInfo: ReduceInfo = opts.reduceInfo.getEmpty();
  entireDelta: Delta = getDeltaFromReduceInfo(opts.reduceInfo);
  assert applyDelta(entireDelta, emptyReduceInfo) == opts.reduceInfo, "entireDelta is not the entire difference between empty- and initReduceInfo"

  resultCache: Dict[ReduceInfo, TestResult] = {}

  # 1.
  n: int = 2
  currentDelta: Delta = entireDelta;
  logger.logInfo(f"Start DD with n = { n } and Δ = { len(currentDelta) } AtomicDeltas")

  while True:
    logger.logInfo(f"New DD Iter with n = { n } and Δ = { len(currentDelta) } AtomicDeltas remaining")

    if (len(currentDelta) == 1): break

    # 2.
    deltas: list[Delta] = splitDelta(currentDelta, n)
    compliments: list[Delta] = list(map(lambda d: currentDelta.difference(d), deltas))
    logger.logInfo(f"Splitting Δ to Δi: { list(map(lambda d: len(d), deltas)) } and ∇i: { list(map(lambda d: len(d), compliments)) }")
    foundBuggy: bool = False

    for delta in deltas:
      if len(delta) == 0: continue
      reduceInfo = applyDelta(delta, emptyReduceInfo)
      if not (reduceInfo in resultCache):
        resultCache[reduceInfo] = Test(replace(opts, reduceInfo=reduceInfo))
      if (resultCache[reduceInfo] == TestResult.BUGGY):
        # 3.a.
        currentDelta = delta
        n = 2
        foundBuggy = True
        break
    if foundBuggy: continue

    for delta in compliments:
      if len(delta) == 0: continue
      reduceInfo = applyDelta(delta, emptyReduceInfo)
      if not (reduceInfo in resultCache):
        resultCache[reduceInfo] = Test(replace(opts, reduceInfo=reduceInfo))
      if (resultCache[reduceInfo] == TestResult.BUGGY):
        # 3.b.
        currentDelta = delta
        n = n - 1
        foundBuggy = True
        break
    if foundBuggy: continue

    # 3.c
    if (all(map(lambda d: len(d) <= 1, deltas)) and all(map(lambda d: len(d) <= 1, compliments))): break;
    n = n * 2

  logger.logInfo(f"Finished DD with Δ = { len(currentDelta) }")
  return applyDelta(currentDelta, emptyReduceInfo);

# Applyu Bisection to the program generation. For each function and each block independintly bisect rule application
def Bisect(topts: TestOpts) -> ReduceInfo:
  initReduceInfo: ReduceInfo = topts.reduceInfo

  currReduceInfo: ReduceInfo = initReduceInfo.copy()
  for functionName, headerLabel, ruleCount in initReduceInfo.items():
    if ruleCount <= 0: continue

    currBisect: int = ruleCount // 2
    lastBuggySize = ruleCount
    while currBisect != lastBuggySize:
      logger.logInfo(f"Bisecting [{functionName}, {headerLabel}] To: {currBisect}, Candidate {lastBuggySize}")
      currBisectSize = max(1, (lastBuggySize - currBisect) // 2)
      currReduceInfo[(functionName, headerLabel)] = currBisect
      testResult : TestResult = Test(replace(topts, reduceInfo=currReduceInfo))
      if (testResult == TestResult.BUGGY):
        lastBuggySize = currBisect
        currBisect -= currBisectSize
      else:
        currBisect += currBisectSize

    logger.logInfo(f"Bisected: [{functionName}, {headerLabel}]: to {lastBuggySize}")
    currReduceInfo[(functionName, headerLabel)] = lastBuggySize

  return currReduceInfo

def main(): 
  parser: ArgumentParser = ArgumentParser("ryreduce", description="Tool for reducing whole programs generated by RyLink, based on a seed")

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
    action='store_true',
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

  if (args.verbose):
    logger.setVerbose(True)

  if (args.logger == "stdout"):
    logger.setOut(sys.stdout)
  elif (args.logger == "stderr"):
    logger.setOut(sys.stderr)
  else:
    logger.setOut(Path(args.logger).open())

  if (args.rule >= 0):
    rule: Optional[int] = args.rule;
  else:
    rule: Optional[int] = None;

  if (args.extra != ""):
    extra: Optional[str] = args.extra;
  else:
    extra: Optional[str] = None;

  if (args.args!= ""):
    testArgs: Optional[str] = args.args;
  else:
    testArgs: Optional[str] = None;

  indir: Path = Path(args.input).resolve().absolute()
  popts: ProgGenOptions = ProgGenOptions(
    bin=args.binary,
    uuid=nextUuid(),
    indir=indir,
    limit=args.limit,
    sno=args.sno,
    seed=args.seed,
    rule=rule,
    extra=extra,
  )

  logger.logInfo(f"========== Working UUID {popts.uuid} ==========")

  outdir: Path = Path(args.outdir).resolve().absolute()

  logger.logInfo("===== Getting RuleInfo output =====")
  reduceInfo: ReduceInfo = getReduceInfo(popts, outdir, args.sno);
  logger.logInfo("Success")

  # validate likeness test
  logger.logInfo("===== Validating likeness test =====")
  topts: TestOpts = TestOpts(popts, outdir, reduceInfo, args.test, testArgs);
  if (Test(topts) != TestResult.BUGGY):
    logger.logWarning("Likeness Test returns 1 on unreduced Program")
    getFinalReduction(popts, outdir, reduceInfo)
    return 1
  logger.logInfo("Passed")

  # validate bug freeness without any rule application
  logger.logInfo("===== Checking Empty Reduce Info =====")
  emptyReduceInfo: ReduceInfo = reduceInfo.getEmpty();
  if (Test(replace(topts, reduceInfo=emptyReduceInfo)) == TestResult.BUGGY):
    logger.logWarning("Likeness Test returns 0 on fully reduced Program")
    getFinalReduction(popts, outdir, emptyReduceInfo)
    return 0
  logger.logInfo("Passed")

  logger.logInfo("===== Delta Debugging: =====")
  reduceInfo = DeltaDebug(topts);

  logger.logInfo("===== Bisecting: =====")
  reduceInfo = Bisect(replace(topts, reduceInfo=reduceInfo));

  getFinalReduction(popts, outdir, reduceInfo)

logger: Logger = Logger()
if __name__ == "__main__":
  main()
