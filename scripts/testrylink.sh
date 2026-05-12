#!/bin/bash

set -e

dir=$1

#make clean
#make -j 8

CC="clang"
CC_FLAGS="-O0 -fsanitize=address,undefined"
j=8

generate() {
	uuid=$1
	seed=$2
	echo "generating.. " $uuid
	./build/bin/rylink --verbose --debug -i $dir -l 1 $uuid -s $seed > /dev/null;
	retVal=$?
	if [ $retVal -ne 0 ]; then
		echo "found failing generation for seed" $seed
	fi
}

compile_and_run() {
	path=$1
	echo "compiling.. " $path;
	$CC $CC_FLAGS $path/*.c -o $path/main.out;
	echo "running.. " $path;
	./$path/main.out;
	retVal=$?
	if [ $retVal -ne 0 ]; then
		echo "found failing testcase at" $path
	fi
	exit 0
}

# Iterates over seeds for easy reproduction
generateJ=0
for i in {0..1024}; do
	uuid="id$i"
	generate $uuid $i &
	generateJ=$((1 + $generateJ))
	if [ $generateJ -gt $j ]; then
		wait
		generateJ=0
	fi	
done

wait

compileJ=0
for d in $dir/prog_*/; do
	compile_and_run $d &
	compileJ=$((1 + $compileJ))
	if [ $compileJ -gt $j ]; then
		wait
		compileJ=0
	fi	
done

wait
