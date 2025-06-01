clean:
	rm -rf procedures mappings exe exe.o

compile: codegen_procedure_generation.cpp params.hpp better_graph.hpp
	mkdir -p procedures mappings
	g++ -D_MP_INTERNAL -DNDEBUG -D_EXTERNAL_RELEASE  -std=c++20 -fvisibility=hidden -fvisibility-inlines-hidden -c -mfpmath=sse -msse -msse2 -O0 -fPIC   -o exe.o  -I/usr/include codegen_procedure_generation.cpp
	g++ -o exe exe.o /usr/lib/x86_64-linux-gnu/libz3.so -lpthread

run: clean compile
	python3 run.py

check_run: clean compile
	python3 check_run.py

retouch:
	python3 retouch.py