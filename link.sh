#!/bin/bash
# source /opt/intel/oneapi/setvars.sh intel64
FILE_NUMBER=$1
echo "File number is $FILE_NUMBER"
GCC_PATH=/local/home/kchopra/compilers/bin/gcc
CLANG_PATH=/local/home/kchopra/compilers/bin/clang
# ICC_PATH=/opt/intel/oneapi/compiler/2023.2.4/linux/bin/icx
compileGCC() {
    # This function compiles the code with GCC
    # $1 is the number
    # $2 is the optimization level
    timeout 5 $GCC_PATH -c main_code/generated_code_$1.c -o obj/generated_code_$1.o -O$2
    timeout 5 $GCC_PATH -c definitions/variable_definitions_$1.c -o obj/variable_definitions_$1.o -O$2
    timeout 5 $GCC_PATH -o gcc_bin_O$2/prog_$1 obj/generated_code_$1.o obj/variable_definitions_$1.o -O$2
}

# compileICX() {
#     # This function compiles the code with ICC
#     # $1 is the number
#     # $2 is the optimization level
#     $ICC_PATH -c main_code/generated_code_$1.c -o obj/generated_code_$1.o -O$2
#     $ICC_PATH -c definitions/variable_definitions_$1.c -o obj/variable_definitions_$1.o -O$2
#     $ICC_PATH -o icc_bin_O$2/prog_$1 obj/generated_code_$1.o obj/variable_definitions_$1.o -O$2
# }
# compileICXInlinable(){
#     # This function compiles the code with ICC
#     # $1 is the number
#     # $2 is the optimization level
#     $ICC_PATH -O$2 -o icc_bin_O$2/prog_inlinable_$1 inlinable/stat_resolvable_$1.c
# }
# compileICX $1 0
# compileICX $1 1
# compileICX $1 2
# compileICX $1 3
# compileICX $1 fast
# compileICXInlinable $1 0
# compileICXInlinable $1 1
# compileICXInlinable $1 2
# compileICXInlinable $1 3
# compileICXInlinable $1 fast
compileGCCInlinable(){
    # This function compiles the code with GCC
    # $1 is the number
    # $2 is the optimization level
    timeout 5 $GCC_PATH -O$2 -o gcc_bin_O$2/prog_static_inlinable_$1 static_inlinable/stat_resolvable_$1.c
    timeout 5 $GCC_PATH -O$2 -o gcc_bin_O$2/prog_inlinable_$1 inlinable/stat_resolvable_$1.c
}

compileGCC $1 0
compileGCC $1 1
compileGCC $1 2
compileGCC $1 3
compileGCC $1 fast
compileGCC $1 s
compileGCCInlinable $1 0
compileGCCInlinable $1 1
compileGCCInlinable $1 2
compileGCCInlinable $1 3
compileGCCInlinable $1 fast
compileGCCInlinable $1 s

compileClang() {
    # This function compiles the code with Clang
    # $1 is the number
    # $2 is the optimization level
    timeout 5 $CLANG_PATH -c main_code/generated_code_$1.c -o obj/generated_code_$1.o -O$2
    timeout 5 $CLANG_PATH -c definitions/variable_definitions_$1.c -o obj/variable_definitions_$1.o -O$2
    timeout 5 $CLANG_PATH -o clang_bin_O$2/prog_$1 obj/generated_code_$1.o obj/variable_definitions_$1.o -O$2
}
compileClangInlinable(){
    # This function compiles the code with Clang
    # $1 is the number
    # $2 is the optimization level
    timeout 5 $CLANG_PATH -O$2 -o clang_bin_O$2/prog_inlinable_$1 inlinable/stat_resolvable_$1.c
    timeout 5 $CLANG_PATH -O$2 -o clang_bin_O$2/prog_static_inlinable_$1 static_inlinable/stat_resolvable_$1.c
}
compileClang $1 0
compileClang $1 1
compileClang $1 2
compileClang $1 3
compileClang $1 fast
compileClang $1 s
compileClangInlinable $1 0
compileClangInlinable $1 1
compileClangInlinable $1 2
compileClangInlinable $1 3
compileClangInlinable $1 fast
compileClangInlinable $1 s
# Now we will run all the binaries and compare the outputs

runBinaries() {
    # This function runs all the binaries and compares the outputs
    # $1 is the number
    # $2 is the optimization level
    timeout 5 ./gcc_bin_O$2/prog_$1 > gcc_bin_O$2/output_$1.txt
    timeout 5 ./clang_bin_O$2/prog_$1 > clang_bin_O$2/output_$1.txt
    timeout 5 ./gcc_bin_O$2/prog_static_inlinable_$1 > gcc_bin_O$2/output_static_inlinable_$1.txt
    timeout 5 ./clang_bin_O$2/prog_static_inlinable_$1 > clang_bin_O$2/output_static_inlinable_$1.txt
    timeout 5 ./gcc_bin_O$2/prog_inlinable_$1 > gcc_bin_O$2/output_inlinable_$1.txt
    timeout 5 ./clang_bin_O$2/prog_inlinable_$1 > clang_bin_O$2/output_inlinable_$1.txt
    
    # timeout 5 ./icc_bin_O$2/prog_$1 > icc_bin_O$2/output_$1.txt
    # timeout 5 ./icc_bin_O$2/prog_inlinable_$1 > icc_bin_O$2/output_inlinable_$1.txt
}

runBinaries $1 0
runBinaries $1 1
runBinaries $1 2
runBinaries $1 3
runBinaries $1 fast
runBinaries $1 s

check() {
    # This function checks if the output of the binary is the same as the gold standard
    # If the output is different, it prints the output of the binary and flags it as a bug
    # $1 is the output file of the binary
    # $2 is the gold standard file
    

    readarray output < $1
    readarray gold < $2
    # Check if the output is the same as the gold standard
    if [ "${output[@]}" != "${gold[@]}" ]; then
        echo "Output of $1 is different from gold standard $2"
        echo "Output of $1:"
        for line in "${output[@]}"; do
            echo "$line"
        done
        echo "Gold standard $2:"
        for line in "${gold[@]}"; do
            echo "$line"
        done
        #write buggy output to a file
        echo "Output of $1 is different from gold standard $2" >> bug_report.txt
    fi
}

checkGCCOutput() {
    # This function checks if the output of the binary is the same as the gold standard
    # If the output is different, it prints the output of the binary and flags it as a bug
    # $1 is the output file of the binary
    # $2 is the gold standard file
    # $3 is the optimization level
    check gcc_bin_O$2/output_$1.txt gold/chronological_values_$1.csv
    check gcc_bin_O$2/output_inlinable_$1.txt gold/chronological_values_$1.csv
    check gcc_bin_O$2/output_static_inlinable_$1.txt gold/chronological_values_$1.csv
}
checkClangOutput(){
    check clang_bin_O$2/output_$1.txt gold/chronological_values_$1.csv
    check clang_bin_O$2/output_inlinable_$1.txt gold/chronological_values_$1.csv
    check clang_bin_O$2/output_static_inlinable_$1.txt gold/chronological_values_$1.csv
}

# checkICCOutput(){
#     check icc_bin_O$2/output_$1.txt gold/chronological_values_$1.csv
#     check icc_bin_O$2/output_inlinable_$1.txt gold/chronological_values_$1.csv
# }

# checkICCOutput $1 0
# checkICCOutput $1 1
# checkICCOutput $1 2
# checkICCOutput $1 3
# checkICCOutput $1 fast

checkGCCOutput $1 0
checkGCCOutput $1 1
checkGCCOutput $1 2
checkGCCOutput $1 3
checkGCCOutput $1 fast
checkGCCOutput $1 s

checkClangOutput $1 0
checkClangOutput $1 1
checkClangOutput $1 2
checkClangOutput $1 3
checkClangOutput $1 fast
checkClangOutput $1 s
