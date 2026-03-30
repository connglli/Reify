#!/bin/bash

dir=$1

# small 1 thread fuzzing that terminates on any error (other than hangs) useful for debugging rylink.
# Iterates over seeds for easy reproduce
for i in {0..1024}; do
	echo "seed: " $i;
	uuid=$(uuidgen)
	echo "generating.. " $uuid
	./build/bin/rylink --verbose --debug -i $dir -l 1 $uuid -s $i;
	echo "compiling.. " $uuid;
	path="$dir/prog_${uuid//"-"/"_"}_0";
	echo $path;
	gcc -O3 $path/*.c -o $path/main.out;
	echo "running.. " $uuid;
	./$path/main.out;
	retVal=$?
	if [ $retVal -ne 0 ]; then
		exit $retVal
	fi
done
