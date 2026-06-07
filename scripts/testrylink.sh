#!/bin/bash

dir=$1
nrTests=16384

#make clean
#make -j 8

CC="clang"
CC_FLAGS="-O0 -fsanitize=address,undefined"
j=8

generate() {
	seed_start=$1
	nr=$2
	for (( i=$seed_start; i<=$(($seed_start + $nr)); i++ )); do
	uuid="id$i"
	#echo "generating.. " $uuid
	./build/bin/rylink --verbose --debug -i $dir -l 1 $uuid -s $i > /dev/null 2> /tmp/rylink_out_$uuid
	retVal=$?
	content=$(<"/tmp/rylink_out_$uuid")
	if [ -n "$content" ]; then 
		echo "Error message of" $uuid ": " $content
	fi
	if [ $retVal -ne 0 ]; then
		echo "found failing generation for seed" $i
	fi
	rm "/tmp/rylink_out_$uuid"
	done
}

compile_and_run() {
	seed_start=$1
	nr=$2
	for (( i=$seed_start; i<=$(($seed_start + $nr)); i++ )); do
		uuid="id$i"
		path="${dir}/prog_${uuid}_0"

		#echo "compiling.. " $path;

		$CC $CC_FLAGS $path/*.c -o $path/main.out &> /dev/null;

		#echo "running.. " $path;

		./$path/main.out > /dev/null;
		retVal=$?
		if [ $retVal -ne 0 ]; then
			echo "found failing testcase for seed" $i
		fi
	done
}

test_per_proc=$(($nrTests / $j))
#
# Iterates over seeds for easy reproduction
for (( i=0; i<$j; i++)); do
	generate $(($i * $test_per_proc)) $test_per_proc &
done

wait

for (( i=0; i<$j; i++)); do
	compile_and_run $(($i * $test_per_proc)) $test_per_proc &
done

wait
