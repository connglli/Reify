directories:
	mkdir -p procedures mappings logs

clean:
	find procedures -type f -delete
	find mappings -type f -delete
	find logs -type f -delete
	rm -f exe exe.o 

all:
	make directories
	make clean
	./compile.sh codegen_procedure_generation.cpp
	python3 run.py