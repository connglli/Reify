#!/bin/bash

# Set a timeout (e.g., 10 seconds)
TIMEOUT=1

# Try to compile with gcc -O1, but kill it if it takes too long
timeout $TIMEOUT /local/home/kchopra/compilers/bin/gcc -O1 check.c -o check
timeout $TIMEOUT ./check >out1.txt 2>&1
return_code_o1=$?
# echo "Return code for gcc -O1: $return_code_o1"
timeout $TIMEOUT /local/home/kchopra/compilers/bin/gcc -O0 check.c -o check
timeout $TIMEOUT ./check > outgcc.txt 2>&1
return_code_o0=$?
# echo "Return code for gcc -O0: $return_code_o0"
timeout $TIMEOUT /local/home/kchopra/llvm-project/build/bin/clang -O0 check.c -o check
timeout $TIMEOUT ./check > outclang.txt 2>&1
return_code_clang=$?
# echo "Return code for clang -O0: $return_code_clang"
##if return_code_o1 == 124 and return_code_o0 != 124 and diff -q outgcc.txt outclang.txt was empty, exit with 0, otherwise exit with 1
if [ $return_code_o1 -eq 124 ] && [ $return_code_o0 -ne 124 ] && diff -q outgcc.txt outclang.txt >/dev/null; then
    # echo "GCC compilation timed out, Clang did not, and outputs are the same."
    exit 0  # Interesting! GCC hung.
else   
    # echo "GCC compilation did not time out or outputs differ."
    exit 1  # Not interesting.
fi