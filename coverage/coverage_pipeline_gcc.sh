#!/bin/bash
# =============================================================================
# coverage_pipeline_gcc.sh
# Full GCC coverage pipeline: clean, parallel profile, merge, lcov, genhtml.
#
# Usage:
#   ./coverage_pipeline_gcc.sh [OPTIONS] TOOL_DIR [TOOL_DIR ...]
#
# Each TOOL_DIR is specified as:   <type>:<path>
# Supported types:
#   rysmith   — recursively finds func.c files
#   rylink    — recursively finds *.c files inside prog_* subdirs
#   csmith    — finds *.c files at top level, adds csmith include
#   yarpgen   — finds driver.c + func.c inside yarpgen_sample_* subdirs
#
# Examples:
#   ./coverage_pipeline_gcc.sh rysmith:/reify/samples csmith:/reify/data/performance/csmith
#   ./coverage_pipeline_gcc.sh rylink:/reify/data/performance/rylink yarpgen:/reify/data/performance/yarpgen
#   ./coverage_pipeline_gcc.sh rysmith:/reify/samples rysmith:/reify/data/performance/rysmith
#
# Options:
#   --gcc PATH          Path to instrumented GCC (default: /reify/coverage/builds/gcc-instrumented/bin/gcc)
#   --gcc-build PATH    Path to GCC build dir with .gcno files (default: /reify/coverage/builds/gcc-build)
#   --results PATH      Results output dir (default: /reify/coverage/results)
#   --timeout N         Seconds per compilation (default: 10)
#   --csmith-include P  Csmith include dir (default: /reify/previous_work/install/include)
#   -h, --help          Show this help
# =============================================================================
#set -euo pipefail

# =============================================================================
# DEFAULTS
# =============================================================================
GCC="/reify/coverage/builds/gcc-instrumented/bin/gcc"
GCC_BUILD="/reify/coverage/builds/gcc-build"
RESULTS_DIR="/reify/coverage/results"
CSMITH_INCLUDE="/reify/previous_work/install/include"
TIMEOUT_SECS=10
TOOL_DIRS=()

GREEN='\033[0;32m'; RED='\033[0;31m'; YELLOW='\033[1;33m'; NC='\033[0m'
info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
die()   { echo -e "${RED}[ERROR]${NC} $*" >&2; exit 1; }

usage() {
    sed -n 's/^# //p' "$0" | head -30
    exit 0
}

# =============================================================================
# PARSE ARGS
# =============================================================================
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)          usage ;;
        --gcc)              GCC="$2";            shift 2 ;;
        --gcc-build)        GCC_BUILD="$2";      shift 2 ;;
        --results)          RESULTS_DIR="$2";    shift 2 ;;
        --timeout)          TIMEOUT_SECS="$2";   shift 2 ;;
        --csmith-include)   CSMITH_INCLUDE="$2"; shift 2 ;;
        *:*)                TOOL_DIRS+=("$1");   shift ;;
        *)                  die "Unknown argument: $1. Use --help for usage." ;;
    esac
done

[ ${#TOOL_DIRS[@]} -eq 0 ] && die "No tool directories specified. Use --help for usage."

PROCS_DIR="$RESULTS_DIR/processes"
MERGED_DIR="$RESULTS_DIR/merged"
INFO_FILE="$RESULTS_DIR/gcc_coverage.info"
REPORT_DIR="$RESULTS_DIR/gcc_report"
LIBGCOVFLUSH="/reify/coverage/builds/libgcovflush.so"

PROCS=$(nproc)
GCOV_STRIP=$(echo "$GCC_BUILD" | tr '/' '\n' | grep -c .)

info "Using $PROCS parallel jobs, GCOV_PREFIX_STRIP=$GCOV_STRIP"
info "Tool directories:"
for td in "${TOOL_DIRS[@]}"; do
    TYPE="${td%%:*}"
    PATH_="${td#*:}"
    info "  [$TYPE] $PATH_"
done

# =============================================================================
# STEP 0 — build libgcovflush if not present
# =============================================================================
# Mismatched gcov-tool versions cause "mismatched profiles" crashes during merge.
GCC_INSTALL="$(dirname "$(dirname "$GCC")")"
GCOV_TOOL="/usr/bin/gcov"
GCOV_TOOL_BIN="/usr/bin/gcov-tool"
if [ ! -f "$GCOV_TOOL_BIN" ]; then
    warn "gcov-tool not found at $GCOV_TOOL_BIN, falling back to system gcov-tool"
    GCOV_TOOL_BIN="gcov-tool"
fi
info "Using gcov-tool: $GCOV_TOOL_BIN"
info "Using gcov:      $GCOV_TOOL"

if [ ! -f "$LIBGCOVFLUSH" ]; then
    info "Building libgcovflush.so..."
    cat > /tmp/gcovflush.c << 'EOF'
#include <signal.h>
#include <stdlib.h>
extern void __gcov_dump(void);
static void flush_on_signal(int sig) {
    __gcov_dump();
    signal(sig, SIG_DFL);
    raise(sig);
}
__attribute__((constructor))
static void install_handlers(void) {
    signal(SIGTERM, flush_on_signal);
    signal(SIGINT,  flush_on_signal);
    signal(SIGHUP,  flush_on_signal);
    atexit(__gcov_dump);
}
EOF
    gcc -O2 -shared -fPIC /tmp/gcovflush.c -o "$LIBGCOVFLUSH" -lgcov
    info "Built: $LIBGCOVFLUSH"
else
    info "libgcovflush.so already present, skipping build."
fi

# =============================================================================
# STEP 1 — clean
# =============================================================================
info "Step 1: Cleaning old .gcda files from $GCC_BUILD..."
find "$GCC_BUILD" -name "*.gcda" -delete

REMAINING=$(find "$GCC_BUILD" -name "*.gcda" | wc -l)
[ "$REMAINING" -ne 0 ] && die "$REMAINING .gcda files still present after clean."
info "Validation passed: build directory is clean."

rm -rf "$PROCS_DIR" "$MERGED_DIR"
mkdir -p "$PROCS_DIR" "$RESULTS_DIR"

# =============================================================================
# STEP 2 — collect work units from specified tool dirs
# Each unit is a directory whose files should be compiled together.
# Each line in the combined list: "<TYPE> <directory>"
# This keeps all files in a subdir together on the same process,
# avoiding dependency issues (e.g. yarpgen driver.c + func.c, rylink prog_*/).
# =============================================================================
info "Step 2: Collecting work units (directories)..."
COMBINED_LIST=$(mktemp)

for td in "${TOOL_DIRS[@]}"; do
    TYPE="${td%%:*}"
    DIR="${td#*:}"
    TYPE_UPPER=$(echo "$TYPE" | tr '[:lower:]' '[:upper:]')

    [ -d "$DIR" ] || { warn "  [$TYPE] $DIR not found, skipping."; continue; }

    case "$TYPE_UPPER" in
        RYSMITH)
            # each func_* subdir is one unit (contains func.c)
            COUNT=$(find "$DIR" -mindepth 1 -maxdepth 1 -type d -name "func_*" | wc -l)
            find "$DIR" -mindepth 1 -maxdepth 1 -type d -name "func_*" | sort | \
                sed "s|^|RYSMITH |" >> "$COMBINED_LIST"
            info "  [rysmith] $COUNT func_* dirs from $DIR"
            ;;
        RYLINK)
            # each prog_* subdir is one unit (contains main.c + func_*.c)
            COUNT=$(find "$DIR" -mindepth 1 -maxdepth 1 -type d -name "prog_*" | wc -l)
            find "$DIR" -mindepth 1 -maxdepth 1 -type d -name "prog_*" | sort | \
                sed "s|^|RYLINK |" >> "$COMBINED_LIST"
            info "  [rylink]  $COUNT prog_* dirs from $DIR"
            ;;
        CSMITH)
            # each .c file is its own unit (flat dir, no deps between files)
            COUNT=$(find "$DIR" -maxdepth 1 -name "*.c" | wc -l)
            find "$DIR" -maxdepth 1 -name "*.c" | sort | \
                awk '{print "CSMITH_FILE "$0}' >> "$COMBINED_LIST"
            info "  [csmith]  $COUNT .c files from $DIR"
            ;;
        YARPGEN)
            # each yarpgen_sample_* subdir is one unit (driver.c + func.c together)
            COUNT=$(find "$DIR" -mindepth 1 -maxdepth 1 -type d -name "yarpgen_sample_*" | wc -l)
            find "$DIR" -mindepth 1 -maxdepth 1 -type d -name "yarpgen_sample_*" | sort | \
                sed "s|^|YARPGEN |" >> "$COMBINED_LIST"
            info "  [yarpgen] $COUNT yarpgen_sample_* dirs from $DIR"
            ;;
        *)
            warn "  Unknown type '$TYPE', skipping. Valid: rysmith, rylink, csmith, yarpgen"
            ;;
    esac
done

TOTAL=$(wc -l < "$COMBINED_LIST")
[ "$TOTAL" -eq 0 ] && die "No work units found across all specified directories."
info "Total: $TOTAL work units, distributing across $PROCS processes."

# split round-robin into PROCS chunks
for (( p=0; p<PROCS; p++ )); do
    mkdir -p "$PROCS_DIR/process_$p"
    > "$PROCS_DIR/process_$p/files.txt"
done
i=0
while IFS= read -r line; do
    p=$(( i % PROCS ))
    echo "$line" >> "$PROCS_DIR/process_$p/files.txt"
    (( i++ ))
done < "$COMBINED_LIST"
rm -f "$COMBINED_LIST"

# =============================================================================
# STEP 3 — parallel profile
# =============================================================================
info "Step 3: Profiling compiler in parallel ($PROCS jobs)..."

for (( p=0; p<PROCS; p++ )); do
    (
        export GCOV_PREFIX="$PROCS_DIR/process_$p"
        export GCOV_PREFIX_STRIP="$GCOV_STRIP"
        [ -f "$PROCS_DIR/process_$p/files.txt" ] || exit 0
        while IFS= read -r line; do
            TOOL=$(echo "$line" | cut -d' ' -f1)
            UNIT=$(echo "$line" | cut -d' ' -f2-)
            case "$TOOL" in
                CSMITH_FILE)
                    # single file unit — compile directly
                    cd "$(dirname "$UNIT")"
                    f="$(basename "$UNIT")"
                    LD_PRELOAD="$LIBGCOVFLUSH" timeout "$TIMEOUT_SECS" \
                        "$GCC" -O3 -fprofile-update=atomic -I"$CSMITH_INCLUDE" -c "$f" -o /dev/null \
                        2>/dev/null || true
                    ;;
                RYSMITH)
                    # dir unit — compile func.c
                    cd "$UNIT"
                    LD_PRELOAD="$LIBGCOVFLUSH" timeout "$TIMEOUT_SECS" \
                        "$GCC" -O3 -fprofile-update=atomic -c func.c -o /dev/null \
                        2>/dev/null || true
                    ;;
                RYLINK)
                    # dir unit — compile all *.c files in the prog dir together
                    cd "$UNIT"
                    for f in *.c; do
                        [ -f "$f" ] || continue
                        LD_PRELOAD="$LIBGCOVFLUSH" timeout "$TIMEOUT_SECS" \
                            "$GCC" -O3 -fprofile-update=atomic -c "$f" -o /dev/null \
                            2>/dev/null || true
                    done
                    ;;
                YARPGEN)
                    # dir unit — compile driver.c and func.c together (func.c may include init.h)
                    cd "$UNIT"
                    for f in driver.c func.c; do
                        [ -f "$f" ] || continue
                        LD_PRELOAD="$LIBGCOVFLUSH" timeout "$TIMEOUT_SECS" \
                            "$GCC" -O3 -fprofile-update=atomic -c "$f" -o /dev/null \
                            2>/dev/null || true
                    done
                    ;;
            esac
        done < "$PROCS_DIR/process_$p/files.txt"
        echo "Process $p done."
    ) &
done

wait
info "All parallel jobs done."

# =============================================================================
# STEP 4 — merge .gcda files
# =============================================================================
info "Step 4: Merging .gcda files from $PROCS processes..."

cp -r "$PROCS_DIR/process_0/." "$MERGED_DIR/"

for (( p=1; p<PROCS; p++ )); do
    if [ -d "$PROCS_DIR/process_$p" ] && \
       [ "$(find "$PROCS_DIR/process_$p" -name "*.gcda" | wc -l)" -gt 0 ]; then
        tmpdir=$(mktemp -d)
        "$GCOV_TOOL_BIN" merge -o "$tmpdir" "$MERGED_DIR" "$PROCS_DIR/process_$p"
        rm -rf "$MERGED_DIR"
        mv "$tmpdir" "$MERGED_DIR"
        echo "Merged process $p."
    fi
done
info "Merge done. Merged .gcda count: $(find "$MERGED_DIR" -name "*.gcda" | wc -l)"

# copy merged gcda files into build tree
info "Copying merged .gcda files into $GCC_BUILD..."
REMAINING=$(find "$GCC_BUILD" -name "*.gcda" | wc -l)
if [ "$REMAINING" -ne 0 ]; then
    die "$REMAINING .gcda files still present in $GCC_BUILD before copy — aborting."
fi
cp -r "$MERGED_DIR/." "$GCC_BUILD/"

# =============================================================================
# STEP 5 — lcov capture
# =============================================================================
info "Step 5: Running lcov..."
mkdir -p "$(dirname "$INFO_FILE")"
lcov --capture \
     --directory "$GCC_BUILD" \
     --output-file "${INFO_FILE}.raw" \
     --gcov-tool "$GCOV_TOOL" \
     --rc lcov_branch_coverage=1 \
     --ignore-errors mismatch,negative,gcov,gcov,empty,unused \
     --rc geninfo_unexecuted_blocks=1 \
	> /dev/null 2>&1

# Filter to gcc/ source only — excludes generated files, libstdc++,
# system headers, libiberty, libcpp, and other subdirs
info "Step 5b: Filtering to gcc/ sources..."
lcov --extract "${INFO_FILE}.raw" \
     "*/gcc-src/*" \
     --rc lcov_branch_coverage=1 \
     --output-file "$INFO_FILE" \
     --ignore-errors mismatch,unused \
	> /dev/null 2>&1
rm -f "${INFO_FILE}.raw"
info "lcov done. Info file: $INFO_FILE"

# =============================================================================
# STEP 6 — genhtml
# =============================================================================
info "Step 6: Generating HTML report..."
mkdir -p "$REPORT_DIR"
genhtml "$INFO_FILE" \
        --output-directory "$REPORT_DIR" \
        --rc lcov_branch_coverage=1 \
        --ignore-errors source,unmapped,empty \
        2>/dev/null || true
info "Done. Report at: $REPORT_DIR/index.html"
