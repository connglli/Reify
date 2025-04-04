PATH_TO_Z3=/home/kavya/z3
/usr/bin/g++ -D_MP_INTERNAL -DNDEBUG -D_EXTERNAL_RELEASE  -std=c++20 -fvisibility=hidden -fvisibility-inlines-hidden -c -mfpmath=sse -msse -msse2 -O0 -fPIC   -o exe.o  -I$PATH_TO_Z3/src/api -I$PATH_TO_Z3/src/api/c++ $1
/usr/bin/g++ -o exe exe.o $PATH_TO_Z3/build/libz3.so -lpthread