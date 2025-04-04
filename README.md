1. You can fine tune the parameters for codegen in params.hpp : my idea right now is to spawn multiple processes with different sets of params and see if any could potentially trigger a bug
2. After that, please update the path to Z3 (built from source) in your system in `compile.sh`, and then please run `./compile.sh codegen_better.cpp`
2. In the script `link.sh`, please update the paths for gcc and clang on your system
3. Please don't modify the existing directory structure
5. Post all these tweaks, we can run `python3 run.py`, and the fuzzing process will start