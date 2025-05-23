#!/bin/bash
GCC_PATH=gcc
CLANG_PATH=clang
for f in *.c ; do
  echo $f
#  echo "O3"
  timeout -s 9 10 $GCC_PATH -O3 $f #>& /dev/null
  timeout -s 9 3 ./a.out >& out1.txt #>& /dev/null
#  echo
  timeout -s 9 10 $CLANG_PATH -O3 $f #>& /dev/null
  timeout -s 9 3 ./a.out >& out2.txt #>& /dev/null
#  echo
  if ! diff -q out1.txt out2.txt ; then
    echo "BUG"
  fi
  timeout -s 9 10 $GCC_PATH -O3 $f #>& /dev/null
  timeout -s 9 3 ./a.out >& out1.txt #>& /dev/null
#  echo
  timeout -s 9 10 $GCC_PATH -O0 $f #>& /dev/null
  timeout -s 9 3 ./a.out >& out2.txt #>& /dev/null
#  echo
  if ! diff -q out1.txt out2.txt ; then
    echo "BUG"
  fi
  timeout -s 9 10 $CLANG_PATH -O3 $f #>& /dev/null
  timeout -s 9 3 ./a.out >& out1.txt #>& /dev/null
#  echo
  timeout -s 9 10 $CLANG_PATH -O0 $f #>& /dev/null
  timeout -s 9 3 ./a.out >& out2.txt #>& /dev/null
#  echo
  if ! diff -q out1.txt out2.txt ; then
    echo "BUG"
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
    echo "BUG"
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
    echo "BUG"
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
    echo "BUG"
  fi
#  echo
#  echo
#  echo
done