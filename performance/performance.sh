#!/bin/bash
# =============================================================================
# generate_programs.sh
# Generates N programs each from csmith, yarpgen, rysmith, rylink.
# Measures SUCC, TMO, Throughput, Overhead, AvgSuccTime.
# =============================================================================
#set -euo pipefail

# =============================================================================
# CONFIG
# =============================================================================
N=20
TIMEOUT=10   # seconds per program

CSMITH_BIN="/reify/previous_work/install/bin/csmith"
YARPGEN_BIN="/reify/previous_work/src/yarpgen/build/yarpgen"
RYSMITH_BIN="/reify/build/bin/rysmith"
RYLINK_BIN="/reify/build/bin/rylink"

DATA_DIR="/reify/data/performance"
CSMITH_OUT="$DATA_DIR/csmith"
YARPGEN_OUT="$DATA_DIR/yarpgen"
RYSMITH_OUT="$DATA_DIR/rysmith"
RYLINK_OUT="$DATA_DIR/rylink"

VENV_DIR="/reify/performance/.venv"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; NC='\033[0m'
info()   { echo -e "${GREEN}[INFO]${NC} $*"; }
die()    { echo -e "${RED}[ERROR]${NC} $*" >&2; exit 1; }
header() { echo -e "\n${BLUE}======================================${NC}"; echo -e "${BLUE}  $*${NC}"; echo -e "${BLUE}======================================${NC}"; }

# =============================================================================
# Sanity checks
# =============================================================================
[ -f "$CSMITH_BIN" ]  || die "csmith not found at $CSMITH_BIN"
[ -f "$YARPGEN_BIN" ] || die "yarpgen not found at $YARPGEN_BIN"
[ -f "$RYSMITH_BIN" ] || die "rysmith not found at $RYSMITH_BIN"
[ -f "$RYLINK_BIN" ]  || die "rylink not found at $RYLINK_BIN"

# =============================================================================
# Virtual environment setup
# =============================================================================
info "Setting up Python virtual environment at $VENV_DIR..."
python3 -m venv "$VENV_DIR"
source "$VENV_DIR/bin/activate"
pip install --quiet pydot 2>/dev/null
info "Virtual environment ready."

# =============================================================================
# Clean data dirs
# =============================================================================
info "Cleaning data directories..."
rm -rf "$CSMITH_OUT" "$YARPGEN_OUT" "$RYSMITH_OUT" "$RYLINK_OUT"
mkdir -p "$CSMITH_OUT" "$YARPGEN_OUT" "$RYSMITH_OUT" "$RYLINK_OUT"
info "Clean done."

# =============================================================================
# Stats helpers
# =============================================================================
declare -A G_SUCC G_TMO G_THROUGHPUT G_OVERHEAD G_AVG_SUCC_TIME

print_stats() {
    local TOOL="$1" SUCC="$2" FAIL="$3" TMO="$4" TOTAL_TIME="$5" AVG_SUCC_TIME="${6:-0}"
    local TOTAL=$(( SUCC + FAIL + TMO ))
    local SUCC_PCT=0 TMO_PCT=0 THROUGHPUT=0 OVERHEAD=0

    if [ "$TOTAL" -gt 0 ]; then
        SUCC_PCT=$(awk "BEGIN {printf \"%.1f\", $SUCC * 100 / $TOTAL}")
        TMO_PCT=$(awk "BEGIN {printf \"%.1f\", $TMO * 100 / $TOTAL}")
    fi
    if (( SUCC > 0 )); then
        THROUGHPUT=$(awk "BEGIN {printf \"%.2f\", $SUCC / $TOTAL_TIME}")
        OVERHEAD=$(awk "BEGIN {printf \"%.3f\", $TOTAL_TIME / $SUCC}")
    fi

    G_SUCC[$TOOL]="$SUCC_PCT"
    G_TMO[$TOOL]="$TMO_PCT"
    G_THROUGHPUT[$TOOL]="$THROUGHPUT"
    G_OVERHEAD[$TOOL]="$OVERHEAD"
    G_AVG_SUCC_TIME[$TOOL]="$AVG_SUCC_TIME"

    printf "  %-10s SUCC=%s%% TMO=%s%% Throughput=%s/s Overhead=%ss AvgSuccTime=%ss\n" \
        "$TOOL" "$SUCC_PCT" "$TMO_PCT" "$THROUGHPUT" "$OVERHEAD" "$AVG_SUCC_TIME"
}

# =============================================================================
# CSMITH
# =============================================================================
header "Generating $N Csmith programs"
SUCC=0; FAIL=0; TMO=0; SUCC_TIME_NS=0
T_START=$(date +%s%N)

for (( i=0; i<N; i++ )); do
    OUT="$CSMITH_OUT/csmith_file_${i}.c"
    echo -ne "\r  [$((i+1))/$N] OK:$SUCC TMO:$TMO FAIL:$FAIL  "

    EXIT_CODE=0
    P_START=$(date +%s%N)
    timeout "$TIMEOUT" "$CSMITH_BIN" \
        --seed "$RANDOM" \
        --output "$OUT" \
        2>/dev/null || EXIT_CODE=$?
    P_END=$(date +%s%N)

    if   [ "$EXIT_CODE" = "124" ];                then (( TMO++  ))
    elif [ "$EXIT_CODE" = "0" ] && [ -f "$OUT" ]; then (( SUCC++ )); SUCC_TIME_NS=$(( SUCC_TIME_NS + P_END - P_START ))
    else                                               (( FAIL++ ))
    fi
done
echo ""

T_END=$(date +%s%N)
TOTAL_TIME=$(awk "BEGIN {printf \"%.6f\", ($T_END - $T_START) / 1000000000}")
if (( SUCC > 0 )); then
    AVG_SUCC_TIME=$(awk "BEGIN {printf \"%.6f\", $SUCC_TIME_NS / 1000000000 / $SUCC}")
else
    AVG_SUCC_TIME="0"
fi
print_stats "Csmith" "$SUCC" "$FAIL" "$TMO" "$TOTAL_TIME" "$AVG_SUCC_TIME"

# =============================================================================
# YARPGEN
# =============================================================================
header "Generating $N YARPGen programs"
SUCC=0; FAIL=0; TMO=0; SUCC_TIME_NS=0
T_START=$(date +%s%N)

for (( i=0; i<N; i++ )); do
    SAMPLE_DIR="$YARPGEN_OUT/yarpgen_sample_${i}"
    mkdir -p "$SAMPLE_DIR"
    echo -ne "\r  [$((i+1))/$N] OK:$SUCC TMO:$TMO FAIL:$FAIL  "

    EXIT_CODE=0
    P_START=$(date +%s%N)
    timeout "$TIMEOUT" "$YARPGEN_BIN" \
        --seed="$RANDOM" \
        --std=c \
        -o "$SAMPLE_DIR" \
        2>/dev/null || EXIT_CODE=$?
    P_END=$(date +%s%N)

    if [ "$EXIT_CODE" = "124" ]; then
        (( TMO++ ))
    elif [ -f "$SAMPLE_DIR/driver.c" ] && [ -f "$SAMPLE_DIR/func.c" ]; then
        (( SUCC++ ))
        SUCC_TIME_NS=$(( SUCC_TIME_NS + P_END - P_START ))
    else
        (( FAIL++ ))
    fi
done
echo ""

T_END=$(date +%s%N)
TOTAL_TIME=$(awk "BEGIN {printf \"%.6f\", ($T_END - $T_START) / 1000000000}")
if (( SUCC > 0 )); then
    AVG_SUCC_TIME=$(awk "BEGIN {printf \"%.6f\", $SUCC_TIME_NS / 1000000000 / $SUCC}")
else
    AVG_SUCC_TIME="0"
fi
print_stats "YARPGen" "$SUCC" "$FAIL" "$TMO" "$TOTAL_TIME" "$AVG_SUCC_TIME"

# =============================================================================
# RYSMITH
# =============================================================================
header "Generating $N Rysmith programs"
SUCC=0; FAIL=0; TMO=0; SUCC_TIME_NS=0
T_START=$(date +%s%N)

for (( i=0; i<N; i++ )); do
    NAME="rysmith_perf"
    echo -ne "\r  [$((i+1))/$N] OK:$SUCC TMO:$TMO FAIL:$FAIL  "

    EXIT_CODE=0
    P_START=$(date +%s%N)
    timeout "$TIMEOUT" "$RYSMITH_BIN" \
        --Xnum-bbls-per-fun 15 \
        --Xnum-vars-per-fun 8 \
        --Xnum-assigns-per-bbl 2 \
        --Xnum-vars-per-assign 2 \
        --Xnum-vars-in-cond 3 \
        --Xbitwuzla-threads 8 -S\
        -o "$RYSMITH_OUT" \
        -n $i \
        "$NAME" \
	--Xdisable-struct-vars \
	-s 963929304\
        2>/dev/null || EXIT_CODE=$?
    P_END=$(date +%s%N)

    if   [ "$EXIT_CODE" = "124" ]; then (( TMO++  ))
    elif [ "$EXIT_CODE" = "0" ];  then (( SUCC++ )); SUCC_TIME_NS=$(( SUCC_TIME_NS + P_END - P_START ))
    else                                (( FAIL++ ))
    fi
done
echo ""

T_END=$(date +%s%N)
TOTAL_TIME=$(awk "BEGIN {printf \"%.6f\", ($T_END - $T_START) / 1000000000}")
if (( SUCC > 0 )); then
    AVG_SUCC_TIME=$(awk "BEGIN {printf \"%.6f\", $SUCC_TIME_NS / 1000000000 / $SUCC}")
else
    AVG_SUCC_TIME="0"
fi
print_stats "Rysmith" "$SUCC" "$FAIL" "$TMO" "$TOTAL_TIME" "$AVG_SUCC_TIME"

# =============================================================================
# RYLINK — links rysmith leaf functions into whole programs
# outputs into the same dir as input (-i), programs are prog_<id>_<n>/
# =============================================================================
header "Generating $N Rylink programs from rysmith output"
SUCC=0; FAIL=0; TMO=0
 
# rylink outputs into the -i directory, so copy rysmith output to rylink dir first
cp -r "$RYSMITH_OUT/." "$RYLINK_OUT/"
 
P_START=$(date +%s%N)
EXIT_CODE=0
echo "  CMD: $RYLINK_BIN -i $RYLINK_OUT -l $N -s $RANDOM rylink_perf"
timeout $(( TIMEOUT * N )) "$RYLINK_BIN" \
    -i "$RYLINK_OUT" \
    -l "$N" \
    -s "$RANDOM" \
    rylink_perf \
    --Xfunction-depth 10 \
    2>/dev/null || EXIT_CODE=$?
P_END=$(date +%s%N)
 
TOTAL_TIME=$(awk "BEGIN {printf \"%.6f\", ($P_END - $P_START) / 1000000000}")
 
if [ "$EXIT_CODE" = "124" ]; then
    TMO=$N; SUCC=0; FAIL=0
    AVG_SUCC_TIME="0"
else
    SUCC=$(find "$RYLINK_OUT" -maxdepth 1 -name "prog_*" -type d 2>/dev/null | wc -l)
    FAIL=$(( N - SUCC < 0 ? 0 : N - SUCC ))
    if (( SUCC > 0 )); then
        AVG_SUCC_TIME=$(awk "BEGIN {printf \"%.6f\", ($P_END - $P_START) / 1000000000 / $SUCC}")
    else
        AVG_SUCC_TIME="0"
    fi
fi
print_stats "Rylink" "$SUCC" "$FAIL" "$TMO" "$TOTAL_TIME" "$AVG_SUCC_TIME"

# =============================================================================
# FINAL SUMMARY TABLE
# =============================================================================
echo ""
echo "=============================================================================="
printf "  %-10s %8s %8s %14s %12s %16s\n" \
    "Tool" "SUCC%" "TMO%" "Throughput/s" "Overhead(s)" "AvgSuccTime(s)"
echo "  ------------------------------------------------------------------------------"
for TOOL in Csmith YARPGen Rysmith Rylink; do
    printf "  %-10s %8s %8s %14s %12s %16s\n" \
        "$TOOL" \
        "${G_SUCC[$TOOL]:-0}%" \
        "${G_TMO[$TOOL]:-0}%" \
        "${G_THROUGHPUT[$TOOL]:-0}" \
        "${G_OVERHEAD[$TOOL]:-0}" \
        "${G_AVG_SUCC_TIME[$TOOL]:-0}"
done
echo "=============================================================================="

# =============================================================================
# TOKEN + CFG STATS
# Uses pydot (installed in venv) to match ggen.py's exact counting method:
#   - Blocks = dot.get_nodes() filtered to Node0x* (same as CfgSkel.parse)
#   - Jumps  = dot.get_edges() count (same as num_jmps)
# Token counting matches count_tokens.py exactly, including boilerplate subtraction.
# =============================================================================
echo ""
echo "Computing token and CFG stats..."

"$VENV_DIR/bin/python3" << 'PYEOF'
import re
import os
import subprocess
import tempfile
import shutil
from pathlib import Path

# ── tokenizer — exact copy of count_tokens.py ────────────────────────────────
def remove_c_comments(code):
    pattern = r'("(?:[^"\\]|\\.)*")|(/\*.*?\*/|//[^\n]*)'
    def replacer(m):
        return m.group(1) if m.group(1) else ''
    return re.sub(pattern, replacer, code, flags=re.DOTALL)

def count_tokens(path=None, code=None):
    if code is None:
        code = Path(path).read_text(encoding='utf-8', errors='replace')
    code = remove_c_comments(code)
    patterns = [
        r'\b[a-zA-Z_][a-zA-Z0-9_]*\b',
        r'\b\d+\.?\d*([eE][+-]?\d+)?[fFlLuU]*\b',
        r'"(?:[^"\\]|\\.)*"',
        r"'(?:[^'\\]|\\.)*'",
        r'==|!=|<=|>=|&&|\|\||<<|>>|\+\+|--|->|\+=|-=|\*=|/=|%=|&=|\|=|\^=|<<=|>>=',
        r'[+\-*/%=<>&|^!~?:]',
        r'[(){}\[\];,.]',
        r'#[^\n]*',
    ]
    combined = '|'.join(f'({p})' for p in patterns)
    tokens = re.findall(combined, code)
    return len([t for group in tokens for t in group if t])

# ── CFG stats — matches ggen.py CfgSkel.parse exactly ────────────────────────
CLANG = shutil.which("clang") or "/usr/bin/clang"
OPT   = shutil.which("opt")   or "/usr/bin/opt"
print(f"  Using clang: {CLANG}")
print(f"  Using opt:   {OPT}")

import pydot

def cfg_stats(c_file, extra_includes=None, fn_pat=None):
    """
    Returns (num_blocks, num_jumps) replicating ggen.py CfgSkel.parse exactly:
      bbls   = nodes starting with Node0x
      succs  = [-1, -1] per bbl, filled by walking edges
      blocks = len(bbls)
      jumps  = sum(1 for s in succs for slot in s if slot != -1)
    """
    try:
        with tempfile.TemporaryDirectory() as tmp:
            ll = Path(tmp) / "prog.ll"
            cmd = [CLANG, "-Xclang", "-disable-llvm-passes", "-emit-llvm", "-S",
                   "-o", str(ll)]
            if extra_includes:
                for inc in extra_includes:
                    cmd += ["-I", inc]
            cmd.append(str(c_file))
            subprocess.run(cmd, capture_output=True, timeout=10)
            if not ll.exists():
                return 0, 0
            opt_cmd = [OPT, "-disable-output", "-passes=dot-cfg",
                       "-cfg-dot-filename-prefix=cfg",
                       str(ll)]
            subprocess.run(opt_cmd, capture_output=True, timeout=10, cwd=tmp)

            total_blocks = 0
            total_jumps  = 0
            _debug_done = False

            # filter dot files by fn_pat if provided
            # dot files are named cfg.<funcname>.dot
            # we match by checking if the function name starts with fn_pat
            all_dots = list(Path(tmp).glob("*.dot"))
            if fn_pat:
                matched_dots = [d for d in all_dots
                                if re.match(rf"cfg\.{re.escape(fn_pat)}.*\.dot", d.name)
                                or d.stem.split(".", 1)[-1].startswith(fn_pat)]
            else:
                matched_dots = all_dots

            for dot_file in matched_dots:
                try:
                    dot_list = pydot.graph_from_dot_file(str(dot_file))
                    if not dot_list:
                        continue
                    for dot in dot_list:
                        # build bbl list — same as CfgSkel.add_bbl
                        bbls = [n.get_name().strip('"') for n in dot.get_nodes()
                                if n.get_name().strip('"').startswith("Node0x")]
                        # succs table: 2 slots per bbl, -1 = unfilled
                        succs = [[-1, -1] for _ in bbls]

                        for edge in dot.get_edges():
                            src = edge.get_source()
                            dst = edge.get_destination()
                            # pydot may return "Node0x123:s0" or Node0x123:s0
                            # strip outer quotes first, then port suffix
                            src = src.strip('"').strip("'")
                            dst = dst.strip('"').strip("'")
                            # strip port suffix :s0 / :s1
                            if ":s0" in src:
                                src = src[:src.index(":s0")]; which = 0
                            elif ":s1" in src:
                                src = src[:src.index(":s1")]; which = 1
                            else:
                                which = -1
                            src = src.strip('"').strip("'")
                            dst = dst.strip('"').strip("'")
                            if src not in bbls or dst not in bbls:
                                continue
                            ui = bbls.index(src)
                            vi = bbls.index(dst)
                            if which == -1:
                                # fill first available slot
                                if succs[ui][0] == -1:
                                    succs[ui][0] = vi
                                elif succs[ui][1] == -1:
                                    succs[ui][1] = vi
                            else:
                                succs[ui][which] = vi

                        jmps = sum(1 for s in succs for slot in s if slot != -1)
                        total_blocks += len(bbls)
                        total_jumps  += jmps
                        if not _debug_done:
                            _debug_done = True
                            import os
                            if os.environ.get("CFG_DEBUG"):
                                print(f"\n  [debug] {dot_file.name}: bbls={len(bbls)} jmps={jmps}")
                                for edge in dot.get_edges():
                                    print(f"    edge src={repr(edge.get_source())} dst={repr(edge.get_destination())}")
                except Exception:
                    continue
            return total_blocks, total_jumps
    except Exception:
        return 0, 0

# ── boilerplate token counts to subtract ─────────────────────────────────────
YARPGEN_BOILERPLATE = count_tokens(code="""
unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}
""")

CRC32_BOILERPLATE = count_tokens(code="""
static uint32_t crc32_tab[256] = {0};
static int32_t uint32_to_int32(uint32_t u) { return u % 2147483647; }
static uint32_t context_free_crc32_byte(uint32_t context, uint8_t b) {
  return ((context >> 8) & 0x00FFFFFF) ^ crc32_tab[(context ^ b) & 0xFF];
}
static uint32_t context_free_crc32_4bytes(uint32_t context, uint32_t val) {
  context = context_free_crc32_byte(context, (val >> 0) & 0xFF);
  context = context_free_crc32_byte(context, (val >> 8) & 0xFF);
  context = context_free_crc32_byte(context, (val >> 16) & 0xFF);
  context = context_free_crc32_byte(context, (val >> 24) & 0xFF);
  return context;
}
int computeStatelessChecksum(int num_args, int args[]) {
  uint32_t checksum = 0xFFFFFFFFUL;
  int i = 0;
  for (; i < num_args; ++i) { checksum = context_free_crc32_4bytes(checksum, args[i]); }
  checksum = uint32_to_int32(checksum ^ 0xFFFFFFFFUL);
  return checksum;
}
""")

# ── measurement helpers ───────────────────────────────────────────────────────
def measure_files(label, files, extra_includes=None, subtract_tokens=0, fn_pat=None):
    tokens_list, blocks_list, jumps_list = [], [], []
    total = len(files)
    if total == 0:
        print(f"  {label}: no files found")
        return
    for i, f in enumerate(files):
        print(f"\r  {label} [{i+1}/{total}]", end="", flush=True)
        try:
            tok = max(0, count_tokens(f) - subtract_tokens)
            b, j = cfg_stats(f, extra_includes=extra_includes, fn_pat=fn_pat)
            tokens_list.append(tok)
            blocks_list.append(b)
            jumps_list.append(j)
        except Exception:
            pass
    print()
    avg_tok = sum(tokens_list) / len(tokens_list) if tokens_list else 0
    avg_blk = sum(blocks_list) / len(blocks_list) if blocks_list else 0
    avg_jmp = sum(jumps_list)  / len(jumps_list)  if jumps_list  else 0
    print(f"  {label:10s}  #Tokens={avg_tok:8.0f}  #Blocks={avg_blk:6.1f}  #Jumps={avg_jmp:6.1f}")

# ── main ──────────────────────────────────────────────────────────────────────
base = Path(os.environ.get("DATA_DIR", "/reify/data/performance"))
print(f"  Stats base dir: {base}")

# csmith include
csmith_include = None
for candidate in ["/reify/previous_work/install/include"]:
    if Path(candidate).exists():
        csmith_include = candidate
        break
if csmith_include:
    print(f"  Using csmith include: {csmith_include}")

# Csmith — test functions are func_* (matches Csmith.fn_pat() = 'func_')
measure_files("Csmith",
    sorted((base / "csmith").glob("*.c")),
    extra_includes=[csmith_include] if csmith_include else None,
    fn_pat="func_")

# YARPGen — measure driver.c + func.c + init.h per sample dir, subtract hash boilerplate
# fn_pat="test" matches YARPGen.fn_pat() in ggen.py
yarpgen_dirs = sorted((base / "yarpgen").glob("yarpgen_sample_*/"))
if not yarpgen_dirs:
    print("  YARPGen: no sample dirs found")
else:
    tokens_list, blocks_list, jumps_list = [], [], []
    total = len(yarpgen_dirs)
    for i, d in enumerate(yarpgen_dirs):
        print(f"\r  YARPGen [{i+1}/{total}]", end="", flush=True)
        try:
            # tokens: sum all 3 files, subtract boilerplate once
            tok = sum(count_tokens(f) for f in [d/"driver.c", d/"func.c", d/"init.h"] if f.exists())
            tok = max(0, tok - YARPGEN_BOILERPLATE)
            # CFG: run on driver.c and func.c with fn_pat="test"
            b_tot, j_tot = 0, 0
            for f in [d/"driver.c", d/"func.c"]:
                if f.exists():
                    b, j = cfg_stats(f, fn_pat="test")
                    b_tot += b
                    j_tot += j
            tokens_list.append(tok)
            blocks_list.append(b_tot)
            jumps_list.append(j_tot)
        except Exception:
            pass
    print()
    avg_tok = sum(tokens_list) / len(tokens_list) if tokens_list else 0
    avg_blk = sum(blocks_list) / len(blocks_list) if blocks_list else 0
    avg_jmp = sum(jumps_list)  / len(jumps_list)  if jumps_list  else 0
    print(f"  {'YARPGen':10s}  #Tokens={avg_tok:8.0f}  #Blocks={avg_blk:6.1f}  #Jumps={avg_jmp:6.1f}")

# Rysmith — func.c only, function name is func_* (matches rysmith output)
measure_files("Rysmith",
    sorted((base / "rysmith").rglob("func.c")),
    fn_pat="func_")

# Rylink — sum all .c files per prog_* dir, subtract CRC32 per file
# (matches count_reify_prog())
rylink_dir = base / "rylink"
try:
    prog_dirs = sorted(d for d in rylink_dir.iterdir()
                       if d.is_dir() and d.name.startswith("prog_"))
except Exception:
    prog_dirs = []

if not prog_dirs:
    print(f"  Rylink: no prog_* dirs found in {rylink_dir}")
else:
    print(f"  Rylink: found {len(prog_dirs)} programs")
    tokens_list, blocks_list, jumps_list = [], [], []
    for i, prog_dir in enumerate(prog_dirs):
        print(f"\r  Rylink [{i+1}/{len(prog_dirs)}]", end="", flush=True)
        try:
            prog_files = sorted(prog_dir.glob("*.c"))
            tok   = sum(max(0, count_tokens(f) - CRC32_BOILERPLATE) for f in prog_files)
            b_tot = 0
            j_tot = 0
            for f in prog_files:
                b, j = cfg_stats(f, fn_pat="func_")
                b_tot += b
                j_tot += j
            tokens_list.append(tok)
            blocks_list.append(b_tot)
            jumps_list.append(j_tot)
        except Exception:
            pass
    print()
    if tokens_list:
        print(f"  {'Rylink':10s}  #Tokens={sum(tokens_list)/len(tokens_list):8.0f}"
              f"  #Blocks={sum(blocks_list)/len(blocks_list):6.1f}"
              f"  #Jumps={sum(jumps_list)/len(jumps_list):6.1f}")
PYEOF
