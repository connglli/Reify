# GraphFuzz

## Prerequisite

Z3: Build and install Z3 C++ following [Build Z3 using Make and GCC/Clang](https://github.com/Z3Prover/z3?tab=readme-ov-file#building-z3-using-make-and-gccclang).

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

# Get FGEN_LIMIT (0 mean unlimited) functions and check if they are UB-free
[FGEN_OUT_DIR=/path/to/output] [FGEN_LIMIT=10000] [GEN_SEED=0] make gen-func-set-check-ub

# Generate a set of programs
make gen-prog-set

# Get PGEN_LIMIT (0 means unlimited) programs and check if they are UB-free
[PGEN_IN_DIR=/path/to/functions] [PGEN_LIMIT=10000] [GEN_SEED=0] make gen-prog-set-check

# Check UB-freeness of the generated functions and their mappings
python3 scripts/ubchk.py <func_dir> <map_dir>

# Check UB-freeness of the generated programs and their mappings
python3 scripts/ubchk.py <prog_dir>
```

## Fuzz

[TBA]

## Comments from Kavya

The subdirectory which has the code for intraprocedure generation is `src/func_gen.cpp`. I've added some comments that
try to explain what each function does. Its hyperparameters are present in params.hpp. Again, I've briefly annotated
that with comments as well. It depends on better_graph.hpp for graph initialisation and walk sampling. You can make run
for codegen, followed by make retouch for cleanup (some files that are created are empty and this gets rid of them)

The subdirectory which has the code for interprocedure generation is `src/prog_gen.cpp`. It essentially samples a
bunch of procedures and their corresponding mappings from their directories and knits them to create interprocedural
programs indefintiely (so you need to kill the process externally - it is quite fast, and I would probably recommend
running it for 40 mins max on `/local/home` for a single process. It runs slower on `/zdata` so you can run it longer
there). Hyperparams are in params.hpp in the same directory. If the procedures are named name.c, then the corresponding
mapping must be named name_mapping, but the procedure generation already takes care of that. You can tailor the
directories where the procedures and mappings are stored in the Makefile
