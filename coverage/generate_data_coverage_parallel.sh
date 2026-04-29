#!/bin/bash
# =============================================================================
# generate_coverage_data.sh
# Generates large program sets for coverage experiments.
# All generation runs in parallel:
#   - Csmith:   split into CSMITH_WORKERS chunks, each chunk runs sequentially
#   - YARPGen:  split into YARPGEN_WORKERS chunks, each chunk runs sequentially
#   - Rysmith:  10 sets run in parallel (one background job per set,
#               WORKERS_PER_SET parallel chunk workers within each set)
#   - Rylink:   10 depths run in parallel (one background job per depth)
#
# Each rysmith index gets:
#   attempt 1: set-specific params
#   attempts 2-4: up to 3 retries with default params on timeout/failure
# =============================================================================
set -euo pipefail

# =============================================================================
# CONFIG
# =============================================================================
CSMITH_BIN="/reify/previous_work/install/bin/csmith"
YARPGEN_BIN="/reify/previous_work/src/yarpgen/build/yarpgen"
RYSMITH_BIN="/reify/build/bin/rysmith"
RYLINK_BIN="/reify/build/bin/rylink"

BASE_DIR="/reify/data/coverage"
CSMITH_OUT="$BASE_DIR/csmith"
YARPGEN_OUT="$BASE_DIR/yarpgen"
RYSMITH_BASE="$BASE_DIR/rysmith"
RYLINK_BASE="$BASE_DIR/rylink"
RYSMITH_ALL="$BASE_DIR/rysmith_all"
RYLINK_ALL="$BASE_DIR/rylink_all"

TIMEOUT=10
BITWUZLA_THREADS=8

RYSMITH_COVERAGE_FLAGS="-A -U --Xenable-lvn-gvn --Xenable-all-ops --Xenable-volatile-vars"

# Default fallback config: [bbls, vars, assigns, vpa, vic]
DEFAULT_BBLS=15; DEFAULT_VARS=8; DEFAULT_ASSIGNS=2; DEFAULT_VPA=2; DEFAULT_VIC=3

# 10 rysmith param configs: [bbls, vars, assigns, vpa, vic]
RYSMITH_CONFIGS=(
    "5  5  1 1 1"
    "8  6  1 1 2"
    "10 8  1 1 2"
    "10 8  2 2 3"
    "15 8  2 2 3"
    "15 10 2 2 3"
    "20 10 2 2 3"
    "20 10 2 2 4"
    "25 10 2 2 4"
    "30 10 1 1 2"
)

RYSMITH_SETS=10
RYLINK_DEPTHS="$(seq 2 1 11)"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; NC='\033[0m'
info()    { echo -e "${GREEN}[INFO]${NC} $*"; }
warning() { echo -e "${YELLOW}[WARN]${NC} $*"; }
die()     { echo -e "${RED}[ERROR]${NC} $*" >&2; exit 1; }
header()  { echo -e "\n${BLUE}======================================${NC}"; echo -e "${BLUE}  $*${NC}"; echo -e "${BLUE}======================================${NC}"; }

# =============================================================================
# ARGS
#   --unit N             Scale (default 1000): csmith=N, yarpgen=N,
#                          rysmith=10x(N/10), rylink=10x(N/10)
#   --csmith-workers N   Workers for csmith (default 30)
#   --yarpgen-workers N  Workers for yarpgen (default 30)
#   --rysmith-workers N  Workers per rysmith set (default 3)
# =============================================================================
UNIT=1000
CSMITH_WORKERS=30
YARPGEN_WORKERS=30
WORKERS_PER_SET=3

while [[ $# -gt 0 ]]; do
    case "$1" in
        --unit)             UNIT="$2";            shift 2 ;;
        --csmith-workers)   CSMITH_WORKERS="$2";  shift 2 ;;
        --yarpgen-workers)  YARPGEN_WORKERS="$2"; shift 2 ;;
        --rysmith-workers)  WORKERS_PER_SET="$2"; shift 2 ;;
        *) die "Unknown argument: $1. Use --help for usage." ;;
    esac
done

CSMITH_N="$UNIT"
YARPGEN_N="$UNIT"
RYSMITH_N=$(( UNIT / 10 )); [ "$RYSMITH_N" -lt 1 ] && RYSMITH_N=1
RYLINK_N=$(( UNIT / 10 ));  [ "$RYLINK_N"  -lt 1 ] && RYLINK_N=1

[ -f "$CSMITH_BIN" ]  || die "csmith not found at $CSMITH_BIN"
[ -f "$YARPGEN_BIN" ] || die "yarpgen not found at $YARPGEN_BIN"
[ -f "$RYSMITH_BIN" ] || die "rysmith not found at $RYSMITH_BIN"
[ -f "$RYLINK_BIN" ]  || die "rylink not found at $RYLINK_BIN"

rm -rf "$CSMITH_OUT" "$YARPGEN_OUT" "$RYSMITH_BASE" "$RYLINK_BASE" "$RYSMITH_ALL" "$RYLINK_ALL"
mkdir -p "$CSMITH_OUT" "$YARPGEN_OUT" "$RYSMITH_BASE" "$RYLINK_BASE" "$RYSMITH_ALL" "$RYLINK_ALL"

info "unit=$UNIT  csmith=$CSMITH_N (workers=$CSMITH_WORKERS)  yarpgen=$YARPGEN_N (workers=$YARPGEN_WORKERS)  rysmith=${RYSMITH_N}x10 (workers_per_set=$WORKERS_PER_SET)  rylink=${RYLINK_N}x10"

# =============================================================================
# PHASE 1 — Csmith, YARPGen, and Rysmith all run in parallel
# =============================================================================
header "Phase 1: Generating Csmith, YARPGen, and Rysmith in parallel"

# ── CSMITH ────────────────────────────────────────────────────────────────────
info "Launching Csmith generation ($CSMITH_N programs, $CSMITH_WORKERS workers)..."

_csmith_worker() {
    local START="$1" END="$2"
    for (( i=START; i<END; i++ )); do
        OUT="$CSMITH_OUT/csmith_${i}.c"
        [ -f "$OUT" ] && continue
        local EXIT_CODE=0
        timeout "$TIMEOUT" "$CSMITH_BIN" \
            --seed "$RANDOM" --output "$OUT" \
            > /dev/null 2>&1 || EXIT_CODE=$?
        [ "$EXIT_CODE" != "0" ] && rm -f "$OUT"
    done
}
export -f _csmith_worker
export CSMITH_OUT CSMITH_BIN TIMEOUT

CHUNK=$(( (CSMITH_N + CSMITH_WORKERS - 1) / CSMITH_WORKERS ))
CSMITH_PIDS=()
for (( p=0; p<CSMITH_WORKERS; p++ )); do
    START=$(( p * CHUNK ))
    END=$(( START + CHUNK ))
    [ "$END" -gt "$CSMITH_N" ] && END=$CSMITH_N
    [ "$START" -ge "$CSMITH_N" ] && break
    bash -c "_csmith_worker $START $END" \
        >> "$CSMITH_OUT/worker_${p}.log" 2>&1 &
    CSMITH_PIDS+=($!)
done
CSMITH_BGPIDS=("${CSMITH_PIDS[@]}")

# ── YARPGEN ───────────────────────────────────────────────────────────────────
info "Launching YARPGen generation ($YARPGEN_N programs, $YARPGEN_WORKERS workers)..."

_yarpgen_worker() {
    local START="$1" END="$2"
    for (( i=START; i<END; i++ )); do
        local SAMPLE_DIR="$YARPGEN_OUT/yarpgen_sample_${i}"
        [ -f "$SAMPLE_DIR/func.c" ] && continue
        mkdir -p "$SAMPLE_DIR"
        local EXIT_CODE=0
        timeout "$TIMEOUT" "$YARPGEN_BIN" \
            --std=c -o "$SAMPLE_DIR" \
            > /dev/null 2>&1 || EXIT_CODE=$?
        if [ "$EXIT_CODE" != "0" ] || [ ! -f "$SAMPLE_DIR/func.c" ]; then
            rm -rf "$SAMPLE_DIR"
        fi
    done
}
export -f _yarpgen_worker
export YARPGEN_OUT YARPGEN_BIN

CHUNK=$(( (YARPGEN_N + YARPGEN_WORKERS - 1) / YARPGEN_WORKERS ))
YARPGEN_PIDS=()
for (( p=0; p<YARPGEN_WORKERS; p++ )); do
    START=$(( p * CHUNK ))
    END=$(( START + CHUNK ))
    [ "$END" -gt "$YARPGEN_N" ] && END=$YARPGEN_N
    [ "$START" -ge "$YARPGEN_N" ] && break
    bash -c "_yarpgen_worker $START $END" \
        >> "$YARPGEN_OUT/worker_${p}.log" 2>&1 &
    YARPGEN_PIDS+=($!)
done
YARPGEN_BGPIDS=("${YARPGEN_PIDS[@]}")

# ── RYSMITH ───────────────────────────────────────────────────────────────────
info "Launching Rysmith generation ($RYSMITH_SETS sets x $RYSMITH_N functions)..."

# Handles one index [i] for a set.
# Attempt 1: set-specific params.
# Attempts 2-4: up to 3 retries with default params on timeout/failure.
# Output is always func_set{SET}_{i}/ — naming is consistent regardless.
_rysmith_chunk_worker() {
    local SET="$1" BBLS="$2" VARS="$3" ASSIGNS="$4" VPA="$5" VIC="$6"
    local START="$7" END="$8"
    local SET_DIR="$RYSMITH_BASE/rysmith_set_${SET}"
    for (( i=START; i<END; i++ )); do
        [ -f "$SET_DIR/func_set${SET}_${i}/func.c" ] && continue

        # attempt 1: set-specific params
        local EC=0
        timeout "$TIMEOUT" "$RYSMITH_BIN" \
            --Xnum-bbls-per-fun    "$BBLS" \
            --Xnum-vars-per-fun    "$VARS" \
            --Xnum-assigns-per-bbl "$ASSIGNS" \
            --Xnum-vars-per-assign "$VPA" \
            --Xnum-vars-in-cond    "$VIC" \
            --Xbitwuzla-threads    "$BITWUZLA_THREADS" \
            --Xdisable-struct-vars \
            $RYSMITH_COVERAGE_FLAGS \
            -S -s "$RANDOM" \
            -o "$SET_DIR" -n "$i" "set${SET}" \
            > /dev/null 2>&1 || EC=$?

        # attempts 2-4: retry with default params up to 3 times
        if [ "$EC" != "0" ] || [ ! -f "$SET_DIR/func_set${SET}_${i}/func.c" ]; then
            rm -rf "$SET_DIR/func_set${SET}_${i}" 2>/dev/null || true
            local RETRY=0
            while [ $RETRY -lt 3 ]; do
                EC=0
                timeout "$TIMEOUT" "$RYSMITH_BIN" \
                    --Xnum-bbls-per-fun    "$DEFAULT_BBLS" \
                    --Xnum-vars-per-fun    "$DEFAULT_VARS" \
                    --Xnum-assigns-per-bbl "$DEFAULT_ASSIGNS" \
                    --Xnum-vars-per-assign "$DEFAULT_VPA" \
                    --Xnum-vars-in-cond    "$DEFAULT_VIC" \
                    --Xbitwuzla-threads    "$BITWUZLA_THREADS" \
                    --Xdisable-struct-vars \
                    $RYSMITH_COVERAGE_FLAGS \
                    -S -s "$RANDOM" \
                    -o "$SET_DIR" -n "$i" "set${SET}" \
                    > /dev/null 2>&1 || EC=$?
                [ "$EC" = "0" ] && [ -f "$SET_DIR/func_set${SET}_${i}/func.c" ] && break
                rm -rf "$SET_DIR/func_set${SET}_${i}" 2>/dev/null || true
                RETRY=$(( RETRY + 1 ))
            done
        fi
    done
}

_rysmith_set_worker() {
    local SET="$1" CONFIG="$2"
    local SET_DIR="$RYSMITH_BASE/rysmith_set_${SET}"
    read -r BBLS VARS ASSIGNS VPA VIC <<< "$CONFIG"
    mkdir -p "$SET_DIR"

    local CHUNK=$(( (RYSMITH_N + WORKERS_PER_SET - 1) / WORKERS_PER_SET ))
    local WPIDS=()
    for (( w=0; w<WORKERS_PER_SET; w++ )); do
        local START=$(( w * CHUNK ))
        local END=$(( START + CHUNK ))
        [ "$END" -gt "$RYSMITH_N" ] && END=$RYSMITH_N
        [ "$START" -ge "$RYSMITH_N" ] && break
        _rysmith_chunk_worker "$SET" "$BBLS" "$VARS" "$ASSIGNS" "$VPA" "$VIC" "$START" "$END" &
        WPIDS+=($!)
    done
    for pid in "${WPIDS[@]}"; do wait "$pid"; done

    local CURRENT
    CURRENT=$(find "$SET_DIR" -name "func.c" | wc -l)
    echo "set${SET}: $CURRENT/$RYSMITH_N functions" \
        >> "$RYSMITH_BASE/rysmith_set_${SET}.log" 2>&1
}
export -f _rysmith_set_worker _rysmith_chunk_worker
export RYSMITH_BASE RYSMITH_BIN RYSMITH_N TIMEOUT BITWUZLA_THREADS WORKERS_PER_SET
export RYSMITH_COVERAGE_FLAGS
export DEFAULT_BBLS DEFAULT_VARS DEFAULT_ASSIGNS DEFAULT_VPA DEFAULT_VIC

RYSMITH_PIDS=()
for (( SET=0; SET<RYSMITH_SETS; SET++ )); do
    CONFIG="${RYSMITH_CONFIGS[$SET]}"
    bash -c "_rysmith_set_worker $SET '$CONFIG'" \
        >> "$RYSMITH_BASE/rysmith_set_${SET}.log" 2>&1 &
    RYSMITH_PIDS+=($!)
    info "  Launched set $SET (pid=${RYSMITH_PIDS[-1]}): config=$CONFIG"
done
RYSMITH_BGPIDS=("${RYSMITH_PIDS[@]}")

# ── WAIT for all Phase 1 ──────────────────────────────────────────────────────
info "Waiting for Csmith, YARPGen, and Rysmith to complete..."
ALL_PHASE1_PIDS=("${CSMITH_BGPIDS[@]}" "${YARPGEN_BGPIDS[@]}" "${RYSMITH_BGPIDS[@]}")
while true; do
    CS=$(find "$CSMITH_OUT"   -name "*.c"  2>/dev/null | wc -l)
    YG=$(find "$YARPGEN_OUT"  -mindepth 1 -maxdepth 1 -type d -name "yarpgen_sample_*" 2>/dev/null | wc -l)
    RS=$(find "$RYSMITH_BASE" -name "func.c" 2>/dev/null | wc -l)
    RT=$(( RYSMITH_N * RYSMITH_SETS ))
    echo -ne "\r  csmith=$CS/$CSMITH_N  yarpgen=$YG/$YARPGEN_N  rysmith=$RS/$RT  "
    all_done=1
    for pid in "${ALL_PHASE1_PIDS[@]}"; do
        kill -0 "$pid" 2>/dev/null && { all_done=0; break; }
    done
    [ "$all_done" = "1" ] && break
    sleep 3
done
wait
echo ""
info "Phase 1 complete."
info "  Csmith:  $(find "$CSMITH_OUT"  -name "*.c" 2>/dev/null | wc -l)/$CSMITH_N"
info "  YARPGen: $(find "$YARPGEN_OUT" -mindepth 1 -maxdepth 1 -type d -name "yarpgen_sample_*" 2>/dev/null | wc -l)/$YARPGEN_N"
info "  Rysmith: $(find "$RYSMITH_BASE" -name "func.c" 2>/dev/null | wc -l)/$RT"

# =============================================================================
# RYSMITH_ALL — merge all sets into one flat dir for rylink
# =============================================================================
info "Merging all rysmith sets into $RYSMITH_ALL..."
for (( SET=0; SET<RYSMITH_SETS; SET++ )); do
    SET_DIR="$RYSMITH_BASE/rysmith_set_${SET}"
    [ -d "$SET_DIR" ] || continue
    find "$SET_DIR" -mindepth 1 -maxdepth 1 -type d -name "func_*" | while read -r d; do
        DEST="$RYSMITH_ALL/$(basename "$d")_s${SET}"
        [ -d "$DEST" ] || cp -r "$d" "$DEST"
    done
done
TOTAL_FUNCS=$(find "$RYSMITH_ALL" -name "func.c" | wc -l)
info "rysmith_all: $TOTAL_FUNCS leaf functions"

# =============================================================================
# PHASE 2 — Rylink (depends on rysmith_all being ready)
# =============================================================================
header "Phase 2: Generating Rylink programs (depths 2-11 in parallel)"

_rylink_depth_worker() {
    local DEPTH="$1"
    local DEPTH_DIR="$RYLINK_BASE/rylink_depth_${DEPTH}"
    rm -rf "$DEPTH_DIR"
    cp -r "$RYSMITH_ALL" "$DEPTH_DIR"

    local EC=0
    timeout $(( TIMEOUT * RYLINK_N )) "$RYLINK_BIN" \
        -i "$DEPTH_DIR" \
        -l "$RYLINK_N" \
        --Xfunction-depth "$DEPTH" \
        "depth${DEPTH}" \
        > /dev/null 2>&1 || EC=$?

    local SUCC
    SUCC=$(find "$DEPTH_DIR" -maxdepth 1 -name "prog_*" -type d 2>/dev/null | wc -l)
    if [ "$EC" = "124" ]; then
        echo "depth${DEPTH}: TIMEOUT, got $SUCC/$RYLINK_N" \
            >> "$RYLINK_BASE/rylink_depth_${DEPTH}.log"
    else
        echo "depth${DEPTH}: $SUCC/$RYLINK_N programs" \
            >> "$RYLINK_BASE/rylink_depth_${DEPTH}.log"
    fi
}
export -f _rylink_depth_worker
export RYLINK_BASE RYLINK_BIN RYSMITH_ALL RYLINK_N TIMEOUT

RYLINK_PIDS=()
for DEPTH in $RYLINK_DEPTHS; do
    bash -c "_rylink_depth_worker $DEPTH" \
        >> "$RYLINK_BASE/rylink_depth_${DEPTH}.log" 2>&1 &
    RYLINK_PIDS+=($!)
    info "  Launched depth=$DEPTH (pid=${RYLINK_PIDS[-1]})"
done

while true; do
    TOTAL_PROGS=$(find "$RYLINK_BASE" -maxdepth 2 -name "prog_*" -type d 2>/dev/null | wc -l)
    TOTAL_TARGET=$(( RYLINK_N * 10 ))
    echo -ne "\r  rylink: $TOTAL_PROGS/$TOTAL_TARGET programs  "
    all_done=1
    for pid in "${RYLINK_PIDS[@]}"; do
        kill -0 "$pid" 2>/dev/null && { all_done=0; break; }
    done
    [ "$all_done" = "1" ] && break
    sleep 5
done
wait
echo ""
TOTAL_PROGS=$(find "$RYLINK_BASE" -maxdepth 2 -name "prog_*" -type d | wc -l)
info "Rylink done: $TOTAL_PROGS programs across all depths"

# =============================================================================
# RYLINK_ALL — merge all depths into one flat dir
# =============================================================================
info "Merging all rylink depths into $RYLINK_ALL..."
for DEPTH in $RYLINK_DEPTHS; do
    DEPTH_DIR="$RYLINK_BASE/rylink_depth_${DEPTH}"
    [ -d "$DEPTH_DIR" ] || continue
    find "$DEPTH_DIR" -mindepth 1 -maxdepth 1 -type d -name "prog_*" | while read -r d; do
        DEST="$RYLINK_ALL/$(basename "$d")_d${DEPTH}"
        [ -d "$DEST" ] || cp -r "$d" "$DEST"
    done
done
TOTAL_PROGS=$(find "$RYLINK_ALL" -maxdepth 1 -name "prog_*" -type d | wc -l)
info "rylink_all: $TOTAL_PROGS programs"

# =============================================================================
# SUMMARY
# =============================================================================
echo ""
echo "=============================================================================="
echo "  Generation complete."
echo "  $CSMITH_OUT/   — $(find "$CSMITH_OUT" -name "*.c" 2>/dev/null | wc -l) csmith programs"
echo "  $YARPGEN_OUT/  — $(find "$YARPGEN_OUT" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | wc -l) yarpgen samples"
for (( SET=0; SET<RYSMITH_SETS; SET++ )); do
    D="$RYSMITH_BASE/rysmith_set_${SET}"
    N_=$(find "$D" -name "func.c" 2>/dev/null | wc -l)
    CONFIG="${RYSMITH_CONFIGS[$SET]}"
    read -r BBLS VARS ASSIGNS VPA VIC <<< "$CONFIG"
    printf "  rysmith_set_%d/ — %4d funcs  [bbls=%s vars=%s assigns=%s vpa=%s vic=%s]\n" \
        "$SET" "$N_" "$BBLS" "$VARS" "$ASSIGNS" "$VPA" "$VIC"
done
printf "  rysmith_all/   — %4d funcs total\n" "$(find "$RYSMITH_ALL" -name "func.c" 2>/dev/null | wc -l)"
for DEPTH in $RYLINK_DEPTHS; do
    D="$RYLINK_BASE/rylink_depth_${DEPTH}"
    N_=$(find "$D" -maxdepth 1 -name "prog_*" -type d 2>/dev/null | wc -l)
    printf "  rylink_depth_%d/ — %4d programs\n" "$DEPTH" "$N_"
done
printf "  rylink_all/    — %4d programs total\n" "$TOTAL_PROGS"
echo "=============================================================================="
