#!/bin/bash
# =============================================================================
# run_coverage_experiment.sh
# Runs Steps 2+3 of the coverage experiment (generation + pipeline).
# See COVERAGE_INSTRUCTIONS.md for the full workflow including Step 1 (build).
#
# Usage:
#   ./run_coverage_experiment.sh [--full | --short] [--compiler gcc|llvm|both]
#                                [--skip-gen] TOOL [TOOL ...]
#
# Tools (space separated, any combination):
#   csmith   yarpgen   rysmith   rylink
#
# Examples:
#   ./run_coverage_experiment.sh --short csmith yarpgen
#   ./run_coverage_experiment.sh --full --compiler llvm rysmith rylink
#   ./run_coverage_experiment.sh --full --compiler both csmith yarpgen rysmith rylink
#   ./run_coverage_experiment.sh --short --skip-gen --compiler gcc rylink
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
GENERATE_SCRIPT="$SCRIPT_DIR/generate_data_coverage_parallel.sh"
GCC_PIPELINE="$SCRIPT_DIR/coverage_pipeline_gcc.sh"
LLVM_PIPELINE="$SCRIPT_DIR/coverage_pipeline_llvm.sh"

CSMITH_DATA="/reify/data/coverage/csmith"
YARPGEN_DATA="/reify/data/coverage/yarpgen"
RYSMITH_DATA="/reify/data/coverage/rysmith_all"
RYLINK_DATA="/reify/data/coverage/rylink_all"

GCC_BIN="/reify/coverage/builds/gcc-instrumented/bin/gcc"
CLANG_BIN="/reify/coverage/builds/llvm-instrumented/bin/clang"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; NC='\033[0m'
info()   { echo -e "${GREEN}[INFO]${NC} $*"; }
warn()   { echo -e "${YELLOW}[WARN]${NC} $*"; }
die()    { echo -e "${RED}[ERROR]${NC} $*" >&2; exit 1; }
header() { echo -e "\n${BLUE}============================================${NC}"; \
           echo -e "${BLUE}  $*${NC}"; \
           echo -e "${BLUE}============================================${NC}"; }

usage() {
    cat << 'EOF'
Usage: ./run_coverage_experiment.sh [OPTIONS] TOOL [TOOL ...]

NOTE: Build instrumented compilers separately first (see COVERAGE_INSTRUCTIONS.md):
  ./build_gcc_instrumented.sh
  ./build_llvm_instrumented.sh

Options:
  --full              Full run  (UNIT=10000: 10K csmith/yarpgen, 10x1K rysmith/rylink)
  --short             Short run (UNIT=10:    10 csmith/yarpgen,  10x1  rysmith/rylink)
  --compiler gcc      Run GCC coverage pipeline only (default)
  --compiler llvm     Run LLVM/Clang coverage pipeline only
  --compiler both     Run both GCC and LLVM coverage pipelines
  --skip-gen          Skip program generation, use existing data
  -h, --help          Show this help

Tools:
  csmith    yarpgen    rysmith    rylink

Examples:
  ./run_coverage_experiment.sh --short csmith yarpgen
  ./run_coverage_experiment.sh --full --compiler llvm rysmith rylink
  ./run_coverage_experiment.sh --full --compiler both csmith yarpgen rysmith rylink
  ./run_coverage_experiment.sh --short --skip-gen --compiler gcc rylink
EOF
    exit 0
}

[ $# -eq 0 ] && usage

MODE=""
COMPILER="gcc"
SKIP_GEN=0
TOOLS=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --full)       MODE="full";       shift ;;
        --short)      MODE="short";      shift ;;
        --compiler)   COMPILER="$2";     shift 2 ;;
        --skip-gen)   SKIP_GEN=1;        shift ;;
        -h|--help)    usage ;;
        csmith|yarpgen|rysmith|rylink)
                      TOOLS+=("$1");     shift ;;
        *) die "Unknown argument: $1. Use --help for usage." ;;
    esac
done

[ -z "$MODE" ]         && die "Must specify --full or --short."
[ ${#TOOLS[@]} -eq 0 ] && die "Must specify at least one tool."
[[ "$COMPILER" =~ ^(gcc|llvm|both)$ ]] || die "Invalid --compiler value: $COMPILER. Use gcc, llvm, or both."

[ -f "$GENERATE_SCRIPT" ] || die "generate_coverage_data.sh not found at $GENERATE_SCRIPT"

# validate compiler binaries
RUN_GCC=0; RUN_LLVM=0
case "$COMPILER" in
    gcc)  RUN_GCC=1 ;;
    llvm) RUN_LLVM=1 ;;
    both) RUN_GCC=1; RUN_LLVM=1 ;;
esac

if [ "$RUN_GCC"  = "1" ]; then
    [ -f "$GCC_BIN" ]   || die "Instrumented GCC not found at $GCC_BIN.\nRun ./build_gcc_instrumented.sh first."
    [ -f "$GCC_PIPELINE" ] || die "coverage_pipeline_gcc.sh not found at $GCC_PIPELINE"
fi
if [ "$RUN_LLVM" = "1" ]; then
    [ -f "$CLANG_BIN" ] || die "Instrumented Clang not found at $CLANG_BIN.\nRun ./build_llvm_instrumented.sh first."
    [ -f "$LLVM_PIPELINE" ] || die "coverage_pipeline_llvm.sh not found at $LLVM_PIPELINE"
fi

case "$MODE" in
    full)  UNIT=10000 ;;
    short) UNIT=10    ;;
esac

header "Coverage Experiment"
info "Mode:       $MODE (UNIT=$UNIT)"
info "Compiler:   $COMPILER"
info "Tools:      ${TOOLS[*]}"
info "Skip gen:   $SKIP_GEN"

# =============================================================================
# STEP 0 — ensure system dependencies are installed
# Done here once in the orchestrator rather than inside each pipeline script.
# =============================================================================
MISSING_DEPS=()
command -v lcov    &>/dev/null || MISSING_DEPS+=(lcov)
command -v genhtml &>/dev/null || MISSING_DEPS+=(genhtml)
command -v gcov    &>/dev/null || MISSING_DEPS+=(gcov)

if [ ${#MISSING_DEPS[@]} -gt 0 ]; then
    info "Installing missing dependencies: ${MISSING_DEPS[*]}"
    apt-get update -qq
    apt-get install -y lcov  # provides lcov, genhtml, and gcov
fi

# =============================================================================
# STEP 2 — generate
# =============================================================================
if [ "$SKIP_GEN" -eq 0 ]; then
    header "Step 2: Generating Program Data (UNIT=$UNIT)"
    bash "$GENERATE_SCRIPT" --unit "$UNIT"
    info "Generation complete."
else
    info "Step 2: Skipping generation (--skip-gen)"
fi

# =============================================================================
# BUILD PIPELINE ARGS
# =============================================================================
PIPELINE_ARGS=()
for TOOL in "${TOOLS[@]}"; do
    case "$TOOL" in
        csmith)  [ -d "$CSMITH_DATA" ]  || die "csmith data not found at $CSMITH_DATA"
                 PIPELINE_ARGS+=("csmith:$CSMITH_DATA") ;;
        yarpgen) [ -d "$YARPGEN_DATA" ] || die "yarpgen data not found at $YARPGEN_DATA"
                 PIPELINE_ARGS+=("yarpgen:$YARPGEN_DATA") ;;
        rysmith) [ -d "$RYSMITH_DATA" ] || die "rysmith data not found at $RYSMITH_DATA"
                 PIPELINE_ARGS+=("rysmith:$RYSMITH_DATA") ;;
        rylink)  [ -d "$RYLINK_DATA" ]  || die "rylink data not found at $RYLINK_DATA"
                 PIPELINE_ARGS+=("rylink:$RYLINK_DATA") ;;
    esac
done

# =============================================================================
# STEP 3 — coverage pipeline(s)
# =============================================================================
if [ "$RUN_GCC" = "1" ]; then
    header "Step 3a: Running GCC Coverage Pipeline"
    info "Running: coverage_pipeline_gcc.sh ${PIPELINE_ARGS[*]}"
    bash "$GCC_PIPELINE" "${PIPELINE_ARGS[@]}"
fi

if [ "$RUN_LLVM" = "1" ]; then
    header "Step 3b: Running LLVM Coverage Pipeline"
    info "Running: coverage_pipeline_llvm.sh ${PIPELINE_ARGS[*]}"
    bash "$LLVM_PIPELINE" "${PIPELINE_ARGS[@]}"
fi

# =============================================================================
# SUMMARY
# =============================================================================
header "Done"
info "Tools:      ${TOOLS[*]}"
info "Mode:       $MODE (UNIT=$UNIT)"
info "Compiler:   $COMPILER"
[ "$RUN_GCC"  = "1" ] && info "GCC report:  /reify/coverage/results/gcc_report/index.html"
[ "$RUN_LLVM" = "1" ] && info "LLVM report: /reify/coverage/results_llvm/llvm_report/index.html"
