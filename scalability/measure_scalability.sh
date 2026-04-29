#!/bin/bash
# =============================================================================
# benchmark_rysmith.sh
# Sweeps rysmith and rylink parameters and measures success rate and throughput.
#
# Rysmith sweeps:
#   bbls: 3-30 step 1, config [*, 10, 1, 1, 2, -]
#   vars: 3-20,        config [10, *, 1, 1, 2, -]
#
# Rylink sweep:
#   --Xfunction-depth: 1-20 step 1
# =============================================================================
#set -euo pipefail

# =============================================================================
# DEFAULTS
# =============================================================================
N=1000
TIMEOUT=3
RYSMITH_BIN="/reify/build/bin/rysmith"
RYLINK_BIN="/reify/build/bin/rylink"
OUTPUT_DIR="/reify/data/scalability/benchmark_rysmith"
DEFAULT_BITWUZLA_THREADS=8

# BBL sweep defaults
BBLS_MIN=3
BBLS_MAX=30
BBLS_STEP=1
BBLS_SWEEP_VARS=10
BBLS_SWEEP_ASSIGNS=1
BBLS_SWEEP_VPA=1
BBLS_SWEEP_VIC=2

# VARS sweep defaults
VARS_MIN=3
VARS_MAX=20
VARS_STEP=1
VARS_SWEEP_BBLS=10
VARS_SWEEP_ASSIGNS=1
VARS_SWEEP_VPA=1
VARS_SWEEP_VIC=2

# RYLINK depth sweep defaults
DEPTH_MIN=1
DEPTH_MAX=20
DEPTH_STEP=1

# Which sweeps to run (all enabled by default)
RUN_BBLS_SWEEP=1
RUN_VARS_SWEEP=1
RUN_DEPTH_SWEEP=1

# Rylink leaf-function source override (auto-detected if empty)
RYLINK_INPUT_OVERRIDE=""

# =============================================================================
# COLORS / LOGGING
# =============================================================================
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; NC='\033[0m'
info()   { echo -e "${GREEN}[INFO]${NC} $*"; }
warn()   { echo -e "${YELLOW}[WARN]${NC} $*"; }
die()    { echo -e "${RED}[ERROR]${NC} $*" >&2; exit 1; }
header() {
    echo -e "\n${BLUE}======================================${NC}"
    echo -e "${BLUE}  $*${NC}"
    echo -e "${BLUE}======================================${NC}"
}

# =============================================================================
# HELP
# =============================================================================
usage() {
    cat <<EOF

${BLUE}Usage:${NC} $(basename "$0") [OPTIONS]

Sweeps rysmith and rylink parameters, measuring success rate and throughput.

${BLUE}General Options:${NC}
  -h, --help                    Show this help message and exit
  -n, --num-programs    INT     Number of programs to generate per parameter
                                value  [default: $N]
  -t, --timeout         SECS    Per-program timeout in seconds  [default: $TIMEOUT]
  -o, --output-dir      PATH    Root directory for all output  [default: $OUTPUT_DIR]
      --rysmith         PATH    Path to rysmith binary  [default: $RYSMITH_BIN]
      --rylink          PATH    Path to rylink binary   [default: $RYLINK_BIN]
      --threads         INT     --Xbitwuzla-threads value  [default: $DEFAULT_BITWUZLA_THREADS]

${BLUE}Sweep Selection:${NC}
      --only-bbls               Run only the bbls sweep
      --only-vars               Run only the vars sweep
      --only-depth              Run only the rylink depth sweep
      --skip-bbls               Skip the bbls sweep
      --skip-vars               Skip the vars sweep
      --skip-depth              Skip the rylink depth sweep

${BLUE}BBL Sweep  [sweeps --Xnum-bbls-per-fun]:${NC}
      --bbls-min        INT     Start of bbls range  [default: $BBLS_MIN]
      --bbls-max        INT     End of bbls range    [default: $BBLS_MAX]
      --bbls-step       INT     Step size            [default: $BBLS_STEP]
      --bbls-vars       INT     Fixed --Xnum-vars-per-fun   [default: $BBLS_SWEEP_VARS]
      --bbls-assigns    INT     Fixed --Xnum-assigns-per-bbl[default: $BBLS_SWEEP_ASSIGNS]
      --bbls-vpa        INT     Fixed --Xnum-vars-per-assign[default: $BBLS_SWEEP_VPA]
      --bbls-vic        INT     Fixed --Xnum-vars-in-cond   [default: $BBLS_SWEEP_VIC]

${BLUE}Vars Sweep  [sweeps --Xnum-vars-per-fun]:${NC}
      --vars-min        INT     Start of vars range  [default: $VARS_MIN]
      --vars-max        INT     End of vars range    [default: $VARS_MAX]
      --vars-step       INT     Step size            [default: $VARS_STEP]
      --vars-bbls       INT     Fixed --Xnum-bbls-per-fun   [default: $VARS_SWEEP_BBLS]
      --vars-assigns    INT     Fixed --Xnum-assigns-per-bbl[default: $VARS_SWEEP_ASSIGNS]
      --vars-vpa        INT     Fixed --Xnum-vars-per-assign[default: $VARS_SWEEP_VPA]
      --vars-vic        INT     Fixed --Xnum-vars-in-cond   [default: $VARS_SWEEP_VIC]

${BLUE}Rylink Depth Sweep  [sweeps --Xfunction-depth]:${NC}
      --depth-min       INT     Start of depth range [default: $DEPTH_MIN]
      --depth-max       INT     End of depth range   [default: $DEPTH_MAX]
      --depth-step      INT     Step size            [default: $DEPTH_STEP]
      --rylink-input    PATH    Directory of leaf functions to link.
                                Defaults to: \$OUTPUT_DIR/bbls_10 if it
                                contains func.c files, else /reify/samples

${BLUE}Examples:${NC}
  # Quick smoke test: 50 programs, 5s timeout, only bbls sweep from 3-10
  $(basename "$0") -n 50 -t 5 --only-bbls --bbls-max 10

  # Full run with 16 bitwuzla threads, custom output dir
  $(basename "$0") --threads 16 -o /tmp/bench_out

  # Vars sweep only, wider range, coarser step
  $(basename "$0") --only-vars --vars-min 5 --vars-max 30 --vars-step 2

  # Rylink depth sweep against a custom set of leaf functions
  $(basename "$0") --only-depth --rylink-input /reify/samples --depth-max 30

EOF
}

# =============================================================================
# ARGUMENT PARSING
# =============================================================================
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)            usage; exit 0 ;;

        # General
        -n|--num-programs)    N="$2";                        shift 2 ;;
        -t|--timeout)         TIMEOUT="$2";                  shift 2 ;;
        -o|--output-dir)      OUTPUT_DIR="$2";               shift 2 ;;
        --rysmith)            RYSMITH_BIN="$2";              shift 2 ;;
        --rylink)             RYLINK_BIN="$2";               shift 2 ;;
        --threads)            DEFAULT_BITWUZLA_THREADS="$2"; shift 2 ;;

        # Sweep selection
        --only-bbls)          RUN_BBLS_SWEEP=1; RUN_VARS_SWEEP=0; RUN_DEPTH_SWEEP=0; shift ;;
        --only-vars)          RUN_BBLS_SWEEP=0; RUN_VARS_SWEEP=1; RUN_DEPTH_SWEEP=0; shift ;;
        --only-depth)         RUN_BBLS_SWEEP=0; RUN_VARS_SWEEP=0; RUN_DEPTH_SWEEP=1; shift ;;
        --skip-bbls)          RUN_BBLS_SWEEP=0; shift ;;
        --skip-vars)          RUN_VARS_SWEEP=0; shift ;;
        --skip-depth)         RUN_DEPTH_SWEEP=0; shift ;;

        # BBL sweep
        --bbls-min)           BBLS_MIN="$2";            shift 2 ;;
        --bbls-max)           BBLS_MAX="$2";            shift 2 ;;
        --bbls-step)          BBLS_STEP="$2";           shift 2 ;;
        --bbls-vars)          BBLS_SWEEP_VARS="$2";     shift 2 ;;
        --bbls-assigns)       BBLS_SWEEP_ASSIGNS="$2";  shift 2 ;;
        --bbls-vpa)           BBLS_SWEEP_VPA="$2";      shift 2 ;;
        --bbls-vic)           BBLS_SWEEP_VIC="$2";      shift 2 ;;

        # Vars sweep
        --vars-min)           VARS_MIN="$2";            shift 2 ;;
        --vars-max)           VARS_MAX="$2";            shift 2 ;;
        --vars-step)          VARS_STEP="$2";           shift 2 ;;
        --vars-bbls)          VARS_SWEEP_BBLS="$2";     shift 2 ;;
        --vars-assigns)       VARS_SWEEP_ASSIGNS="$2";  shift 2 ;;
        --vars-vpa)           VARS_SWEEP_VPA="$2";      shift 2 ;;
        --vars-vic)           VARS_SWEEP_VIC="$2";      shift 2 ;;

        # Depth sweep
        --depth-min)          DEPTH_MIN="$2";           shift 2 ;;
        --depth-max)          DEPTH_MAX="$2";           shift 2 ;;
        --depth-step)         DEPTH_STEP="$2";          shift 2 ;;
        --rylink-input)       RYLINK_INPUT_OVERRIDE="$2"; shift 2 ;;

        *) die "Unknown option: $1  (try --help)" ;;
    esac
done

# =============================================================================
# VALIDATION
# =============================================================================
[ -f "$RYSMITH_BIN" ] || die "rysmith not found at $RYSMITH_BIN"
[ -f "$RYLINK_BIN" ]  || die "rylink not found at $RYLINK_BIN"

for var_name in N TIMEOUT DEFAULT_BITWUZLA_THREADS \
    BBLS_MIN BBLS_MAX BBLS_STEP BBLS_SWEEP_VARS BBLS_SWEEP_ASSIGNS BBLS_SWEEP_VPA BBLS_SWEEP_VIC \
    VARS_MIN VARS_MAX VARS_STEP VARS_SWEEP_BBLS VARS_SWEEP_ASSIGNS VARS_SWEEP_VPA VARS_SWEEP_VIC \
    DEPTH_MIN DEPTH_MAX DEPTH_STEP; do
    val="${!var_name}"
    [[ "$val" =~ ^[0-9]+$ ]] || die "--${var_name} must be a positive integer, got: '$val'"
done

[ "$BBLS_MIN"  -le "$BBLS_MAX"  ] || die "--bbls-min ($BBLS_MIN) must be <= --bbls-max ($BBLS_MAX)"
[ "$VARS_MIN"  -le "$VARS_MAX"  ] || die "--vars-min ($VARS_MIN) must be <= --vars-max ($VARS_MAX)"
[ "$DEPTH_MIN" -le "$DEPTH_MAX" ] || die "--depth-min ($DEPTH_MIN) must be <= --depth-max ($DEPTH_MAX)"

mkdir -p "$OUTPUT_DIR"

# Build ranges now that all params are validated
BBLS_RANGE="$(seq "$BBLS_MIN"  "$BBLS_STEP"  "$BBLS_MAX")"
VARS_RANGE="$(seq "$VARS_MIN"  "$VARS_STEP"  "$VARS_MAX")"
DEPTH_RANGE="$(seq "$DEPTH_MIN" "$DEPTH_STEP" "$DEPTH_MAX")"

# =============================================================================
# PRINT EFFECTIVE CONFIGURATION
# =============================================================================
info "Effective configuration:"
echo "  General:  N=$N  TIMEOUT=${TIMEOUT}s  threads=$DEFAULT_BITWUZLA_THREADS"
echo "  Output:   $OUTPUT_DIR"
echo "  Binaries: rysmith=$RYSMITH_BIN  rylink=$RYLINK_BIN"
echo "  Sweeps:   bbls=$([ "$RUN_BBLS_SWEEP"  = 1 ] && echo ON || echo OFF)"
echo "            vars=$([ "$RUN_VARS_SWEEP"  = 1 ] && echo ON || echo OFF)"
echo "            depth=$([ "$RUN_DEPTH_SWEEP" = 1 ] && echo ON || echo OFF)"
if [ "$RUN_BBLS_SWEEP" = 1 ]; then
    echo "  BBL sweep:   range=${BBLS_MIN}-${BBLS_MAX} step=${BBLS_STEP}  fixed: vars=$BBLS_SWEEP_VARS assigns=$BBLS_SWEEP_ASSIGNS vpa=$BBLS_SWEEP_VPA vic=$BBLS_SWEEP_VIC"
fi
if [ "$RUN_VARS_SWEEP" = 1 ]; then
    echo "  Var sweep:   range=${VARS_MIN}-${VARS_MAX} step=${VARS_STEP}  fixed: bbls=$VARS_SWEEP_BBLS assigns=$VARS_SWEEP_ASSIGNS vpa=$VARS_SWEEP_VPA vic=$VARS_SWEEP_VIC"
fi
if [ "$RUN_DEPTH_SWEEP" = 1 ]; then
    echo "  Depth sweep: range=${DEPTH_MIN}-${DEPTH_MAX} step=${DEPTH_STEP}"
fi

# =============================================================================
# run_rysmith_sweep <param_name> <param_value> <bbls> <vars> <assigns> <vpa> <vic>
# =============================================================================
run_rysmith_sweep() {
    local PARAM_NAME="$1"
    local PARAM_VAL="$2"
    local BBLS="$3"
    local VARS="$4"
    local ASSIGNS="$5"
    local VPA="$6"
    local VIC="$7"

    local OUTDIR="$OUTPUT_DIR/${PARAM_NAME}_${PARAM_VAL}"
    rm -rf "$OUTDIR"
    mkdir -p "$OUTDIR"

    local SUCC=0 FAIL=0 TMO=0 SUCC_TIME_NS=0
    local T_START P_START P_END EXIT_CODE
    T_START=$(date +%s%N)

    for (( i=0; i<N; i++ )); do
        EXIT_CODE=0
        P_START=$(date +%s%N)
        echo -ne "\r  [$((i+1))/$N] bbls=$BBLS vars=$VARS OK:$SUCC TMO:$TMO FAIL:$FAIL  " >&2
        timeout "$TIMEOUT" "$RYSMITH_BIN" \
            --Xnum-bbls-per-fun    "$BBLS" \
            --Xnum-vars-per-fun    "$VARS" \
            --Xnum-assigns-per-bbl "$ASSIGNS" \
            --Xnum-vars-per-assign "$VPA" \
            --Xnum-vars-in-cond    "$VIC" \
            --Xbitwuzla-threads    "$DEFAULT_BITWUZLA_THREADS" \
            --Xdisable-struct-vars \
            -S \
            -o "$OUTDIR" \
            -n "$i" \
            sweep \
            > /dev/null 2>&1 || EXIT_CODE=$?
        P_END=$(date +%s%N)

        if   [ "$EXIT_CODE" = "124" ]; then
            (( TMO++ ))
        elif [ -f "$OUTDIR/func_sweep_${i}/func.c" ]; then
            (( SUCC++ ))
            SUCC_TIME_NS=$(( SUCC_TIME_NS + P_END - P_START ))
        else
            (( FAIL++ ))
        fi
    done

    local T_END TOTAL_TIME SUCC_PCT TMO_PCT THROUGHPUT AVG_SUCC_TIME
    T_END=$(date +%s%N)
    TOTAL_TIME=$(awk "BEGIN {printf \"%.3f\", ($T_END - $T_START) / 1000000000}")
    local TOTAL=$(( SUCC + FAIL + TMO ))
    SUCC_PCT=$(awk  "BEGIN {printf \"%.1f\", $SUCC * 100 / ($TOTAL > 0 ? $TOTAL : 1)}")
    TMO_PCT=$(awk   "BEGIN {printf \"%.1f\", $TMO  * 100 / ($TOTAL > 0 ? $TOTAL : 1)}")
    THROUGHPUT=$(awk "BEGIN {printf \"%.2f\", $SUCC / ($TOTAL_TIME > 0 ? $TOTAL_TIME : 1)}")
    if (( SUCC > 0 )); then
        AVG_SUCC_TIME=$(awk "BEGIN {printf \"%.3f\", $SUCC_TIME_NS / 1000000000 / $SUCC}")
    else
        AVG_SUCC_TIME="N/A"
    fi

    echo "$PARAM_VAL $SUCC_PCT $TMO_PCT $THROUGHPUT $AVG_SUCC_TIME $SUCC $TMO $FAIL"
}

# =============================================================================
# run_rylink_sweep <depth> <input_dir>
# =============================================================================
run_rylink_sweep() {
    local DEPTH="$1"
    local INPUT_DIR="$2"
    local OUTDIR="$OUTPUT_DIR/rylink_depth_${DEPTH}"
    rm -rf "$OUTDIR"
    mkdir -p "$OUTDIR"
    cp -r "$INPUT_DIR/." "$OUTDIR/"

    local P_START P_END EXIT_CODE SUCC TMO FAIL AVG_SUCC_TIME
    P_START=$(date +%s%N)
    EXIT_CODE=0
    timeout $(( TIMEOUT * N )) "$RYLINK_BIN" \
        -i "$OUTDIR" \
        -l "$N" \
        --Xfunction-depth "$DEPTH" \
        rylink_sweep \
        2>/dev/null || EXIT_CODE=$?
    P_END=$(date +%s%N)

    local TOTAL_TIME SUCC_PCT TMO_PCT THROUGHPUT
    TOTAL_TIME=$(awk "BEGIN {printf \"%.3f\", ($P_END - $P_START) / 1000000000}")

    if [ "$EXIT_CODE" = "124" ]; then
        TMO=$N; SUCC=0; FAIL=0
        AVG_SUCC_TIME="N/A"
    else
        SUCC=$(find "$OUTDIR" -maxdepth 1 -name "prog_*" -type d 2>/dev/null | wc -l)
        FAIL=$(( N - SUCC < 0 ? 0 : N - SUCC ))
        TMO=0
        if (( SUCC > 0 )); then
            AVG_SUCC_TIME=$(awk "BEGIN {printf \"%.3f\", ($P_END - $P_START) / 1000000000 / $SUCC}")
        else
            AVG_SUCC_TIME="N/A"
        fi
    fi

    local TOTAL=$(( SUCC + FAIL + TMO ))
    SUCC_PCT=$(awk  "BEGIN {printf \"%.1f\", $SUCC * 100 / ($TOTAL > 0 ? $TOTAL : 1)}")
    TMO_PCT=$(awk   "BEGIN {printf \"%.1f\", $TMO  * 100 / ($TOTAL > 0 ? $TOTAL : 1)}")
    THROUGHPUT=$(awk "BEGIN {printf \"%.2f\", $SUCC / ($TOTAL_TIME > 0 ? $TOTAL_TIME : 1)}")

    echo "$DEPTH $SUCC_PCT $TMO_PCT $THROUGHPUT $AVG_SUCC_TIME $SUCC $TMO $FAIL"
}

# =============================================================================
# print_table <param_label> <rows...>
# =============================================================================
print_table() {
    local PARAM="$1"; shift
    local RESULTS=("$@")
    echo ""
    printf "  %-8s %8s %8s %14s %14s  (%s/%s/%s)\n" \
        "$PARAM" "SUCC%" "TMO%" "Throughput/s" "AvgSuccTime" "OK" "TMO" "FAIL"
    echo "  ------------------------------------------------------------------------"
    for row in "${RESULTS[@]}"; do
        read -r VAL SUCC_PCT TMO_PCT THROUGHPUT AVG_SUCC_TIME SUCC TMO FAIL <<< "$row"
        printf "  %-8s %8s %8s %14s %14s  (%s/%s/%s)\n" \
            "$VAL" "${SUCC_PCT}%" "${TMO_PCT}%" \
            "${THROUGHPUT}/s" "${AVG_SUCC_TIME}s" \
            "$SUCC" "$TMO" "$FAIL"
    done
}

# =============================================================================
# SWEEP 1: rysmith --Xnum-bbls-per-fun
# =============================================================================
if [ "$RUN_BBLS_SWEEP" = 1 ]; then
    header "Rysmith: sweeping --Xnum-bbls-per-fun  [*, $BBLS_SWEEP_VARS, $BBLS_SWEEP_ASSIGNS, $BBLS_SWEEP_VPA, $BBLS_SWEEP_VIC]"
    RESULTS=()
    for VAL in $BBLS_RANGE; do
        echo -ne "\r  bbls=$VAL ...    " >&2
        ROW=$(run_rysmith_sweep "bbls" "$VAL" \
            "$VAL" "$BBLS_SWEEP_VARS" "$BBLS_SWEEP_ASSIGNS" \
            "$BBLS_SWEEP_VPA" "$BBLS_SWEEP_VIC")
        RESULTS+=("$ROW")
    done
    echo ""
    print_table "bbls" "${RESULTS[@]}"
fi

# =============================================================================
# SWEEP 2: rysmith --Xnum-vars-per-fun
# =============================================================================
if [ "$RUN_VARS_SWEEP" = 1 ]; then
    header "Rysmith: sweeping --Xnum-vars-per-fun   [$VARS_SWEEP_BBLS, *, $VARS_SWEEP_ASSIGNS, $VARS_SWEEP_VPA, $VARS_SWEEP_VIC]"
    RESULTS=()
    for VAL in $VARS_RANGE; do
        echo -ne "\r  vars=$VAL ...    " >&2
        VPA=$(( VARS_SWEEP_VPA < VAL ? VARS_SWEEP_VPA : VAL ))
        VIC=$(( VARS_SWEEP_VIC < VAL ? VARS_SWEEP_VIC : VAL ))
        ROW=$(run_rysmith_sweep "vars" "$VAL" \
            "$VARS_SWEEP_BBLS" "$VAL" "$VARS_SWEEP_ASSIGNS" \
            "$VPA" "$VIC")
        RESULTS+=("$ROW")
    done
    echo ""
    print_table "vars" "${RESULTS[@]}"
fi

# =============================================================================
# SWEEP 3: rylink --Xfunction-depth
# =============================================================================
if [ "$RUN_DEPTH_SWEEP" = 1 ]; then
    if [ -n "$RYLINK_INPUT_OVERRIDE" ]; then
        RYLINK_INPUT="$RYLINK_INPUT_OVERRIDE"
        [ -d "$RYLINK_INPUT" ] || die "--rylink-input path does not exist: $RYLINK_INPUT"
    else
        RYLINK_INPUT="$OUTPUT_DIR/bbls_10"
        if [ ! -d "$RYLINK_INPUT" ] || [ "$(find "$RYLINK_INPUT" -name "func.c" | wc -l)" -eq 0 ]; then
            RYLINK_INPUT="/reify/samples"
        fi
    fi
    info "Rylink sweep using leaf functions from: $RYLINK_INPUT"

    header "Rylink: sweeping --Xfunction-depth ($DEPTH_MIN to $DEPTH_MAX step $DEPTH_STEP)"
    RESULTS=()
    for DEPTH in $DEPTH_RANGE; do
        echo -ne "\r  depth=$DEPTH ...    " >&2
        ROW=$(run_rylink_sweep "$DEPTH" "$RYLINK_INPUT")
        RESULTS+=("$ROW")
    done
    echo ""
    print_table "depth" "${RESULTS[@]}"
fi

# =============================================================================
# FINAL SUMMARY
# =============================================================================
echo ""
echo "=============================================================================="
echo "  Benchmark complete. Output dirs in: $OUTPUT_DIR"
echo "  Parameters: N=$N programs per value, TIMEOUT=${TIMEOUT}s"
echo "=============================================================================="
rm -rf /reify/data/scalability
