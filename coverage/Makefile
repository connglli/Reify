.PHONY: all build gcc llvm clean

all: build

build: gcc llvm

gcc:
	@echo "Building GCC instrumented version..."
	./build_instrumented_gcc.sh

llvm:
	@echo "Building LLVM instrumented version..."
	./build_instrumented_llvm.sh

clean:
	@echo "Removing everything except .sh files and Makefile..."
	find . -mindepth 1 ! -name '*.sh' ! -name 'Makefile' -exec rm -rf {} +
