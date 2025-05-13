#!/bin/bash
BADCC1=()
BADCC3=("gcc -O1" "gcc -O2" "gcc -Os")
BADCC2=()
MODE=("-m64")
#!/usr/bin/env bash

# need to configure this part
#BADCC1=("clang-trunk -O3") # compilation failures
#BADCC2=() # exec failures
#BADCC3=() # wrong results
#MODE=-m64

readonly GOODCC=("gcc -O0")
readonly TIMEOUTCC=3
readonly TIMEOUTEXE=1
readonly TIMEOUTEXEBAD=1
readonly TIMEOUTCCOMP=10
# flag to control whether to use CompCert to validate the test program.
readonly USE_COMPCERT=false
readonly USE_CLANG_UBSAN=true
readonly USE_CLANG_MSAN=false

readonly CFILE=static_bug.c
readonly CSMITH_INC="-I/local/suz-local/software/local/include"
readonly CFLAG="-o t"
readonly CLANGFC="clang -w -m64 -O0 -fwrapv -ftrapv -fsanitize=undefined,address"
readonly CLANG_MEM_SANITIZER="clang -w -O0 -m64 -fsanitize=memory"

#################################################################################

### check for undefined behaviors first (from creduce scripts)

rm -f out*.txt

if true; then
if
/local/home/kchopra/llvm-project/build/bin/clang . -Wfatal-errors -pedantic -Wall -Wsystem-headers -O0 -c $CFILE >out.txt 2>&1 &&\
! grep 'conversions than data arguments' out.txt &&\
! grep 'incompatible redeclaration' out.txt &&\
! grep 'ordered comparison between pointer' out.txt &&\
! grep 'eliding middle term' out.txt &&\
#! grep 'end of non-void function' out.txt &&\
! grep 'invalid in C99' out.txt &&\
! grep 'specifies type' out.txt &&\
! grep 'should return a value' out.txt &&\
#! grep 'uninitialized' out.txt &&\
! grep 'incompatible pointer to' out.txt &&\
! grep 'incompatible integer to' out.txt &&\
! grep 'type specifier missing' out.txt &&\
/local/home/kchopra/compilers/bin/gcc . -Wfatal-errors -Wall -Wextra -Wsystem-headers -O0 $CFILE >outa.txt 2>&1 &&\
# ! grep uninitialized outa.txt &&\
! grep 'Wimplicit-int' outa.txt &&\
! grep 'division by zero' outa.txt &&\
! grep 'without a cast' outa.txt &&\
#! grep 'control reaches end' outa.txt &&\
! grep 'return type defaults' outa.txt &&\
! grep 'cast from pointer to integer' outa.txt &&\
! grep 'useless type name in empty declaration' outa.txt &&\
! grep 'no semicolon at end' outa.txt &&\
! grep 'type defaults to' outa.txt &&\
! grep 'too few arguments for format' outa.txt &&\
#! grep 'incompatible pointer' outa.txt &&\
! grep 'ordered comparison of pointer with integer' outa.txt &&\
! grep 'declaration does not declare anything' outa.txt &&\
! grep 'expects type' outa.txt &&\
! grep 'pointer from integer' outa.txt &&\
# ! grep 'incompatible implicit' outa.txt &&\
# ! grep 'excess elements in struct initializer' outa.txt &&\
# ! grep 'return type of \'main\' is not \'int\'' outa.txt &&\
! grep 'comparison between pointer and integer' outa.txt #&&\
# frama-c -val-signed-overflow-alarms -val -stop-at-first-alarm -no-val-show-progress -machdep x86_64 -obviously-terminates -precise-unions $CFILE >out_framac.txt 2>&1 &&\
! egrep -i '(user error|assert)' out_framac.txt >/dev/null 2>&1
then
 : # do nothing
else
 echo "clang or gcc found undefined behavior"
 exit 1
fi
fi

###################################################
# compcert first
###################################################
echo "compcert first"
if $USE_COMPCERT ; then
 timeout -s 9 $TIMEOUTCCOMP ccomp -interp -fall . $CFILE >& /dev/null
 ret=$?
 if [ $ret != 0 ] ; then
  exit 1
 fi
fi

###################################################
# clang memory sanitizer
###################################################
echo "clang memory sanitizer"
if ${USE_CLANG_MSAN} ; then
 readonly TEMP_EXE="temp.exe"
 timeout -s 9 $TIMEOUTCC $CLANG_MEM_SANITIZER $CFILE -o $TEMP_EXE > /dev/null
 if [[ $? != 0 ]] ; then
  echo "clang memory sanitizer compilation failed"
  exit 1
 fi
  
 readonly MEM_SANITIZER_OUTPUT="mem-sanitizer.output"
 (timeout -s 9 $TIMEOUTEXE ./$TEMP_EXE &> $MEM_SANITIZER_OUTPUT ) &> /dev/null
 if [[ $? != 0 ]] ; then
    echo "clang memory sanitizer execution failed"
  exit 1
 fi
  
 if grep -q "MemorySanitizer" $MEM_SANITIZER_OUTPUT ; then
    echo "clang memory sanitizer found an error"
  exit 1
 fi
  
 rm $MEM_SANITIZER_OUTPUT $TEMP_EXE
fi


###################################################
# @clangfc @ -O0 to check for undefined behavior
###################################################

echo "clang -O0 to check for undefined behavior"
if ${USE_CLANG_UBSAN} ; then
 rm -f ./t ./out*.txt
 timeout -s 9 $TIMEOUTCC $CLANGFC $CFLAG $CFILE > /dev/null
 ret=$?
  
 if [ $ret != 0 ] ; then
 echo "clang compilation failed"
  exit 1
 fi
  
 (timeout -s 9 $TIMEOUTEXE ./t >out0.txt 2>&1) >&/dev/null
 ret=$?
  
 if [ $ret != 0 ] ; then
    echo "clang execution failed"
  exit 1
 fi
  
 if grep -q "runtime error" out0.txt ; then
  exit 1
 fi
fi

#############################
# iterate over the good ones
#############################


#for cc in "${GOODCC[@]}" ; do
#for ((i=0; i < ${#GOODCC[@]} ; ++i )) ; do
echo "good ones"
for ((i=0; i < ${#GOODCC[@]} ; ++i )) ; do
 cc=${GOODCC[$i]}
 rm -f ./t ./out1.txt
  
 timeout -s 9 $TIMEOUTCC $cc $CFLAG $CFILE >& /dev/null
 ret=$?
 
 if [ $ret != 0 ] ; then
    echo "compilation failed with $cc"
  exit 1
 fi
  
 # execute
 (timeout -s 9 $TIMEOUTEXE ./t >out1.txt 2>&1) >&/dev/null
 ret=$?
 if [ $ret != 0 ] ; then
    echo "execution failed with $cc"
    echo "execution failed"
  exit 1
 fi
  
 if [[ "$i" == 0 ]] ; then
  mv out1.txt out0.txt
  continue
 fi
  
 # compare with reference: out0.txt
 if ! diff -q out0.txt out1.txt >/dev/null ; then
    echo "output mismatch"
  exit 1
 fi
done

#############################
# iterate over the bad ones
#############################

echo "bad ones"
for cc in "${BADCC1[@]}" ; do
 for mode in "${MODE[@]}" ; do
  rm -f ./t ./out2.txt
   
  # compile
  echo "running $cc $CFLAG $mode $CFILE"
  (timeout -s 9 $TIMEOUTCC $cc $CFLAG $mode $CFILE >out2.txt 2>&1) >& /dev/null
  ret=$?
  echo "ret = $ret"
  if [ $ret -ne 137 ] ; then
    exit 1

  fi
  # if ! grep -q 'internal compiler error' out2.txt && \
  # ! grep -q 'internal error:' out2.txt && \
  # ! grep -q 'PLEASE ATTACH THE FOLLOWING FILES TO THE BUG REPORT' out2.txt && \
  # ! grep -q 'clang: error: linker command failed with exit code 1 (use -v to see invocation)' out2.txt
  # then
  #  exit 1
  # fi
 done
done

for cc in "${BADCC2[@]}" ; do
 for mode in "${MODE[@]}" ; do
  rm -f ./t ./out2.txt
   
  # compile
  timeout -s 9 $TIMEOUTCC $cc $CFLAG $mode $CFILE >& /dev/null
  ret=$?
  echo "ret = $ret"
  if [ $ret -ne 0 ] ; then
   exit 1
  fi
   
  # execute
  (timeout -s 9 $TIMEOUTEXE ./t >out2.txt 2>&1) >&/dev/null
  ret=$?
  if [ $ret -ne 137 ] ; then
    echo "execution failed with $cc"
   exit 1
  fi
 done
done

for cc in "${BADCC3[@]}" ; do
 for mode in "${MODE[@]}" ; do
  rm -f ./t ./out2.txt  # compile
  timeout -s 9 $TIMEOUTCC $cc $CFLAG $mode $CFILE >& /dev/null
  ret=$?
  if [ $ret != 0 ] ; then
   exit 1
  fi  # execute
  (timeout -s 9 $TIMEOUTEXE ./t >out2.txt 2>&1) >&/dev/null
  ret=$?
  if [ $ret != 134 ] ; then
    echo "execution failed with $cc"
   exit 1
  fi
 done
done

# now we have passed everything, return 0
exit 0