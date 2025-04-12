clean:
	find obj -type f -delete
	find gold -type f -delete
	find definitions -type f -delete
	find inlinable -type f -delete
	find main_code -type f -delete
	find error -type f -delete	
	find graphs -type f -delete
	find smt_queries -type f -delete
	find models -type f -delete
	find walks -type f -delete
	find basic_blocks -type f -delete
	find static_inlinable -type f -delete
	find logs -type f -delete
	find gcc_bin_O0 -type f -delete
	find gcc_bin_O1 -type f -delete
	find gcc_bin_O2 -type f -delete
	find gcc_bin_O3 -type f -delete
	find gcc_bin_Ofast -type f -delete
	find gcc_bin_Os -type f -delete
	find clang_bin_O0 -type f -delete
	find clang_bin_O1 -type f -delete
	find clang_bin_O2 -type f -delete
	find clang_bin_O3 -type f -delete
	find clang_bin_Ofast -type f -delete
	find clang_bin_Os -type f -delete
	> bug_report.txt
	rm -f exe exe.o exe_init exe_init.o checkUB


directories:
	mkdir -p obj gold definitions inlinable static_inlinable main_code error graphs smt_queries models walks basic_blocks logs
	mkdir -p gcc_bin_O0 gcc_bin_O1 gcc_bin_O2 gcc_bin_O3 gcc_bin_Ofast gcc_bin_Os
	mkdir -p clang_bin_O0 clang_bin_O1 clang_bin_O2 clang_bin_O3 clang_bin_Ofast clang_bin_Os

codegen:
	make clean
	make directories
	./compile.sh codegen_better.cpp
	python3 run.py

codegen_init:
	make clean
	make directories
	./compile.sh codegen_init.cpp
	python3 run.py
	