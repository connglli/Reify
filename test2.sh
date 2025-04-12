#!/bin/bash
GCC_PATH=/local/home/kchopra/.gcclocal/bin/gcc
CLANG_PATH=/local/home/kchopra/llvm-project/build/bin/clang
for f in generated_code_*.c ; do
  i=`echo $f | tr -d -c 0-9`
  FILE=generated_code_$i.c
  DEFS=../definitions/variable_definitions_$i.c
  echo $FILE
#  echo "O3"
  timeout -s 9 10 $GCC_PATH -O3 $FILE $DEFS #>& /dev/null
  timeout -s 9 3 ./a.out >& out1.txt #>& /dev/null
#  echo
  timeout -s 9 10 $CLANG_PATH -O3 $FILE $DEFS #>& /dev/null
  timeout -s 9 3 ./a.out >& out2.txt #>& /dev/null
#  echo
  if ! diff -q out1.txt out2.txt ; then
    echo "BUG"
  fi
  timeout -s 9 10 $CLANG_PATH -O0 $FILE $DEFS #>& /dev/null
  timeout -s 9 3 ./a.out >& out1.txt #>& /dev/null
#  echo
  timeout -s 9 10 $CLANG_PATH -O3 $FILE $DEFS #>& /dev/null
  timeout -s 9 3 ./a.out >& out2.txt #>& /dev/null
#  echo
  if ! diff -q out1.txt out2.txt ; then
    echo "BUG"
  fi
  timeout -s 9 10 $GCC_PATH -O0 $FILE $DEFS #>& /dev/null
  timeout -s 9 3 ./a.out >& out1.txt #>& /dev/null
#  echo
  timeout -s 9 10 $GCC_PATH -O3 $FILE $DEFS #>& /dev/null
  timeout -s 9 3 ./a.out >& out2.txt #>& /dev/null
#  echo
  if ! diff -q out1.txt out2.txt ; then
    echo "BUG"
  fi
#  echo
#  echo "O2"
  timeout -s 9 10 $GCC_PATH -O2 $FILE $DEFS #>& /dev/null
  timeout -s 9 3 ./a.out >& out1.txt #>& /dev/null
#  echo
  timeout -s 9 10 $CLANG_PATH -O2 $FILE $DEFS #>& /dev/null
  timeout -s 9 3 ./a.out >& out2.txt #>& /dev/null
#  echo
  if ! diff -q out1.txt out2.txt ; then
    echo "BUG"
  fi
#  echo
#  echo "Os"
  timeout -s 9 10 $GCC_PATH -Os $FILE $DEFS #>& /dev/null
  timeout -s 9 3 ./a.out >& out1.txt #>& /dev/null
#  echo
  timeout -s 9 10 $CLANG_PATH -Os $FILE $DEFS #>& /dev/null
  timeout -s 9 3 ./a.out >& out2.txt #>& /dev/null
#  echo
  if ! diff -q out1.txt out2.txt ; then
    echo "BUG"
  fi
#  echo
#  echo "O1"
  timeout -s 9 10 $GCC_PATH -O1 $FILE $DEFS #>& /dev/null
  timeout -s 9 3 ./a.out >& out1.txt #>& /dev/null
#  echo
  timeout -s 9 10 $CLANG_PATH -O1 $FILE $DEFS #>& /dev/null
  timeout -s 9 3 ./a.out >& out2.txt #>& /dev/null
#  echo
  if ! diff -q out1.txt out2.txt ; then
    echo "BUG"
  fi
#  echo
#  echo
#  echo
done