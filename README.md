# GraphFuzz

## Prerequisite

Z3: Build and install Z3 C++
following [Build Z3 using Make and GCC/Clang](https://github.com/Z3Prover/z3?tab=readme-ov-file#building-z3-using-make-and-gccclang).

## Build

```sh
# Build all targets
[DEBUG=true] make all

# Build the function generation target
[DEBUG=true] make fgen

# Build the program generation target
[DEBUG=true] make pgen
```

## Generate

```sh
# Generate a set of functions
make gen-func-set

# Get FGEN_LIMIT (0 mean unlimited) functions and check if they are UB-free controlled by GEN_SEED
# When FGEN_GEN_MAIN is set to any value (even false), a program will also be generated following each function
[FGEN_OUT_DIR=/path/to/output] [FGEN_LIMIT=10000] [GEN_SEED=0] [FGEN_GEN_MAIN=true] make gen-func-set-check-ub

# Generate a set of programs
make gen-prog-set

# Get PGEN_LIMIT (0 means unlimited) programs and check if they are UB-free controlled by GEN_SEED
[PGEN_IN_DIR=/path/to/functions] [PGEN_LIMIT=10000] [GEN_SEED=0] make gen-prog-set-check

# Check UB-freeness of the generated functions and their mappings
python3 scripts/ubchk.py <func_dir> <map_dir>

# Check UB-freeness of the generated programs and their mappings
python3 scripts/ubchk.py <prog_dir>
```

The output directory (i.e., directories pointed by `FGEN_OUT_DIR` for function generation or `PGEN_IN_DIR` for program
generation) include the following sub-directories:

- `functions`: A set of functions that can be correctly compiled into `.o` (but not executable)
- `mappings`: The input-output mappings for each generated function, ensuring their executions are UB-free
- `loggings`: The detailed generation log for each function
- `programs`: A set of programs that can be solely compiled (into executables), corresponding to those functions in
  the `functions` directory

## Fuzz

[TBA]

