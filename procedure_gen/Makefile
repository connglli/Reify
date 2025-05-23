clean:
	find procedures -type f -delete 
	find mappings -type f -delete 
	rm -f exe exe.o

compile: codegen_procedure_generation.cpp params.hpp better_graph.hpp
	mkdir -p procedures mappings
	./compile.sh codegen_procedure_generation.cpp

run: clean compile
	python3 run.py

retouch:
	python3 retouch.py