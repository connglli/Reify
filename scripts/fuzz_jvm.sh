#!/bin/bash

#
# MIT License
#
# Copyright (c) 2025.
#
# Kavya Chopra (chopra.kavya04@gmail.com)
# Cong Li (cong.li@inf.ethz.ch)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

RY_HOME=$(dirname "$0")/..

FUZZ_PROC=$1 # Number of parallel fuzzing processes
FUZZ_ITER=$2 # Number of fuzzing iterations
FUZZ_DIR=$3  # Output directory for fuzzing results
JAVA_HOME=$4 # Path to the JDK installation directory

# Simple logging helpers
_ts() { date '+%Y-%m-%d::%H:%M:%S'; }
log() { printf '[%s][INFO] %s\n' "$(_ts)" "$*"; }
logw() { printf '\033[93m[%s][WARN] %s\033[0m\n' "$(_ts)" "$*"; }
loge() { printf '\033[91m[%s][ERRO] %s\033[0m\n' "$(_ts)" "$*"; }

# Validate inputs early
if [[ -z "$FUZZ_PROC" || -z "$FUZZ_ITER" || -z "$FUZZ_DIR" || -z "$JAVA_HOME" ]]; then
  loge "Usage: $0 <num_proc> <fuzz_iter> <fuzz_dir> <JAVA_HOME>"
  exit 1
fi
if ! command -v uuidgen >/dev/null 2>&1; then
  loge "uuidgen not found on PATH; please install uuid-runtime"
  exit 1
fi

# Check if FUZZ_DIR exists. If it exists, ask the user to confirm overwriting.
if [[ -d "$FUZZ_DIR" ]]; then
  logw "Output directory $FUZZ_DIR already exists. Overwrite? (Y/n): "
  read -rp "" ans
  if [[ "$ans" != "Y" ]]; then
    log "Aborting. Please specify a different output directory."
    exit 0
  fi
  logw "Overwriting existing directory $FUZZ_DIR"
  rm -rf "$FUZZ_DIR"
fi

JAVA=${JAVA_HOME}/bin/java
if [[ -z "$JAVA" || ! -x "$JAVA" ]]; then
  loge "Java binary not found or not executable! Ensure a valid JAVA_HOME."
  exit 1
fi

RY_FG=$RY_HOME/build/bin/fgen
if [[ ! -f "$RY_FG" ]]; then
  loge "fgen binary not found! Please build the project first."
  exit 1
fi
RY_RES_DIR=$RY_HOME/res

# Exit code used by GNU timeout when a command times out
TIMEOUT_RC=124

# Run the fuzzing loop per process
fuzz_worker() {
  worker_id=$1
  fuzz_dir=$2

  stop_evt=0
  on_signal() {
    local sig="$1"
    logw "[@$worker_id] Received SIGNAL $sig; stopping fuzzing after current step."
    stop_evt=1
  }
  trap 'on_signal SIGINT' INT
  trap 'on_signal SIGTERM' TERM

  log "[@$worker_id] Fuzzing options: maxiter=$FUZZ_ITER, outdir=$fuzz_dir, java=$JAVA, fgen=$RY_FG"

  mkdir -p "$fuzz_dir"/sexpressions \
    "$fuzz_dir"/functions \
    "$fuzz_dir"/mappings \
    "$fuzz_dir"/programs \
    "$fuzz_dir"/loggings \
    "$fuzz_dir"/javaclasses
  # Bug classification buckets
  mkdir -p "$fuzz_dir"/reify_bugs "$fuzz_dir"/wrong_codes "$fuzz_dir"/intern_errors "$fuzz_dir"/prog_hangs
  log "[@$worker_id] Prepared output directories under $fuzz_dir"

  sno=0
  while [[ "$sno" -lt "$FUZZ_ITER" && $stop_evt -eq 0 ]]; do
    # Allow early exit if stop was requested before iteration starts
    [[ $stop_evt -ne 0 ]] && break

    uuid=$(uuidgen)
    uid=${uuid//-/_}
    base_id="${uid}_${sno}"
    clazz="Class_${base_id}"

    log "[@$worker_id] [$sno] Generating Java program: base_id=$base_id"

    # Generate a Java class/program
    fg_cmd=("$RY_FG" -J -m -S -A -U -v --Xenable-lvn-gvn -o "$fuzz_dir" -s "$RANDOM" -n "$sno" "$uid")
    log "[@$worker_id] [$sno] Run: ${fg_cmd[*]} (timeout=3s)"
    if ! timeout 3s "${fg_cmd[@]}" >/dev/null 2>&1; then
      logw "[@$worker_id] [$sno] Generation failed or timed out; skip"
      continue # Generation fails or times out, skip to next iteration
    fi

    jvm_log_def="$fuzz_dir/loggings/function_${base_id}.def.log"
    jvm_log_jit="$fuzz_dir/loggings/function_${base_id}.jit.log"
    jvm_log_int="$fuzz_dir/loggings/function_${base_id}.int.log"

    rc_def=0
    rc_jit=0
    rc_int=0

    def_cmd=("${JAVA}" -cp "$fuzz_dir/javaclasses:$RY_RES_DIR" "$clazz")
    jit_cmd=("${JAVA}" -Xcomp -cp "$fuzz_dir/javaclasses:$RY_RES_DIR" "$clazz")
    int_cmd=("${JAVA}" -Xint -cp "$fuzz_dir/javaclasses:$RY_RES_DIR" "$clazz")

    log "[@$worker_id] [$sno] Run default VM     -> $jvm_log_def (timeout=60s)"
    timeout 60s "${def_cmd[@]}" >"$jvm_log_def" 2>&1 || rc_def=$?
    [[ $rc_def -eq $TIMEOUT_RC ]] && logw "[$sno] Default VM timed out after 60s"

    log "[@$worker_id] [$sno] Run -Xcomp (JIT)   -> $jvm_log_jit (timeout=60s)"
    # Run with JIT compilers enabled (force compilation where possible)
    timeout 60s "${jit_cmd[@]}" >"$jvm_log_jit" 2>&1 || rc_jit=$?
    [[ $rc_jit -eq $TIMEOUT_RC ]] && logw "[$sno] -Xcomp (JIT) timed out after 60s"

    log "[@$worker_id] [$sno] Run -Xint (interp) -> $jvm_log_int (timeout=60s)"
    # Run with JIT compilers disabled (interpreter only)
    timeout 60s "${int_cmd[@]}" >"$jvm_log_int" 2>&1 || rc_int=$?
    [[ $rc_int -eq $TIMEOUT_RC ]] && logw "[$sno] -Xint (interp) timed out after 60s"

    log "[@$worker_id] [$sno] Exit codes: def=$rc_def, jit=$rc_jit, int=$rc_int"

    # Success: all modes executed without timeout or error
    if [[ ${rc_def} -eq 0 && $rc_jit -eq 0 && $rc_int -eq 0 ]]; then
      log "[@$worker_id] [$sno] All VM modes succeeded; cleaning generated artifacts"
      # Both modes succeeded: clean up all generated artifacts for this seed
      rm -f "$fuzz_dir"/sexpressions/function_${base_id}.sexp 2>/dev/null
      rm -f "$fuzz_dir"/functions/function_${base_id}.c 2>/dev/null
      rm -f "$fuzz_dir"/mappings/function_${base_id}.jsonl 2>/dev/null
      rm -f "$fuzz_dir"/programs/${base_id}.c 2>/dev/null
      rm -f "$fuzz_dir"/loggings/function_${base_id}.log 2>/dev/null
      rm -f "$fuzz_dir"/loggings/function_${base_id}.*.log 2>/dev/null
      rm -f "$fuzz_dir"/javaclasses/"${clazz}".class 2>/dev/null
    else
      # At least one mode failed: classify and archive artifacts
      fail_log="$jvm_log_def"
      [[ $rc_int -ne 0 ]] && fail_log="$jvm_log_int"
      [[ $rc_jit -ne 0 ]] && fail_log="$jvm_log_jit"

      dest_bucket=""
      if [[ $rc_def -eq $TIMEOUT_RC || $rc_jit -eq $TIMEOUT_RC || $rc_int -eq $TIMEOUT_RC ]]; then
        dest_bucket="prog_hangs"
      elif grep -qiE 'VerifyError|verification error' "$fail_log"; then
        dest_bucket="reify_bugs"
      elif grep -q 'ChecksumNotEqualsException' "$fail_log"; then
        dest_bucket="wrong_codes"
      else
        dest_bucket="intern_errors"
      fi

      dest_root="$fuzz_dir/$dest_bucket/${base_id}"
      logw "[@$worker_id] [$sno] FAILURE detected; bucket=$dest_bucket -> $dest_root (see $fail_log)"
      mkdir -p "$dest_root"/sexpressions \
        "$dest_root"/functions \
        "$dest_root"/mappings \
        "$dest_root"/programs \
        "$dest_root"/loggings \
        "$dest_root"/javaclasses

      # Move or copy all related artifacts
      mv -f "$fuzz_dir"/sexpressions/function_${base_id}.sexp "$dest_root"/sexpressions/ 2>/dev/null
      mv -f "$fuzz_dir"/functions/function_${base_id}.c "$dest_root"/functions/ 2>/dev/null
      mv -f "$fuzz_dir"/mappings/function_${base_id}.jsonl "$dest_root"/mappings/ 2>/dev/null
      mv -f "$fuzz_dir"/programs/${base_id}.c "$dest_root"/programs/ 2>/dev/null
      mv -f "$fuzz_dir"/loggings/function_${base_id}.log "$dest_root"/loggings/ 2>/dev/null
      mv -f "$fuzz_dir"/loggings/function_${base_id}.*.log "$dest_root"/loggings/ 2>/dev/null
      mv -f "$fuzz_dir"/javaclasses/"${clazz}".class "$dest_root"/javaclasses/ 2>/dev/null

      # Save reproduction info
      {
        echo "${fg_cmd[@]}"
        echo "${def_cmd[@]}"
        echo "${int_cmd[@]}"
        echo "${jit_cmd[@]}"
      } >"$dest_root/command.txt"
      log "[@$worker_id] [$sno] Reproducer saved to $dest_root/command.txt"
    fi

    # After finishing this iteration, check for stop request before next
    sno=$((sno + 1))
  done

  if [[ $stop_evt -ne 0 ]]; then
    logw "[@$worker_id] Fuzzing stopped by signal at iteration $sno"
  else
    log "[@$worker_id] Finished JVM fuzzing: iter=$sno/$FUZZ_ITER, outdir=$fuzz_dir"
  fi
}

on_signal() {
  local sig="$1"
  logw "Received SIGNAL $sig; stopping fuzzing"
}
trap 'on_signal SIGINT' INT
trap 'on_signal SIGTERM' TERM

source "$RY_HOME/scripts/job_pool.sh"

job_pool_init "$FUZZ_PROC" 0

for ((i = 0; i < FUZZ_PROC; i++)); do
  log "Starting fuzzing worker $i ..."
  worker_id="Worker$i"
  fuzz_dir="$FUZZ_DIR/fuzz_proc_$(printf '%05d' "$i")"
  mkdir -p "$fuzz_dir"
  job_pool_run fuzz_worker "$worker_id" "$fuzz_dir"
done

job_pool_shutdown

log "All fuzzing processes completed. Results are in $FUZZ_DIR"
