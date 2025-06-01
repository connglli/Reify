#!/bin/bash
GCC_PATH=/local/home/kchopra/compilers/bin/gcc
CLANG_PATH=/local/home/kchopra/llvm-project/build/bin/clang
f=$1
  echo $f
#  echo "O3"
  timeout -s 9 10 $GCC_PATH -O3 $f #>& /dev/null
  timeout -s 9 3 ./a.out >& out1.txt #>& /dev/null
#  echo
  timeout -s 9 10 $CLANG_PATH -O3 $f #>& /dev/null
  timeout -s 9 3 ./a.out >& out2.txt #>& /dev/null
#  echo
  if ! diff -q out1.txt out2.txt ; then
    echo "BUG at O3"
  fi
  timeout -s 9 10 $GCC_PATH -O3 $f #>& /dev/null
  timeout -s 9 3 ./a.out >& out1.txt #>& /dev/null
#  echo
  timeout -s 9 10 $GCC_PATH -O0 $f #>& /dev/null
  timeout -s 9 3 ./a.out >& out2.txt #>& /dev/null
#  echo
  if ! diff -q out1.txt out2.txt ; then
    echo "BUG at GCC O0 vs O3"
  fi
  timeout -s 9 10 $CLANG_PATH -O3 $f #>& /dev/null
  timeout -s 9 3 ./a.out >& out1.txt #>& /dev/null
#  echo
  timeout -s 9 10 $CLANG_PATH -O0 $f #>& /dev/null
  timeout -s 9 3 ./a.out >& out2.txt #>& /dev/null
#  echo
  if ! diff -q out1.txt out2.txt ; then
    echo "BUG at CLANG O0 vs O3"
  fi
#  echo
#  echo "O2"
  timeout -s 9 10 $GCC_PATH -O2 $f #>& /dev/null
  timeout -s 9 3 ./a.out >& out1.txt #>& /dev/null
#  echo
  timeout -s 9 10 $CLANG_PATH -O2 $f #>& /dev/null
  timeout -s 9 3 ./a.out >& out2.txt #>& /dev/null
#  echo
  if ! diff -q out1.txt out2.txt ; then
    echo "BUG at O2"
  fi
#  echo
#  echo "Os"
  timeout -s 9 10 $GCC_PATH -Os $f #>& /dev/null
  timeout -s 9 3 ./a.out >& out1.txt #>& /dev/null
#  echo
  timeout -s 9 10 $CLANG_PATH -Os $f #>& /dev/null
  timeout -s 9 3 ./a.out >& out2.txt #>& /dev/null
#  echo
  if ! diff -q out1.txt out2.txt ; then
    echo "BUG at Os"
  fi
#  echo
#  echo "O1"
  timeout -s 9 10 $GCC_PATH -O1 $f #>& /dev/null
  timeout -s 9 3 ./a.out >& out1.txt #>& /dev/null
#  echo
  timeout -s 9 10 $CLANG_PATH -O1 $f #>& /dev/null
  timeout -s 9 3 ./a.out >& out2.txt #>& /dev/null
#  echo
  if ! diff -q out1.txt out2.txt ; then
    echo "BUG at O1"
  fi
