PATH_TO_Z3=/local/home/kchopra/z3
/usr/bin/g++ -D_MP_INTERNAL -DNDEBUG -D_EXTERNAL_RELEASE  -std=c++20 -fvisibility=hidden -fvisibility-inlines-hidden -c -mfpmath=sse -msse -msse2 -O0 -fPIC   -o exe.o  -I/usr/include $1
/usr/bin/g++ -o exe exe.o /usr/lib/x86_64-linux-gnu/libz3.so -lpthread