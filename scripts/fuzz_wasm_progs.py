#!/usr/bin/env python3
"""
Continuous WASM fuzzing campaign.
Runs forever (or until killed), generating 2500 programs per batch and testing
them across multiple runtimes against expected outputs from .jsonl mappings.
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
from pathlib import Path
from datetime import datetime

# ─── Paths (all relative to repo root, which is one level above this script) ───
SCRIPT_DIR   = Path(__file__).resolve().parent
BASE_DIR     = SCRIPT_DIR.parent
PROGRAMS_DIR = BASE_DIR / "generated" / "programs"
MAPPINGS_DIR = BASE_DIR / "generated" / "mappings"
GENERATED_DIR= BASE_DIR / "generated"
BUGS_DIR     = BASE_DIR / "wasm_bugs"
EXEC_PATHS   = BASE_DIR / "exec_paths.json"
DB_PATH      = BASE_DIR / "fuzz_results.db"

BATCH_SIZE   = 2500
UUID_PREFIX  = "sriya"
VERBOSE      = True   # set False for long unattended runs

# ─── Runtime definitions ────────────────────────────────────────────────────────
# Each runtime is a dict with:
#   name      : display name
#   kind      : "wasmtime_api" | "cli_wat" | "cli_wasm" | "node"
#   bin       : executable name (for which/shutil.which check)
RUNTIMES = [
    {"name": "wasmtime", "kind": "wasmtime_api",  "bin": "wasmtime"},
    {"name": "wasmer",   "kind": "cli_wasm",       "bin": "wasmer"},
    {"name": "wasm3",    "kind": "cli_wasm",       "bin": "wasm3"},
    {"name": "iwasm",    "kind": "cli_wasm",       "bin": "iwasm"},
    {"name": "node",     "kind": "node",            "bin": "node"},
]

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
            batch_id    INTEGER PRIMARY KEY AUTOINCREMENT,
            uuid        TEXT,
            started_at  TEXT,
            finished_at TEXT,
            programs_generated INTEGER,
            fgen_avg_time_s    REAL
        );
        CREATE TABLE IF NOT EXISTS programs (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            batch_id    INTEGER,
            filename    TEXT,
            num_funcs   INTEGER
        );
        CREATE TABLE IF NOT EXISTS func_results (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            batch_id    INTEGER,
            program     TEXT,
            func_name   TEXT,
            runtime     TEXT,
            status      TEXT,   -- pass | fail | trap | error
            detail      TEXT
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

def run_fgen(batch_id: int) -> float:
    """Run the function generation step. Returns avg time of successful gens."""
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
        cwd=BASE_DIR,
        env=env,
        capture_output=True,
        text=True,
    )
    elapsed = time.time() - t0

    # Parse SUCC lines for timing
    times = []
    for line in (proc.stdout + proc.stderr).splitlines():
        m = re.search(r"SUCC \(time=([\d.]+)s\)", line)
        if m:
            times.append(float(m.group(1)))

    avg = sum(times) / len(times) if times else 0.0
    print(f"  [gen] fgen done in {elapsed:.1f}s — {len(times)} funcs OK, avg {avg:.2f}s each")
    return avg

def run_pgen(batch_id: int, uuid: str):
    """Run program generation (pgen). Blocks until done."""
    print(f"  [gen] Running pgen (limit={BATCH_SIZE}, uuid={uuid}) ...")
    t0 = time.time()
    proc = subprocess.run(
        [
            str(BASE_DIR / "build" / "bin" / "pgen"),
            "--wasm",
            "--input", str(GENERATED_DIR),
            "--limit", str(BATCH_SIZE),
            "--uuid",  uuid,
        ],
        cwd=BASE_DIR,
        capture_output=True,
        text=True,
    )
    elapsed = time.time() - t0
    count = len(list(PROGRAMS_DIR.glob("*.wat")))
    print(f"  [gen] pgen done in {elapsed:.1f}s — {count} .wat files produced")
    return count

def refresh_exec_paths():
    """Wipe and recreate exec_paths.json (empty, generator may populate it)."""
    EXEC_PATHS.write_text("{}\n")

# ═══════════════════════════════════════════════════════════════════════════════
# WAT → WASM CONVERSION
# ═══════════════════════════════════════════════════════════════════════════════

_wat2wasm_available = None

def wat2wasm_available():
    global _wat2wasm_available
    if _wat2wasm_available is None:
        _wat2wasm_available = shutil.which("wat2wasm") is not None
    return _wat2wasm_available

def convert_to_wasm(wat_path: Path) -> Path | None:
    """Convert .wat to .wasm in a temp dir. Returns .wasm path or None on error."""
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

def run_wasmtime_api(wat_path: Path, func_name: str, inputs: list[int]) -> list[int] | str:
    """Use wasmtime Python API directly."""
    try:
        from wasmtime import Engine, Store, Module, Instance
        engine = Engine()
        store  = Store(engine)
        module = Module.from_file(engine, str(wat_path))
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

# Node.js harness template
_NODE_HARNESS = textwrap.dedent("""\
    const fs = require('fs');
    const buf = fs.readFileSync('{wasm_path}');
    const mod = new WebAssembly.Module(buf);
    const inst = new WebAssembly.Instance(mod);
    const fn = inst.exports['{func_name}'];
    const result = fn({args});
    if (Array.isArray(result)) {{
        console.log(JSON.stringify(result.map(Number)));
    }} else {{
        console.log(JSON.stringify([Number(result)]));
    }}
""")

def run_node(wasm_path: Path, func_name: str, inputs: list[int]) -> list[int] | str:
    args_str = ", ".join(str(x) for x in inputs)
    script   = _NODE_HARNESS.format(
        wasm_path=str(wasm_path),
        func_name=func_name,
        args=args_str,
    )
    with tempfile.NamedTemporaryFile(suffix=".mjs", mode="w", delete=False) as f:
        f.write(script)
        tmp = f.name
    try:
        r = subprocess.run(["node", tmp], capture_output=True, text=True, timeout=10)
        if r.returncode != 0:
            return "TRAP:" + r.stderr.strip()[:120]
        return [to_i32(x) for x in json.loads(r.stdout.strip())]
    except Exception as e:
        return "ERROR:" + str(e)[:120]
    finally:
        os.unlink(tmp)

def run_cli_wasm(binary: str, wasm_path: Path, func_name: str, inputs: list[int]) -> list[int] | str:
    """
    Generic CLI runner for wasmer / wasm3 / iwasm.
    Invokes:  <binary> <wasm_path> --invoke <func_name> -- <args...>
    Parses numeric output from stdout.
    """
    args = [binary, str(wasm_path), "--invoke", func_name, "--"] + [str(x) for x in inputs]
    try:
        r = subprocess.run(args, capture_output=True, text=True, timeout=10)
        if r.returncode != 0:
            out = (r.stdout + r.stderr).strip()
            return "TRAP:" + out[:120]
        # Parse: expect one or more integers, one per line or space-separated
        nums = re.findall(r"-?\d+", r.stdout)
        if not nums:
            return "ERROR:no_output"
        return [to_i32(int(n)) for n in nums]
    except subprocess.TimeoutExpired:
        return "TRAP:timeout"
    except Exception as e:
        return "ERROR:" + str(e)[:120]

def run_runtime(rt: dict, wat_path: Path, wasm_path: Path | None,
                func_name: str, inputs: list[int]) -> list[int] | str:
    kind = rt["kind"]
    if kind == "wasmtime_api":
        return run_wasmtime_api(wat_path, func_name, inputs)
    if kind == "node":
        if wasm_path is None:
            return "ERROR:no_wasm_for_node"
        return run_node(wasm_path, func_name, inputs)
    if kind == "cli_wasm":
        if wasm_path is None:
            return "ERROR:no_wasm_binary"
        return run_cli_wasm(rt["bin"], wasm_path, func_name, inputs)
    return "ERROR:unknown_kind"

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

def check_runtime_available(rt: dict) -> bool:
    if rt["kind"] == "wasmtime_api":
        try:
            import wasmtime  # noqa
            return True
        except ImportError:
            return False
    return shutil.which(rt["bin"]) is not None

# ═══════════════════════════════════════════════════════════════════════════════
# BATCH EXECUTION
# ═══════════════════════════════════════════════════════════════════════════════

def run_batch(batch_num: int, con: sqlite3.Connection, available_runtimes: list[dict]):
    uuid = f"{UUID_PREFIX}_{batch_num}"
    started = datetime.utcnow().isoformat()
    print(f"\n{'═'*60}")
    print(f"  BATCH {batch_num}  |  UUID: {uuid}  |  {started}")
    print(f"{'═'*60}")

    # ── Generation ──────────────────────────────────────────────────────────
    clear_generated()
    refresh_exec_paths()

    fgen_avg = run_fgen(batch_num)
    prog_count = run_pgen(batch_num, uuid)

    cur = con.cursor()
    cur.execute(
        "INSERT INTO batches (uuid, started_at, programs_generated, fgen_avg_time_s) "
        "VALUES (?,?,?,?)",
        (uuid, started, prog_count, fgen_avg)
    )
    batch_id = cur.lastrowid
    con.commit()

    # ── Fuzzing ─────────────────────────────────────────────────────────────
    wat_files = sorted(PROGRAMS_DIR.glob("*.wat"))
    print(f"\n  [fuzz] {len(wat_files)} programs × {len(available_runtimes)} runtimes\n")

    batch_stats = {rt["name"]: {"pass": 0, "fail": 0, "trap": 0, "error": 0}
                   for rt in available_runtimes}
    failed_programs = set()

    for prog_idx, wat_path in enumerate(wat_files, 1):
        func_names = extract_func_names(wat_path)
        if not func_names:
            continue

        # Convert once per program
        wasm_path = convert_to_wasm(wat_path) if wat2wasm_available() else None

        cur.execute(
            "INSERT INTO programs (batch_id, filename, num_funcs) VALUES (?,?,?)",
            (batch_id, wat_path.name, len(func_names))
        )
        con.commit()

        prog_failed = False

        if VERBOSE:
            print(f"\n  [{prog_idx}/{len(wat_files)}] {wat_path.name}  ({len(func_names)} funcs)")

        for func_name in func_names:
            jsonl_path = MAPPINGS_DIR / f"{func_name}.jsonl"
            if not jsonl_path.exists():
                continue

            # Load test cases
            test_cases = []
            with open(jsonl_path) as f:
                for line in f:
                    if line.strip():
                        d = json.loads(line)
                        inp = [item["elems"][0] for item in d["ini"]]
                        exp = [to_i32(item["elems"][0]) for item in d["fin"]]
                        test_cases.append((inp, exp))

            if not test_cases:
                continue

            # Collect results across all runtimes for this func (for verbose display)
            func_rt_results = {}

            for rt in available_runtimes:
                rt_name = rt["name"]
                func_passed = True
                status = "pass"
                detail = ""
                first_bad_inp = None
                first_bad_exp = None
                first_bad_got = None

                for inp, exp in test_cases:
                    result = run_runtime(rt, wat_path, wasm_path, func_name, inp)

                    if isinstance(result, str):
                        func_passed = False
                        status = "trap" if result.startswith("TRAP") else "error"
                        detail = result
                        first_bad_inp = inp
                        first_bad_exp = exp
                        first_bad_got = result
                        break
                    elif result != exp:
                        func_passed = False
                        status = "fail"
                        detail = f"expected={exp} got={result}"
                        first_bad_inp = inp
                        first_bad_exp = exp
                        first_bad_got = result
                        break

                batch_stats[rt_name][status] += 1
                func_rt_results[rt_name] = (status, first_bad_inp, first_bad_exp, first_bad_got)

                if not func_passed:
                    prog_failed = True
                    cur.execute(
                        "INSERT INTO func_results "
                        "(batch_id, program, func_name, runtime, status, detail) "
                        "VALUES (?,?,?,?,?,?)",
                        (batch_id, wat_path.name, func_name, rt_name, status, detail)
                    )

            if VERBOSE:
                # Summarise this function: one line per runtime
                all_pass = all(v[0] == "pass" for v in func_rt_results.values())
                status_str = "  ".join(
                    f"{rn}={'OK' if v[0]=='pass' else v[0].upper()}"
                    for rn, v in func_rt_results.items()
                )
                print(f"    {func_name[:40]:40s}  {status_str}")
                # For any failure, print expected vs got on the next line
                for rn, (st, bad_inp, bad_exp, bad_got) in func_rt_results.items():
                    if st != "pass":
                        print(f"      └─ {rn}: inp={bad_inp}")
                        print(f"         exp={bad_exp}")
                        print(f"         got={bad_got}")

        if prog_failed:
            failed_programs.add(wat_path)

        # Progress every 100 programs (non-verbose) or always in verbose
        if not VERBOSE and prog_idx % 100 == 0:
            print(f"  [fuzz] {prog_idx}/{len(wat_files)} programs checked ...")

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

    # ── Batch summary ────────────────────────────────────────────────────────
    finished = datetime.utcnow().isoformat()
    cur.execute("UPDATE batches SET finished_at=? WHERE batch_id=?", (finished, batch_id))
    con.commit()

    print(f"\n  {'─'*56}")
    print(f"  BATCH {batch_num} SUMMARY")
    print(f"  {'─'*56}")
    print(f"  Programs generated : {prog_count}")
    print(f"  fgen avg time      : {fgen_avg:.2f}s")
    print(f"  Programs with bugs : {len(failed_programs)}")
    for rt in available_runtimes:
        s = batch_stats[rt["name"]]
        total = sum(s.values())
        print(f"  {rt['name']:10s}  pass={s['pass']}  fail={s['fail']}  "
              f"trap={s['trap']}  err={s['error']}  (/{total})")
    print(f"  {'─'*56}\n")

# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════════

def main():
    BUGS_DIR.mkdir(parents=True, exist_ok=True)

    # Check which runtimes are available
    available = [rt for rt in RUNTIMES if check_runtime_available(rt)]
    unavailable = [rt["name"] for rt in RUNTIMES if not check_runtime_available(rt)]

    print("╔══════════════════════════════════════════════════╗")
    print("║         WASM CONTINUOUS FUZZING CAMPAIGN         ║")
    print("╚══════════════════════════════════════════════════╝")
    print(f"  Runtimes active  : {[r['name'] for r in available]}")
    if unavailable:
        print(f"  Runtimes missing : {unavailable}  (skipped)")
    print(f"  Batch size       : {BATCH_SIZE} programs")
    print(f"  Results DB       : {DB_PATH}")
    print(f"  Bug output       : {BUGS_DIR}")
    print()

    if not available:
        print("ERROR: No runtimes available. Install at least wasmtime.")
        return

    con = init_db()

    batch_num = 1
    try:
        while True:
            run_batch(batch_num, con, available)
            batch_num += 1
    except KeyboardInterrupt:
        print("\n\nStopped by user. Results saved to fuzz_results.db.")
    finally:
        con.close()

if __name__ == "__main__":
    main()