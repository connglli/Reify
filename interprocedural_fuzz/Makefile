UUID := $(shell uuidgen)
clean:
	find new_procedures -type f -delete
	rm -f exe exe.o

compile: function_parser.cpp
	mkdir -p procedures mappings new_procedures
	./compile.sh function_parser.cpp

run: clean compile
	./exe ../procedure_gen/procedures ../procedure_gen/mappings $(UUID)