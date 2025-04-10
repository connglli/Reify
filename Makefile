clean:
	rm -rf obj/* gold/* definitions/* inlinable/* main_code/* error/* graphs/* smt_queries/* models/* walks/* basic_blocks/* static_inlinable/*
	rm -rf gcc_bin_O0/* gcc_bin_O1/* gcc_bin_O2/* gcc_bin_O3/* gcc_bin_Ofast/* gcc_bin_Os/*
	rm -rf clang_bin_O0/* clang_bin_O1/* clang_bin_O2/* clang_bin_O3/* clang_bin_Ofast/*  clang_bin_Os/*
	> bug_report.txt


directories:
	mkdir -p obj gold definitions inlinable static_inlinable main_code error graphs smt_queries models walks basic_blocks
	mkdir -p gcc_bin_O0 gcc_bin_O1 gcc_bin_O2 gcc_bin_O3 gcc_bin_Ofast gcc_bin_Os
	mkdir -p clang_bin_O0 clang_bin_O1 clang_bin_O2 clang_bin_O3 clang_bin_Ofast clang_bin_Os