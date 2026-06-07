#!/bin/bash

# Parses a worker log for a specific function generation UUID, and regenerates all the function
# TODO: Worker logs what failed we should not even attempt fgen for seeds we know failed before

worker_log=$1
uuid=$2
outdir=$3
if [ -z "$4" ]; then
	rysmith="build/bin/rysmith"
else
	rysmith=$4
fi

# build the list of commands and execute them
awk -v u="$uuid" -v bin="$rysmith" -v out="$outdir" -F',' '
{
	if (!index($0, "Generating function: bin=./build/bin/rysmith, uuid=" u)) next

	sub(/.*=/,"",$2) # uuid
	sub(/.*=/,"-n ",$3) # -n
	sub(/.*\(/,"",$5)
	sub(/\).*/,"",$9)
	sub(/.*=/,"",$13)     # -A
	sub(/.*=/,"",$14)     # -U
	sub(/.*=/,"-s ",$15)  # -s
	sub(/.*=/,"",$16)     # extra

	if ($13 == "True")
		$13 = "-A"
	else
		$13 = ""

	if ($14 == "True")
		$14 = "-U"
	else
		$14 = ""

	cmd = \
		bin " "                       \
		$3 " " $13 " -S " " -m "      \
		" " $14 " " $15               \
		" --Xnum-bbls-per-fun " $5    \
		" --Xnum-vars-per-fun" $6     \
		" --Xnum-assigns-per-bbl" $7  \
		" --Xnum-vars-per-assign" $8  \
		" --Xnum-vars-in-cond " $9    \
		" --Xproc-timeout 3 "         \
		$16 " -o " out " " $2

	getline

	if (index($0, "Failure")) next
	else print cmd
}
' "$worker_log" | bash

for func in $outdir/*; do
	if [ $(ls $func | wc -l) -le 1 ]; then
		rm -r $func;
	fi
done
