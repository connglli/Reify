# GraphFuzz

## Preriquests

Z3

## Build

```
# Build all targets
make all

# Build function generation target
make fgen

# Build inter-procedural program generation target
make interpgen
```

## Generate

```
# Generate a set of functions
make genfuncset

# Generate a set of programs supporting inter-procedural functions
make geninterpset
```

## Fuzz

[TBA]


## Comments from Kavya

The subdirectory which has the code for intraprocedure generation is `src/func_gen.cpp`. I've added some comments that try to explain what each function does. Its hyperparameters are present in params.hpp. Again, I've briefly annotated that with comments as well. It depends on better_graph.hpp for graph initialisation and walk sampling. You can make run for codegen, followed by make retouch for cleanup (some files that are created are empty and this gets rid of them)

The subdirectory which has the code for interprocedure generation is `src/interp_gen.cpp`. It essentially samples a bunch of procedures and their corresponding mappings from their directories and knits them to create interprocedural programs indefintiely (so you need to kill the process externally - it is quite fast, and I would probably recommend running it for 40 mins max on `/local/home` for a single process. It runs slower on `/zdata` so you can run it longer there). Hyperparams are in params.hpp in the same directory. If the procedures are named name.c, then the corresponding mapping must be named name_mapping, but the procedure generation already takes care of that. You can tailor the directories where the procedures and mappings are stored in the Makefile
