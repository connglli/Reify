UUID := $(shell uuidgen)

clean:
	rm -rf exe exe.o new_procedures

compile: function_parser.cpp
	mkdir -p new_procedures
	g++ -D_MP_INTERNAL -DNDEBUG -D_EXTERNAL_RELEASE  -std=c++20 -fvisibility=hidden -fvisibility-inlines-hidden -c -mfpmath=sse -msse -msse2 -O0 -fPIC   -o exe.o  -I/usr/include function_parser.cpp
	g++ -o exe exe.o /usr/lib/x86_64-linux-gnu/libz3.so -lpthread

run: clean compile
	./exe ../procedure_gen/procedures ../procedure_gen/mappings $(UUID)
