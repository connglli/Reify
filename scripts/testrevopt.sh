#!/bin/bash

set -e

dir=$1

# small 1 thread fuzzing that terminates on any error (other than hangs) useful for debugging rylink.
# Iterates over seeds for easy reproduce
for i in {0..1024}; do
	echo "seed: " $i;
	uuid=$(uuidgen)
	echo "generating.. " $uuid
	./build/bin/revopt-fgen -v -m -n 0 -o $dir $uuid -s $i;
	retVal=$?
	echo "compiling.. " $uuid;
	path="$dir/func_${uuid//"-"/"_"}_0";
	echo $path;
	clang -O0 $path/*.c -o $path/main.out;
	echo "running.. " $uuid;
	./$path/main.out;
	retVal=$?
	if [ $retVal -ne 0 ]; then
		echo "Wrong Code generated"
		exit $retVal
	fi
done
