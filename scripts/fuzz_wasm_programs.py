#!/usr/bin/env python3
"""
Continuous WASM fuzzing campaign.
Runs forever (or until killed), generating 2500 programs per batch and testing
them across multiple runtimes against expected outputs from .jsonl mappings.

Speed optimisations:
  - wasmtime: Python API, Engine/Store reused across the whole batch
  - wasmer:   Python API (wasmer + wasmer-compiler-cranelift), no subprocess
  - wasm3/iwasm/node: CLI subprocesses, run in parallel via ThreadPoolExecutor
"""

import os
import re
import json
import time
import shutil
import sqlite3
import subprocess
import tempfile
import textwrap
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
from datetime import datetime

# ─── Paths ──────────────────────────────────────────────────────────────────────
SCRIPT_DIR    = Path(__file__).resolve().parent
BASE_DIR      = SCRIPT_DIR.parent
PROGRAMS_DIR  = BASE_DIR / "generated" / "programs"
MAPPINGS_DIR  = BASE_DIR / "generated" / "mappings"
FUNCS_DIR     = BASE_DIR / "generated" / "wasm"
GENERATED_DIR = BASE_DIR / "generated"
BUGS_DIR      = BASE_DIR / "wasm_bugs"
EXEC_PATHS    = BASE_DIR / "exec_paths.json"
DB_PATH       = BASE_DIR / "fuzz_results.db"

# ─── Config ─────────────────────────────────────────────────────────────────────
BATCH_SIZE    = 2500
UUID_PREFIX   = "sriya"
VERBOSE       = False  # per-function OK/FAIL lines — turn on for debugging only
SANITY_CHECK  = True   # print raw runtime output for 1 program per batch
NUM_WORKERS   = 12     # parallel workers for program execution

# ─── i32 sign correction ────────────────────────────────────────────────────────
def to_i32(x):
    return (int(x) + 2**31) % 2**32 - 2**31

# ═══════════════════════════════════════════════════════════════════════════════
# DATABASE
# ═══════════════════════════════════════════════════════════════════════════════

def init_db():
    con = sqlite3.connect(DB_PATH)
    cur = con.cursor()
    cur.executescript("""
        CREATE TABLE IF NOT EXISTS batches (
            batch_id             INTEGER PRIMARY KEY AUTOINCREMENT,
            uuid                 TEXT,
            started_at           TEXT,
            finished_at          TEXT,
            programs_generated   INTEGER,
            fgen_funcs_ok        INTEGER,
            fgen_avg_time_s      REAL,
            pgen_elapsed_s       REAL,
            pgen_avg_per_prog_ms REAL
        );
        CREATE TABLE IF NOT EXISTS func_stats (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            batch_id     INTEGER,
            func_name    TEXT,
            total_blocks INTEGER,
            path_length  INTEGER
        );
        CREATE TABLE IF NOT EXISTS programs (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            batch_id  INTEGER,
            filename  TEXT,
            num_funcs INTEGER
        );
        CREATE TABLE IF NOT EXISTS func_results (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            batch_id  INTEGER,
            program   TEXT,
            func_name TEXT,
            runtime   TEXT,
            status    TEXT,
            detail    TEXT,
            inp       TEXT,
            exp       TEXT
        );
        CREATE TABLE IF NOT EXISTS sanity_checks (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            batch_id     INTEGER,
            program      TEXT,
            func_name    TEXT,
            inp          TEXT,
            exp          TEXT,
            wasmtime_raw TEXT,
            wasmer_raw   TEXT,
            wasm3_raw    TEXT,
            iwasm_raw    TEXT,
            node_raw     TEXT
        );
    """)
    con.commit()
    return con

# ═══════════════════════════════════════════════════════════════════════════════
# GENERATION
# ═══════════════════════════════════════════════════════════════════════════════

def clear_generated():
    if GENERATED_DIR.exists():
        shutil.rmtree(GENERATED_DIR)
    GENERATED_DIR.mkdir(parents=True)
    (GENERATED_DIR / "programs").mkdir()
    (GENERATED_DIR / "mappings").mkdir()
    print("  [gen] Cleared ./generated/")

def run_fgen() -> tuple[int, float]:
    env = os.environ.copy()
    env.update({
        "FGEN_GEN_WASM": "true",
        "FGEN_GEN_SEXP": "true",
        "FGEN_INJ_UBS":  "true",
        "FGEN_ALL_OPS":  "true",
        "FGEN_LIMIT":    "200",
    })
    print("  [gen] Running fgen (make gen-func-set) ...")
    t0 = time.time()
    proc = subprocess.run(
        ["make", "gen-func-set"],
        cwd=BASE_DIR, env=env, capture_output=True, text=True,
    )
    elapsed = time.time() - t0
    times = [float(m.group(1))
             for line in (proc.stdout + proc.stderr).splitlines()
             for m in [re.search(r"SUCC \(time=([\d.]+)s\)", line)] if m]
    avg = sum(times) / len(times) if times else 0.0
    print(f"  [gen] fgen done in {elapsed:.1f}s — {len(times)} funcs OK, avg {avg:.2f}s each")
    return len(times), avg

def run_pgen(uuid: str) -> tuple[int, float]:
    print(f"  [gen] Running pgen (limit={BATCH_SIZE}, uuid={uuid}) ...")
    t0 = time.time()
    subprocess.run(
        [str(BASE_DIR / "build" / "bin" / "pgen"),
         "--wasm", "--input", str(GENERATED_DIR),
         "--limit", str(BATCH_SIZE), "--uuid", uuid],
        cwd=BASE_DIR, capture_output=True, text=True,
    )
    elapsed = time.time() - t0
    count   = len(list(PROGRAMS_DIR.glob("*.wat")))
    avg_ms  = (elapsed / count * 1000) if count else 0.0
    print(f"  [gen] pgen done in {elapsed:.1f}s — {count} programs, avg {avg_ms:.1f}ms each")
    return count, elapsed

def refresh_exec_paths():
    EXEC_PATHS.write_text("{}\n")

# ═══════════════════════════════════════════════════════════════════════════════
# BB STATS
# ═══════════════════════════════════════════════════════════════════════════════

def parse_bb_comment(wat_path: Path) -> tuple[int, int] | None:
    try:
        with open(wat_path) as f:
            first_line = f.readline()
        if not first_line.startswith(";;"):
            return None
        bb_nums = [int(m) for m in re.findall(r"BB(\d+)", first_line)]
        if not bb_nums:
            return None
        return max(bb_nums) + 1, len(bb_nums)
    except Exception:
        return None

def collect_func_stats(batch_id: int, cur: sqlite3.Cursor):
    if not FUNCS_DIR.exists():
        return
    rows = []
    for wat in FUNCS_DIR.glob("*.wat"):
        result = parse_bb_comment(wat)
        if result:
            total_blocks, path_length = result
            rows.append((batch_id, wat.stem, total_blocks, path_length))
    if rows:
        cur.executemany(
            "INSERT INTO func_stats (batch_id, func_name, total_blocks, path_length) "
            "VALUES (?,?,?,?)", rows
        )
    print(f"  [gen] BB stats collected for {len(rows)} functions")

# ═══════════════════════════════════════════════════════════════════════════════
# WAT → WASM CONVERSION
# ═══════════════════════════════════════════════════════════════════════════════

_wat2wasm_ok = None

def wat2wasm_available():
    global _wat2wasm_ok
    if _wat2wasm_ok is None:
        _wat2wasm_ok = shutil.which("wat2wasm") is not None
    return _wat2wasm_ok

def convert_to_wasm(wat_path: Path) -> Path | None:
    if not wat2wasm_available():
        return None
    wasm_path = wat_path.with_suffix(".wasm")
    r = subprocess.run(
        ["wat2wasm", str(wat_path), "-o", str(wasm_path)],
        capture_output=True, text=True
    )
    return wasm_path if r.returncode == 0 else None

# ═══════════════════════════════════════════════════════════════════════════════
# RUNTIME RUNNERS
# ═══════════════════════════════════════════════════════════════════════════════

_NODE_HARNESS = textwrap.dedent("""\
    const fs = require('fs');
    const buf = fs.readFileSync('{wasm_path}');
    const mod = new WebAssembly.Module(buf);
    const inst = new WebAssembly.Instance(mod);
    const fn = inst.exports['{func_name}'];
    const result = fn({args});
    if (result === null || result === undefined) {{
        console.log(JSON.stringify([]));
    }} else if (typeof result === 'object' && !Array.isArray(result)) {{
        const keys = Object.keys(result).sort((a,b) => Number(a)-Number(b));
        console.log(JSON.stringify(keys.map(k => Number(result[k]))));
    }} else if (Array.isArray(result)) {{
        console.log(JSON.stringify(result.map(Number)));
    }} else {{
        console.log(JSON.stringify([Number(result)]));
    }}
""")

# Check once at startup whether the wasmer Python package is available
try:
    from wasmer import engine as wasmer_engine, Store as WasmerStore, Module as WasmerModule, Instance as WasmerInstance
    from wasmer_compiler_cranelift import Compiler as WasmerCompiler
    _WASMER_PYTHON = True
except ImportError:
    _WASMER_PYTHON = False

def _run_raw(args: list[str], timeout=10) -> tuple[str, str, int]:
    try:
        r = subprocess.run(args, capture_output=True, text=True, timeout=timeout)
        return r.stdout, r.stderr, r.returncode
    except subprocess.TimeoutExpired:
        return "", "timeout", -1
    except Exception as e:
        return "", str(e), -1

def run_wasmtime(wat_path: Path, func_name: str, inputs: list[int],
                 wt_engine, wt_store_factory) -> list[int] | str:
    """Use shared wasmtime Engine, create a fresh Store per call."""
    try:
        from wasmtime import Store, Module, Instance
        store  = wt_store_factory()
        module = Module.from_file(wt_engine, str(wat_path))
        inst   = Instance(store, module, [])
        fn     = inst.exports(store).get(func_name)
        if fn is None:
            return "ERROR:export_missing"
        result = fn(store, *inputs)
        if isinstance(result, (list, tuple)):
            return [to_i32(x) for x in result]
        return [to_i32(result)]
    except Exception as e:
        msg = str(e)
        return "TRAP:" + msg[:120] if "trap" in msg.lower() else "ERROR:" + msg[:120]

def run_wasmer_python(wat_path: Path, func_name: str, inputs: list[int]) -> list[int] | str:
    """Use wasmer Python API — reads .wat directly."""
    try:
        store  = WasmerStore(wasmer_engine.JIT(WasmerCompiler))
        wasm_bytes = subprocess.run(
            ["wat2wasm", str(wat_path), "--output=-"],
            capture_output=True).stdout
        module = WasmerModule(store, wasm_bytes)
        inst   = WasmerInstance(module, {})
        fn     = getattr(inst.exports, func_name, None)
        if fn is None:
            return "ERROR:export_missing"
        result = fn(*inputs)
        if isinstance(result, (list, tuple)):
            return [to_i32(x) for x in result]
        return [to_i32(result)]
    except Exception as e:
        msg = str(e)
        return "TRAP:" + msg[:120] if "trap" in msg.lower() else "ERROR:" + msg[:120]

def run_wasmer_cli(wat_path: Path, func_name: str, inputs: list[int]) -> list[int] | str:
    args_list = [str(x) for x in inputs]
    stdout, stderr, rc = _run_raw(
        ["wasmer", "run", str(wat_path), "--invoke", func_name, "--"] + args_list)
    if rc != 0:
        return "TRAP:" + (stdout + stderr).strip()[:120]
    nums = re.findall(r"-?\d+", stdout)
    if not nums:
        return "ERROR:no_output"
    return [to_i32(int(n)) for n in nums]

def run_wasm3(wasm_path: Path, func_name: str, inputs: list[int]) -> list[int] | str:
    args_list = [str(x) for x in inputs]
    stdout, stderr, rc = _run_raw(
        ["wasm3", "--func", func_name, str(wasm_path)] + args_list)
    if rc != 0:
        return "TRAP:" + (stdout + stderr).strip()[:120]
    combined = stdout + stderr
    nums = re.findall(r"Result:\s*(-?\d+)", combined)
    if not nums:
        return f"ERROR:no_output({combined.strip()[:80]})"
    return [to_i32(int(n)) for n in nums]

def run_iwasm(wasm_path: Path, func_name: str, inputs: list[int]) -> list[int] | str:
    args_list = [str(x) for x in inputs]
    stdout, stderr, rc = _run_raw(
        ["iwasm", "-f", func_name, str(wasm_path)] + args_list)
    if rc != 0:
        return "TRAP:" + (stdout + stderr).strip()[:120]
    results = []
    for token in stdout.strip().split(","):
        m = re.match(r"\s*(0x[0-9a-fA-F]+|-?\d+):i32", token.strip())
        if m:
            results.append(to_i32(int(m.group(1), 0)))
    if not results:
        return "ERROR:no_output"
    return results

def run_node(wasm_path: Path, func_name: str, inputs: list[int]) -> list[int] | str:
    script = _NODE_HARNESS.format(
        wasm_path=str(wasm_path), func_name=func_name,
        args=", ".join(str(x) for x in inputs))
    with tempfile.NamedTemporaryFile(suffix=".js", mode="w", delete=False) as f:
        f.write(script); tmp = f.name
    try:
        stdout, stderr, rc = _run_raw(["node", tmp])
        if rc != 0:
            return "TRAP:" + stderr.strip()[:120]
        return [to_i32(x) for x in json.loads(stdout.strip())]
    except Exception as e:
        return "ERROR:" + str(e)[:120]
    finally:
        os.unlink(tmp)

def run_runtime(rt_name: str, wat_path: Path, wasm_path: Path | None,
                func_name: str, inputs: list[int],
                wt_engine=None, wt_store_factory=None) -> list[int] | str:
    if rt_name == "wasmtime":
        return run_wasmtime(wat_path, func_name, inputs, wt_engine, wt_store_factory)
    elif rt_name == "wasmer":
        if _WASMER_PYTHON:
            return run_wasmer_python(wat_path, func_name, inputs)
        else:
            return run_wasmer_cli(wat_path, func_name, inputs)
    elif rt_name == "wasm3":
        return run_wasm3(wasm_path, func_name, inputs) if wasm_path else "ERROR:no_wasm"
    elif rt_name == "iwasm":
        return run_iwasm(wasm_path, func_name, inputs) if wasm_path else "ERROR:no_wasm"
    elif rt_name == "node":
        return run_node(wasm_path, func_name, inputs) if wasm_path else "ERROR:no_wasm"
    return "ERROR:unknown_runtime"

# ═══════════════════════════════════════════════════════════════════════════════
# SANITY CHECK
# ═══════════════════════════════════════════════════════════════════════════════

def run_raw_for_sanity(func_name: str, wat_path: Path, wasm_path: Path | None,
                       inp: list[int], wt_engine, wt_store_factory) -> dict[str, str]:
    args_list = [str(x) for x in inp]
    out = {}

    # wasmtime via Python API
    try:
        from wasmtime import Store, Module, Instance
        store  = wt_store_factory()
        module = Module.from_file(wt_engine, str(wat_path))
        inst   = Instance(store, module, [])
        fn     = inst.exports(store).get(func_name)
        result = fn(store, *inp) if fn else None
        if isinstance(result, (list, tuple)):
            out["wasmtime"] = repr([to_i32(x) for x in result])
        elif result is not None:
            out["wasmtime"] = repr([to_i32(result)])
        else:
            out["wasmtime"] = "ERROR:export_missing"
    except Exception as e:
        out["wasmtime"] = f"EXCEPTION:{e}"

    # wasmer
    if _WASMER_PYTHON:
        out["wasmer"] = repr(run_wasmer_python(wat_path, func_name, inp))
    else:
        so, se, _ = _run_raw(["wasmer", "run", str(wat_path), "--invoke", func_name, "--"] + args_list)
        out["wasmer"] = (so + se).strip()

    if wasm_path:
        so, se, _ = _run_raw(["wasm3", "--func", func_name, str(wasm_path)] + args_list)
        out["wasm3"] = (so + se).strip()
        so, se, _ = _run_raw(["iwasm", "-f", func_name, str(wasm_path)] + args_list)
        out["iwasm"] = (so + se).strip()
        script = _NODE_HARNESS.format(
            wasm_path=str(wasm_path), func_name=func_name,
            args=", ".join(str(x) for x in inp))
        with tempfile.NamedTemporaryFile(suffix=".js", mode="w", delete=False) as f:
            f.write(script); tmp = f.name
        so, se, _ = _run_raw(["node", tmp])
        os.unlink(tmp)
        out["node"] = (so + se).strip()
    return out

# ═══════════════════════════════════════════════════════════════════════════════
# HELPERS
# ═══════════════════════════════════════════════════════════════════════════════

def extract_func_names(wat_path: Path) -> list[str]:
    names = []
    with open(wat_path) as f:
        for line in f:
            m = re.search(r'\(export "([^"]+)" \(func \$[^)]+\)\)', line)
            if m:
                names.append(m.group(1))
    return names

def check_runtime_available(name: str) -> bool:
    if name == "wasmtime":
        try:
            from wasmtime import Engine; return True
        except ImportError:
            return False
    if name == "wasmer":
        return _WASMER_PYTHON or shutil.which("wasmer") is not None
    return shutil.which(name) is not None

# ═══════════════════════════════════════════════════════════════════════════════
# PER-PROGRAM WORKER  (runs in thread pool)
# ═══════════════════════════════════════════════════════════════════════════════

def process_program(wat_path: Path, rt_names: list[str],
                    wt_engine, wt_store_factory) -> dict:
    """
    Process a single program: load test cases, run all runtimes, return results.
    Returns a dict with:
      func_names, wasm_path, failures: list of (func_name, rt_name, status, detail, inp, exp)
      prog_failed: bool
      stats: {rt_name: {pass/fail/trap/error: count}}
    """
    func_names = extract_func_names(wat_path)
    if not func_names:
        return {"func_names": [], "wasm_path": None, "failures": [],
                "prog_failed": False, "stats": {}}

    wasm_path = convert_to_wasm(wat_path) if wat2wasm_available() else None

    stats    = {rt: {"pass": 0, "fail": 0, "trap": 0, "error": 0} for rt in rt_names}
    failures = []

    for func_name in func_names:
        jsonl_path = MAPPINGS_DIR / f"{func_name}.jsonl"
        if not jsonl_path.exists():
            continue
        test_cases = []
        try:
            with open(jsonl_path) as f:
                for line in f:
                    if line.strip():
                        d   = json.loads(line)
                        inp = [item["elems"][0] for item in d["ini"]]
                        exp = [to_i32(item["elems"][0]) for item in d["fin"]]
                        test_cases.append((inp, exp))
        except Exception:
            continue
        if not test_cases:
            continue

        for rt_name in rt_names:
            func_passed = True
            status      = "pass"
            detail      = ""
            bad_inp = bad_exp = None

            for inp, exp in test_cases:
                result = run_runtime(rt_name, wat_path, wasm_path, func_name, inp,
                                     wt_engine, wt_store_factory)
                if isinstance(result, str):
                    func_passed = False
                    status  = "trap" if result.startswith("TRAP") else "error"
                    detail  = result
                    bad_inp, bad_exp = inp, exp
                    break
                elif result != exp:
                    func_passed = False
                    status  = "fail"
                    detail  = f"expected={exp} got={result}"
                    bad_inp, bad_exp = inp, exp
                    break

            stats[rt_name][status] += 1
            if not func_passed:
                failures.append((func_name, rt_name, status, detail,
                                  json.dumps(bad_inp), json.dumps(bad_exp)))

    prog_failed = len(failures) > 0
    return {"func_names": func_names, "wasm_path": wasm_path,
            "failures": failures, "prog_failed": prog_failed, "stats": stats}

# ═══════════════════════════════════════════════════════════════════════════════
# BATCH EXECUTION
# ═══════════════════════════════════════════════════════════════════════════════

def run_batch(batch_num: int, con: sqlite3.Connection, rt_names: list[str]):
    uuid    = f"{UUID_PREFIX}_{batch_num}"
    started = datetime.utcnow().isoformat()
    print(f"\n{'═'*62}")
    print(f"  BATCH {batch_num}  |  UUID: {uuid}  |  {started}")
    print(f"{'═'*62}")

    clear_generated()
    refresh_exec_paths()

    fgen_ok, fgen_avg        = run_fgen()
    prog_count, pgen_elapsed = run_pgen(uuid)
    pgen_avg_ms = (pgen_elapsed / prog_count * 1000) if prog_count else 0.0

    cur = con.cursor()
    cur.execute(
        "INSERT INTO batches (uuid, started_at, programs_generated, fgen_funcs_ok, "
        "fgen_avg_time_s, pgen_elapsed_s, pgen_avg_per_prog_ms) VALUES (?,?,?,?,?,?,?)",
        (uuid, started, prog_count, fgen_ok, fgen_avg, pgen_elapsed, pgen_avg_ms)
    )
    batch_id = cur.lastrowid
    collect_func_stats(batch_id, cur)
    con.commit()

    # ── Build shared wasmtime Engine once for the whole batch ───────────────
    from wasmtime import Engine, Store
    wt_engine        = Engine()
    wt_store_factory = lambda: Store(wt_engine)

    # ── Fuzzing ─────────────────────────────────────────────────────────────
    wat_files = sorted(PROGRAMS_DIR.glob("*.wat"))
    print(f"\n  [fuzz] {len(wat_files)} programs × {len(rt_names)} runtimes  "
          f"({', '.join(rt_names)})  workers={NUM_WORKERS}\n")

    batch_stats     = {rt: {"pass": 0, "fail": 0, "trap": 0, "error": 0} for rt in rt_names}
    failed_programs = set()
    sanity_done     = False
    completed       = 0

    with ThreadPoolExecutor(max_workers=NUM_WORKERS) as pool:
        future_to_wat = {
            pool.submit(process_program, wp, rt_names, wt_engine, wt_store_factory): wp
            for wp in wat_files
        }

        for future in as_completed(future_to_wat):
            wat_path = future_to_wat[future]
            completed += 1

            try:
                res = future.result()
            except Exception as e:
                print(f"  [!] Worker error on {wat_path.name}: {e}")
                continue

            # ── Accumulate stats ─────────────────────────────────────────
            for rt_name, counts in res["stats"].items():
                for k, v in counts.items():
                    batch_stats[rt_name][k] += v

            # ── DB inserts ───────────────────────────────────────────────
            cur.execute(
                "INSERT INTO programs (batch_id, filename, num_funcs) VALUES (?,?,?)",
                (batch_id, wat_path.name, len(res["func_names"]))
            )
            for (func_name, rt_name, status, detail, inp_json, exp_json) in res["failures"]:
                cur.execute(
                    "INSERT INTO func_results "
                    "(batch_id, program, func_name, runtime, status, detail, inp, exp) "
                    "VALUES (?,?,?,?,?,?,?,?)",
                    (batch_id, wat_path.name, func_name, rt_name, status, detail,
                     inp_json, exp_json)
                )

            if res["prog_failed"]:
                failed_programs.add(wat_path)

            # ── Sanity check: first completed program ────────────────────
            if SANITY_CHECK and not sanity_done and res["func_names"]:
                func_name = res["func_names"][0]
                jsonl_path = MAPPINGS_DIR / f"{func_name}.jsonl"
                if jsonl_path.exists():
                    with open(jsonl_path) as f:
                        first_line = f.readline()
                    if first_line.strip():
                        d   = json.loads(first_line)
                        inp = [item["elems"][0] for item in d["ini"]]
                        exp = [to_i32(item["elems"][0]) for item in d["fin"]]
                        raw = run_raw_for_sanity(func_name, wat_path, res["wasm_path"],
                                                 inp, wt_engine, wt_store_factory)
                        print(f"  [SANITY] {wat_path.name} / {func_name}")
                        print(f"    inp={inp}")
                        print(f"    exp={exp}")
                        for rn, raw_out in raw.items():
                            print(f"    {rn:10s} raw: {raw_out!r}")
                        cur.execute(
                            "INSERT INTO sanity_checks "
                            "(batch_id, program, func_name, inp, exp, "
                            "wasmtime_raw, wasmer_raw, wasm3_raw, iwasm_raw, node_raw) "
                            "VALUES (?,?,?,?,?,?,?,?,?,?)",
                            (batch_id, wat_path.name, func_name,
                             json.dumps(inp), json.dumps(exp),
                             raw.get("wasmtime",""), raw.get("wasmer",""),
                             raw.get("wasm3",""),    raw.get("iwasm",""),
                             raw.get("node",""))
                        )
                        sanity_done = True

            if completed % 100 == 0:
                print(f"  [fuzz] {completed}/{len(wat_files)} ...")

        con.commit()

    # ── Move failures ────────────────────────────────────────────────────────
    if failed_programs:
        bug_batch_dir = BUGS_DIR / f"batch_{batch_num}"
        bug_batch_dir.mkdir(parents=True, exist_ok=True)
        for p in failed_programs:
            shutil.copy2(p, bug_batch_dir / p.name)
            wasm = p.with_suffix(".wasm")
            if wasm.exists():
                shutil.copy2(wasm, bug_batch_dir / wasm.name)
        print(f"\n  [bugs] {len(failed_programs)} programs → {bug_batch_dir}")

    # ── Summary ──────────────────────────────────────────────────────────────
    finished = datetime.utcnow().isoformat()
    cur.execute("UPDATE batches SET finished_at=? WHERE batch_id=?", (finished, batch_id))
    con.commit()

    print(f"\n  {'─'*58}")
    print(f"  BATCH {batch_num} SUMMARY")
    print(f"  {'─'*58}")
    print(f"  fgen: {fgen_ok} funcs OK, avg {fgen_avg:.2f}s each")
    print(f"  pgen: {prog_count} programs in {pgen_elapsed:.1f}s ({pgen_avg_ms:.1f}ms/prog)")
    print(f"  bugs: {len(failed_programs)} programs flagged")
    for rt in rt_names:
        s     = batch_stats[rt]
        total = sum(s.values())
        print(f"  {rt:10s}  pass={s['pass']}  fail={s['fail']}  "
              f"trap={s['trap']}  err={s['error']}  (/{total})")
    print(f"  {'─'*58}\n")

# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════════

RUNTIME_NAMES = ["wasmtime", "wasmer", "wasm3", "iwasm", "node"]

def main():
    BUGS_DIR.mkdir(parents=True, exist_ok=True)

    available   = [r for r in RUNTIME_NAMES if check_runtime_available(r)]
    unavailable = [r for r in RUNTIME_NAMES if not check_runtime_available(r)]

    wasmer_mode = "python API" if _WASMER_PYTHON else "CLI"

    print("╔══════════════════════════════════════════════════════╗")
    print("║          WASM CONTINUOUS FUZZING CAMPAIGN            ║")
    print("╚══════════════════════════════════════════════════════╝")
    print(f"  Runtimes active  : {available}")
    if unavailable:
        print(f"  Runtimes missing : {unavailable}  (skipped)")
    print(f"  wasmtime         : Python API (shared Engine)")
    print(f"  wasmer           : {wasmer_mode}")
    print(f"  wasm3/iwasm/node : CLI subprocesses")
    print(f"  Batch size       : {BATCH_SIZE} programs")
    print(f"  Workers          : {NUM_WORKERS}")
    print(f"  Results DB       : {DB_PATH}")
    print(f"  Bug output       : {BUGS_DIR}")
    print()

    if not available:
        print("ERROR: No runtimes available.")
        return

    con = init_db()
    batch_num = 1
    try:
        while True:
            run_batch(batch_num, con, available)
            batch_num += 1
    except KeyboardInterrupt:
        print("\n\nStopped. All results saved to fuzz_results.db.")
    finally:
        con.close()

if __name__ == "__main__":
    main()