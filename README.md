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
# Get FGEN_LIMIT (0 mean unlimited) functions into FGEN_OUT_DIR, controlled by GEN_SEED.
# When FGEN_GEN_MAIN is set to any value (even false), a program will also be generated following each function
# When FGEN_GEN_SEXP is set to any value (even false), its S experssion will also be generated following each function. This is required when we are going to generate new programs using them as seeds.
# When FGEN_ALL_OPTS is set to any value (even false), the generation will use all kinds of operators for terms and expression
# When tailing with -check-ub, each function (and program) will be checked against undefined behavior
# Note, the double quotes inside the single quotes cannot be ignored
[FGEN_OUT_DIR=/path/to/output]              \
[FGEN_LIMIT=10000]                          \
[GEN_SEED=0]                                \
[FGEN_GEN_MAIN=true]                        \
[FGEN_GEN_SEXP=true]                        \
[FGEN_ALL_OPTS=true]                        \
[FGEN_EX_OPS='"..."']                       \
make gen-func-set[-check-ub]

# Get PGEN_LIMIT (0 means unlimited) programs into PGEN_IN_DIR, controlled by GEN_SEED.
# When tailing with -check, each program will be checked against undefined behavior
# Note, different from generating functions, the single quotes cannot have double quotes
[PGEN_IN_DIR=/path/to/functions]            \
[PGEN_LIMIT=10000]                          \
[GEN_SEED=0]                                \
[PGEN_EX_OPS='...']                         \
make gen-prog-set-check

# Check UB-freeness of the generated functions and their mappings
python3 scripts/ubchk.py <func_dir> <map_dir>

# Check UB-freeness of the generated programs and their mappings
python3 scripts/ubchk.py <prog_dir>
```

For example, generating 100 functions into the `generated` directory with each function having `10` variables defined and each function being checked UB-free, using the seed of `1`:

```sh
FGEN_OUT_DIR=generated                    \
FGEN_LIMIT=100                            \
GEN_SEED=1                                \
FGEN_GEN_SEXP=true                        \
FGEN_EX_OPS='"--Xnum-vars-per-fun 10"'    \
make gen-func-set-check-ub
```

The output directory (i.e., directories pointed by `FGEN_OUT_DIR` for function generation or `PGEN_IN_DIR` for program generation) include the following sub-directories:

- `functions`: A set of functions that can be correctly compiled into `.o` (but not executable)
- `mappings`: The input-output mappings for each generated function, ensuring their executions are UB-free
- `loggings`: The detailed generation log for each function
- `programs`: A set of programs that can be solely compiled (into executables), corresponding to those functions in
  the `functions` directory

Afterward, we can further generate 100 new programs with the above programs as seeds:

```sh
PGEN_IN_DIR=generated                     \
PGEN_LIMIT=100                            \
make gen-prog-set-check
```

The new programs will be added to the `programs` subdirectory and each program will be checked against undefined behavior.

## Fuzz

[TBA]

