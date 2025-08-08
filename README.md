# GraphFuzz

## Prerequisite

Z3: Build and install Z3's C++ binding
following [Build Z3 using Make and GCC/Clang](https://github.com/Z3Prover/z3?tab=readme-ov-file#building-z3-using-make-and-gccclang).

## Build GraphFuzz

```sh
# Build all targets
[DEBUG=true] make -j8 all

# Build the function generation target
[DEBUG=true] make -j8 fgen

# Build the program generation target
[DEBUG=true] make -j8 pgen
```

## Generate Programs

We can generate functions and programs using the following `make` commands.

```sh
# Get FGEN_LIMIT (0 mean unlimited) functions into FGEN_OUT_DIR, controlled by GEN_SEED.
# When FGEN_GEN_MAIN is set to any value (even false), a program will also be generated following each function
# When FGEN_GEN_SEXP is set to any value (even false), its S expression will also be generated following each function. This is required when we are going to generate new programs using them as seeds.
# When FGEN_ALL_OPS is set to any value (even false), the generation will use all kinds of operators for terms and expression
# When FGEN_INJ_UBS is set to any value (even false), the generation will inject undefined behaviors into the unexecuted basic blocks
# When tailing with -check-ub, each function (and program) will be checked against undefined behavior
# Note, the double quotes inside the single quotes cannot be ignored
[GEN_SEED=0]                                \
[FGEN_OUT_DIR=/path/to/output]              \
[FGEN_LIMIT=10000]                          \
[FGEN_GEN_MAIN=true]                        \
[FGEN_GEN_SEXP=true]                        \
[FGEN_ALL_OPS=true]                         \
[FGEN_INJ_UBS=true]                         \
[FGEN_EX_OPS='...']                         \
make gen-func-set[-check-ub]

# Get PGEN_LIMIT (0 means unlimited) programs into PGEN_IN_DIR, controlled by GEN_SEED.
# When tailing with -check, each program will be checked against undefined behavior
# Note, different from generating functions, the single quotes cannot have double quotes
[GEN_SEED=0]                                \
[PGEN_IN_DIR=/path/to/functions]            \
[PGEN_LIMIT=10000]                          \
[PGEN_EX_OPS='...']                         \
make gen-prog-set-check
```

For example, generating 100 functions into the `generated` directory with each function having `10` variables defined
and each function being checked UB-free, using the seed of `1`:

```sh
GEN_SEED=1                                  \
FGEN_OUT_DIR=generated                      \
FGEN_LIMIT=100                              \
FGEN_GEN_SEXP=true                          \
FGEN_GEN_MAIN=true                          \
FGEN_EX_OPS='--Xnum-vars-per-fun 10'        \
make gen-func-set-check-ub
```

The output directory (i.e., directories pointed by `FGEN_OUT_DIR` for function generation or `PGEN_IN_DIR` for program
generation) include the following sub-directories:

- `functions`: A set of functions that can be correctly compiled into `.o` (but not executable)
- `mappings`: The input-output mappings for each generated function, ensuring their executions are UB-free
- `loggings`: The detailed generation log for each function
- `sexpressions`: A set of S expressions corresponding to the generated functions, which can be used as seeds for
  generating new programs
- `programs`: A set of programs that can be solely compiled (into executables), corresponding to those functions in
  the `functions` directory

Afterward, we can further generate 100 new programs with the above generated functions as seeds:

```sh
GEN_SEED=0                                  \
PGEN_IN_DIR=generated                       \
PGEN_LIMIT=100                              \
make gen-prog-set-check
```

The new programs will be added to the `programs` subdirectory and each program will be checked against undefined
behavior.

## Fuzz Compilers

The fuzzing process can be started by for example running the following command:

```sh
python3 scripts/fuzz.py -o fuzzdir -s 0 -j 10 'gcc -O3 -fno-tree-slsr -fno-tree-ch'
```

This means that the fuzzing process will start 10 jobs in parallel, each compiling the generated programs with the given
command (i.e., `gcc -O3 -fno-tree-slsr -fno-tree-ch`) and seeding the random number generator with the `0`. All the
generated programs will be put into the `fuzzdir` directory. Note, useless programs will be removed during the fuzzing
process to save some space.

## Other Scripts

There're some other useful scripts.

```sh
# Generate a set of functions (which is called by the above `make gen-func-set[-check-ub]` command)
python3 scripts/fgen.py -h

# Generate a single function (which is called by the above command)
./build/bin/fgen -h

# Generate a set of programs (which is called by the above `make gen-prog-set[-check]` command)
./build/bin/pgen -h

# Check UB-freeness of the generated functions and their mappings
python3 scripts/ubchk.py <func_dir> <map_dir>

# Check UB-freeness of the generated programs and their mappings
python3 scripts/ubchk.py <prog_dir>
```
