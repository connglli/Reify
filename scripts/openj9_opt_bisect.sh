#!/bin/bash

#
# MIT License
#
# Copyright (c) 2025 Cong Li (cong.li@inf.ethz.ch)
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

OPENJ9_HOME=""
OPT_LEVEL=""
OPT_ST_IND=""
CLASS_FILE=""
CLASSPATH=""

show_usage() {
  echo "Usage: $0 --openj9-home <openj9_home> --opt-level <opt_level> --start-opt-index <start_opt_index> --classpath <classpath> --class-file <class_file>"
  echo "  --openj9-home     : Required. Path to the OpenJ9 JDK home directory."
  echo "  --opt-level       : Required. Optimization level (noOpt,cold,warm,hot,veryHot,scorching)."
  echo "  --start-opt-index : Required. Starting optimization index to test (larger than 0)."
  echo "  --class-file      : Required. Path to the Java class file to run (with .class extension)."
  echo "  --classpath       : Optional. Additional classpath entries (colon-separated)."
}

while [[ $# -gt 0 ]]; do
  case "$1" in
  --openj9-home)
    OPENJ9_HOME="$2"
    shift 2
    ;;
  --opt-level)
    OPT_LEVEL="$2"
    shift 2
    ;;
  --start-opt-index)
    OPT_ST_IND="$2"
    shift 2
    ;;
  --class-file)
    CLASS_FILE="$2"
    shift 2
    ;;
  --classpath)
    CLASSPATH="$2"
    shift 2
    ;;
  *)
    echo "Error: Unknown option $1"
    show_usage
    exit 1
    ;;
  esac
done

if [ -z "$OPENJ9_HOME" ]; then
  echo "Error: OPENJ9_HOME is not set."
  show_usage
  exit 1
elif [ ! -d "$OPENJ9_HOME" ]; then
  echo "Error: Directory $OPENJ9_HOME does not exist."
  exit 1
fi

JAVA=${OPENJ9_HOME}/bin/java
if [ ! -x "$JAVA" ]; then
  echo "Error: Java executable not found in $JAVA."
  exit 1
fi

if [ -z "$OPT_LEVEL" ]; then
  echo "Error: OPT_LEVEL is not set."
  show_usage
  exit 1
elif [[ ! "$OPT_LEVEL" =~ ^(noOpt|cold|warm|hot|veryHot|scorching)$ ]]; then
  echo "Error: OPT_LEVEL must be one of noOpt, cold, warm, hot, veryHot, scorching."
  exit 1
fi

if [ -z "$OPT_ST_IND" ]; then
  echo "Error: OPT_ST_IND is not set."
  show_usage
  exit 1
elif ! [[ "$OPT_ST_IND" =~ ^[0-9]+$ ]]; then
  echo "Error: OPT_ST_IND must be a non-negative integer."
  exit 1
elif [ "$OPT_ST_IND" -le 0 ]; then
  echo "Error: OPT_ST_IND must be larger than 0."
  exit 1
fi

if [ -z "$CLASS_FILE" ]; then
  echo "Error: CLASS_FILE is not set."
  show_usage
  exit 1
elif [ ! -f "$CLASS_FILE" ]; then
  echo "Error: Class file $CLASS_FILE does not exist."
  exit 1
fi

CLASS_DIR=$(dirname "$CLASS_FILE")
CLASS_NAME=$(basename "$CLASS_FILE" .class)

if [ -z "$CLASSPATH" ]; then
  CLASSPATH="$CLASS_DIR"
else
  CLASSPATH="$CLASSPATH:$CLASS_DIR"
fi

tmo_sec=30s
bug_type="unknown"

# Format the Java command with the given optimization index and verbosity option
# Arguments: optimization_index, [verbose_flag]
# Echos: formatted command string
fmt_cmd() {
  local opt_ind=$1
  local ver_opts=""
  if [ -n "$2" ]; then
    ver_opts=",verbose,log=class.log"
  fi
  printf "%s -Xjit:count=0,optLevel=%s,lastOptIndex=%s,traceLastOpt,limit={%s.*}%s -cp %s %s" \
    "$JAVA" \
    "$OPT_LEVEL" \
    "$opt_ind" \
    "$CLASS_NAME" \
    "$ver_opts" \
    "$CLASSPATH" \
    "$CLASS_NAME"
}

# Run the Java program with a specified optimization index
# Arguments: optimization_index
# Returns: exit code of the Java program
run_j9() {
  local j9_cmd
  j9_cmd=$(fmt_cmd "$1")
  echo "> running ($tmo_sec): $j9_cmd"
  timeout --signal KILL "$tmo_sec" bash -c "$j9_cmd"
  return $?
}

# Report the bug with detailed information
# Arguments: optimization_index
# No return value
report_bug() {
  echo "------------------------------------------------------------"
  bugg_ind="$1"
  verbose_command="$(fmt_cmd "$bugg_ind" 1)"
  timeout --signal KILL "$tmo_sec" bash -c "$verbose_command" >/dev/null 2>&1
  echo "Exit code: $?"
  echo "Bug type: $bug_type"
  echo "Buggy index: $bugg_ind"
  buggy_line=$(grep "Ending " class.log.* | tail -n 1)
  buggy_pass=${buggy_line:7}
  echo "Buggy pass: ${buggy_pass:-Unknown}"
  echo "Command: $verbose_command"
  echo "------------------------------------------------------------"
  rm -rf class.log.*
}

# Try reproducing the bug first
run_j9 0
ref_rc=$?
if [ "$ref_rc" -ne 0 ]; then
  report_bug 0
  if [ "$CLASSPATH" -eq "$CLASS_DIR" ]; then
    echo "Note: This might be a false positive due to missing additional classpath."
  fi
  exit 0
fi

run_j9 "$OPT_ST_IND"
ref_rc=$?
if [ "$ref_rc" -eq 0 ]; then
  echo "Error: The bug cannot be reproduced. Try using a larger starting optimization index."
  exit 1
elif [ "$ref_rc" -eq 137 ]; then
  bug_type="program hang"
else
  bug_type="wrong code or internal error"
fi

# Binary search for the buggy optimization index
# Arguments: start_index, end_index
# Requirements: start_index < end_index and start_index cannot be buggy
opt_bisect() {
  local st_ind=$1
  local ed_ind=$2

  echo "> opt-bisect: start=$st_ind, end=$ed_ind"

  if [ "$st_ind" -eq "$ed_ind" ]; then
    echo "Error: Starting index and ending index are the same: $st_ind"
    exit 1
  fi

  if [ "$((st_ind + 1))" -eq "$ed_ind" ]; then
    return "$ed_ind"
  fi

  local mid_ind=$(((st_ind + ed_ind) / 2))

  run_j9 "$mid_ind"
  local curr_rc=$?

  if [ "$curr_rc" -eq 0 ]; then
    opt_bisect $((mid_ind)) "$ed_ind"
    return $?
  elif [ $curr_rc -eq $ref_rc ]; then
    opt_bisect "$st_ind" "$mid_ind"
    return $?
  else
    echo "Error: Unexpected return code $curr_rc at optimization index $mid_ind (start=$st_ind, end=$ed_ind)."
    exit 1
  fi
}

opt_bisect 0 "$OPT_ST_IND"
bug_ind=$?

report_bug "$bug_ind"
