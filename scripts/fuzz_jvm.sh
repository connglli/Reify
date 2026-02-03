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

RY_HOME=$(cd "$(dirname "$0")"/.. && pwd || exit)

#-=========================================
# Logging functions
#-=========================================

_ts() { date '+%Y-%m-%d::%H:%M:%S'; }

mylog() {
  local color=""
  case $1 in
  green) color="32"; shift ;;
  red) color="31"; shift ;;
  yellow) color="33"; shift ;;
  *) color="" ;;
  esac
  if [[ -n "$color" ]]; then
    printf '\033[%sm[%s][INFO] %s\033[0m\n' "$color" "$(_ts)" "$*"
  else
    printf '[%s][INFO] %s\n' "$(_ts)" "$*"
  fi
}

mylogw() { mylog "yellow" "$*"; }

myloge() { mylog "red" "$*"; }

#-=========================================
# Logging functions
#-=========================================

OPT_FUZZ_PROC=$(nproc)   # Number of parallel fuzzing processes
OPT_FUZZ_ITER="inf"      # Number of fuzzing iterations
OPT_FUZZ_DIR="fuzzout"   # Output directory for fuzzing results
JAVA_HOME=""             # Path to the JDK installation directory

#-=========================================
# Options parsing
#-=========================================

check_deps() {
  if ! command -v uuidgen >/dev/null 2>&1; then
    myloge "Error: uuidgen not found on PATH; please install uuid-runtime"
    exit 1
  fi
}

show_usage() {
  echo "Usage: $0 --nproc <num_proc> --niter <fuzz_iter> --output <work_dir> --java-home <JAVA_HOME>"
  echo ""
  echo "Options:"
  echo "  --nproc       Number of parallel fuzzing processes (default: ${OPT_FUZZ_PROC})"
  echo "  --niter       Number of fuzzing iterations per process (default: ${OPT_FUZZ_ITER})"
  echo "  --output      Output directory for fuzzing results (default: ${OPT_FUZZ_DIR})"
  echo "  --java-home   Path to the JDK installation directory (required)"
  echo "  --help        Show this help message"
}

check_deps

while [[ $# -gt 0 ]]; do
  case $1 in
  --help)
    show_usage
    exit 0
    ;;
  --nproc)
    OPT_FUZZ_PROC=$2
    shift 2
    ;;
  --niter)
    OPT_FUZZ_ITER=$2
    shift 2
    ;;
  --output)
    OPT_FUZZ_DIR=$2
    shift 2
    ;;
  --java-home)
    JAVA_HOME=$2
    shift 2
    ;;
  *)
    myloge "Unknown argument: $1"
    exit 1
    ;;
  esac
done

if [[ -z "$OPT_FUZZ_PROC" ]]; then
  myloge "Error: Number of parallel fuzzing processes (--nproc) is not specified."
  exit 1
fi

if [[ -z "$OPT_FUZZ_ITER" ]]; then
  myloge "Error: Number of fuzzing iterations (--niter) is not specified."
  exit 1
fi

if [[ -z "$OPT_FUZZ_DIR" ]]; then
  myloge "Error: Output directory for fuzzing results (--output) is not specified."
  exit 1
fi

if [[ -z "$JAVA_HOME" ]]; then
  myloge "Error: Path to the JVM installation directory (--java-home) is not specified."
  exit 1
fi

# Check if OPT_FUZZ_DIR exists. If it exists, ask the user to confirm overwriting.
if [[ -d "$OPT_FUZZ_DIR" ]]; then
  mylogw "Output directory $OPT_FUZZ_DIR already exists. Overwrite? (Y/n): "
  read -rp "" ans
  if [[ "$ans" != "Y" ]]; then
    mylog "Aborting. Please specify a different output directory."
    exit 0
  fi
  mylogw "Overwriting existing directory $OPT_FUZZ_DIR"
  rm -rf "$OPT_FUZZ_DIR"
fi

JAVA=${JAVA_HOME}/bin/java
if [[ -z "$JAVA" || ! -x "$JAVA" ]]; then
  myloge "Error: The 'java' binary not found or not executable! Ensure a valid JAVA_HOME (--java-home)."
  exit 1
fi

RYSMITH=$RY_HOME/build/bin/rysmith
if [[ ! -f "$RYSMITH" || ! -x "$RYSMITH" ]]; then
  myloge "Error: The 'rysmith' binary not found! Please build the project first."
  exit 1
fi
RY_RES_DIR=$RY_HOME/res

#-=========================================
# Suggested Configurations
#-=========================================

SUGGESTED_CONFIGS=(
  "15 8 2 2 3"
  "15 8 2 2 4"
  "15 8 3 2 4"
  "10 8 2 2 4"
  "8 10 2 2 3"
  "11 8 2 2 4"
  "10 12 2 2 4"
  "15 10 2 2 2"
  "10 8 3 2 4"
  "15 8 4 2 3"
)

#-=========================================
# Fuzz workers
#-=========================================

# Exit code used by GNU timeout when a command times out
TIMEOUT_RC=137
TIMEOUT_SG=KILL

# Run the fuzzing loop per process
fuzz_worker() {
  local worker_id=$1
  local work_dir=$2

  local stop_evt=0
  # shellcheck disable=SC2317
  on_signal() {
    local sig="$1"
    mylogw "[$worker_id] Received SIGNAL $sig; stopping fuzzing after current step."
    stop_evt=1
  }
  trap 'on_signal SIGINT' INT
  trap 'on_signal SIGTERM' TERM

  mylog "[$worker_id] Fuzzing options: maxiter=$OPT_FUZZ_ITER, outdir=$work_dir, java=$JAVA, rysmith=$RYSMITH"

  # Bug classification buckets
  mkdir -p "$work_dir"/reify_bugs "$work_dir"/wrong_codes "$work_dir"/intern_errors "$work_dir"/compiler_hangs
  mylog "[$worker_id] Prepared output directories under $work_dir"

  local sno=0
  while [[ ("$OPT_FUZZ_ITER" = "inf" || "$sno" -lt "$OPT_FUZZ_ITER") && $stop_evt -eq 0 ]]; do
    # Allow early exit if stop was requested before iteration starts
    [[ $stop_evt -ne 0 ]] && break

    uuid=$(uuidgen)
    uid=${uuid//-/_}
    fid="${uid}_${sno}"
    clazz="Class_${fid}"

    # Path to the test directory generated by rysmith
    test_dir="$work_dir/func_${fid}"

    mylog "[$worker_id] [$sno] Generating Java program with fid $fid ..."

    # Generate a Java class/program
    config_idx=$((RANDOM % ${#SUGGESTED_CONFIGS[@]}))
    read -r conf_bbls conf_vars conf_assn conf_tera conf_terc <<<"${SUGGESTED_CONFIGS[$config_idx]}"
    config_opts=(
      --Xnum-bbls-per-fun "$conf_bbls"
      --Xnum-vars-per-fun "$conf_vars"
      --Xnum-assigns-per-bbl "$conf_assn"
      --Xnum-vars-per-assign "$conf_tera"
      --Xnum-vars-in-cond "$conf_terc"
    )
    fg_cmd=(
      "$RYSMITH" -J -m -S -A -U
      --Xenable-lvn-gvn
      "${config_opts[@]}"
      -o "$work_dir"
      -s "$RANDOM"
      -n "$sno"
      "$uid"
    )
    mylog "[$worker_id] [$sno] Run: ${fg_cmd[*]} (timeout=3s)"
    if ! timeout --signal "$TIMEOUT_SG" 3s "${fg_cmd[@]}" >/dev/null 2>&1; then
      mylogw "[$worker_id] [$sno] Generation failed or timed out; skip"
      rm -rf "$test_dir" # Cleanup potential partial directory
      continue # Generation fails or times out, skip to next iteration
    fi

    # Verify if directory exists
    if [[ ! -d "$test_dir" ]]; then
       mylogw "[$worker_id] [$sno] Test directory not found: $test_dir"
       continue
    fi

    jvm_log_mix="$test_dir/jvm.mix.log"
    jvm_log_jit="$test_dir/jvm.jit.log"
    jvm_log_int="$test_dir/jvm.int.log"

    rc_mix=0
    rc_jit=0
    rc_int=0

    # Classpath includes the test directory (containing Class.class) and resources
    mix_cmd=("${JAVA}" -cp "$test_dir:$RY_RES_DIR" "$clazz")
    jit_cmd=("${JAVA}" -Xcomp -cp "$test_dir:$RY_RES_DIR" "$clazz")
    int_cmd=("${JAVA}" -Xint -cp "$test_dir:$RY_RES_DIR" "$clazz")

    mylog "[$worker_id] [$sno] Run -Xmixed (default) -> $jvm_log_mix (timeout=60s)"
    timeout --signal "$TIMEOUT_SG" 60s "${mix_cmd[@]}" >"$jvm_log_mix" 2>&1 || rc_mix=$?
    [[ $rc_mix -eq $TIMEOUT_RC ]] && logw "[$sno] -Xmixed (default) timed out after 60s"

    mylog "[$worker_id] [$sno] Run -Xcomp (JIT)      -> $jvm_log_jit (timeout=60s)"
    # Run with JIT compilers enabled (force compilation where possible)
    timeout --signal "$TIMEOUT_SG" 60s "${jit_cmd[@]}" >"$jvm_log_jit" 2>&1 || rc_jit=$?
    [[ $rc_jit -eq $TIMEOUT_RC ]] && logw "[$sno] -Xcomp (JIT) timed out after 60s"

    mylog "[$worker_id] [$sno] Run -Xint (interp)    -> $jvm_log_int (timeout=60s)"
    # Run with JIT compilers disabled (interpreter only)
    timeout --signal "$TIMEOUT_SG" 60s "${int_cmd[@]}" >"$jvm_log_int" 2>&1 || rc_int=$?
    [[ $rc_int -eq $TIMEOUT_RC ]] && logw "[$sno] -Xint (interp) timed out after 60s"

    mylog "[$worker_id] [$sno] Exit codes: mix=$rc_mix, jit=$rc_jit, int=$rc_int"

    # Success: all modes executed without timeout or error
    if [[ ${rc_mix} -eq 0 && $rc_jit -eq 0 && $rc_int -eq 0 ]]; then
      mylog "[$worker_id] [$sno] All VM modes succeeded; cleaning generated artifacts"
      rm -rf "$test_dir"
    else
      # At least one mode failed: classify and archive artifacts
      mylog "[$worker_id] [$sno] Detected VM failure; classifying ..."
      err_log="$jvm_log_mix"
      [[ $rc_int -ne 0 ]] && err_log="$jvm_log_int"
      [[ $rc_jit -ne 0 ]] && err_log="$jvm_log_jit"

      err_type=""
      if [[ $rc_mix -eq $TIMEOUT_RC || $rc_jit -eq $TIMEOUT_RC || $rc_int -eq $TIMEOUT_RC ]]; then
        err_type="compiler_hangs" # The JVM fails to terminate within the timeout
      elif grep -qiE 'VerifyError|verification error' "$err_log"; then
        err_type="reify_bugs" # It should be a bug of our tool
      elif grep -q 'ChecksumNotEqualsException' "$err_log"; then
        err_type="wrong_codes" # The JVM computes a wrong checksum value
      else
        err_type="intern_errors" # Other exceptions or JVM crashes
      fi

      err_dir="$work_dir/$err_type/${fid}"
      mylog "green" "[$worker_id] [$sno] FAILURE detected: type=$err_type -> $err_dir (see $err_log)"

      # Move the whole test directory to the bug category directory
      mv "$test_dir" "$err_dir"

      # Save reproduction info
      {
        echo "${fg_cmd[@]}"
        echo "${mix_cmd[@]}"
        echo "${int_cmd[@]}"
        echo "${jit_cmd[@]}"
      } >"$err_dir/commands.txt"
      mylog "[$worker_id] [$sno] Reproducer saved to $err_dir/commands.txt"
    fi

    sno=$((sno + 1))
  done

  if [[ $stop_evt -ne 0 ]]; then
    mylogw "[$worker_id] Fuzzing stopped by signal at iteration $sno"
  else
    mylog "green" "[$worker_id] Finished JVM fuzzing: iter=$sno/$OPT_FUZZ_ITER, outdir=$work_dir"
  fi
}

#-=========================================
# Worker management
#-=========================================

worker_pids=() # Array to store worker PIDs

on_signal() {
  local sig="$1"
  mylogw "[@MAIN] Received SIGNAL $sig; stopping fuzzing"
  for pid in "${worker_pids[@]}"; do
    mylogw "[@MAIN] Sending $sig to worker with PID $pid"
    kill -"$sig" "$pid" 2>/dev/null
  done
  mylog "[@MAIN] Waiting for all workers to terminate..."
  wait
}

trap 'on_signal SIGINT' INT
trap 'on_signal SIGTERM' TERM

for ((i = 0; i < OPT_FUZZ_PROC; i++)); do
  mylog "[@MAIN] Starting fuzzing worker with ID $i ..."
  worker_id="@WORKER$i"
  work_dir="$OPT_FUZZ_DIR/fuzz_proc_$(printf '%05d' "$i")"
  mkdir -p "$work_dir"
  fuzz_worker "$worker_id" "$work_dir" &
  worker_pid="$!"
  mylog "green" "[@MAIN] Worker with ID $i started: PID is $worker_pid"
  worker_pids+=("$worker_pid") # Save the PID of the background process
done

# Wait for all workers to finish
mylog "[@MAIN] All workers started. Waiting for them to finish ..."
wait

mylog "[@MAIN] All fuzzing processes completed. Results are in $OPT_FUZZ_DIR."
