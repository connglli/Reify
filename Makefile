CXX := $(shell command -v clang++ || command -v g++)

# Complain if we don't have any available compilers
ifeq ($(CXX),)
	$(error Neither clang++ nor g++ found! Please install one of them.)
endif

# TODO: Complain if our dependencies are not installed for example z3

INC_DIR := include
SRC_DIR := src

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin

CXXFLAGS := -Wall -Wextra -std=c++20 -O0 -I$(INC_DIR)
LDFLAGS := -lz3 -lpthread

.PHONY: all clean gen-func-set gen-prog-set

########################################################################
## Buliding Targets
########################################################################

all: fgen pgen

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

fgen: $(OBJ_DIR)/func_gen.o
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $(BIN_DIR)/fgen $^ $(LDFLAGS) 

pgen: $(OBJ_DIR)/interp_gen.o
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $(BIN_DIR)/pgen $^ $(LDFLAGS) 


########################################################################
## Generation Targets
########################################################################

PROCEDURE_DIR        := procedures
MAPPING_DIR          := mappings
LOGGINGS_DIR         := logs
NEW_PROCEDURE_DIR    := new_procedures

gen-func-set: fgen
	@mkdir -p $(PROCEDURE_DIR) $(MAPPING_DIR)
	@python3 scripts/fgen.py

gen-func-set-checked: fgen
	@mkdir -p $(PROCEDURE_DIR) $(MAPPING_DIR) $(LOGGINGS_DIR)
	@python3 scripts/fgen.py --check

gen-prog-set: pgen
	@mkdir -p $(NEW_PROCEDURE_DIR)
	@python3 scripts/retouch.py  # cleanup
	$(BIN_DIR)/pgen $(PROCEDURE_DIR) $(MAPPING_DIR) $(shell uuidgen)

########################################################################
## Other Targets
########################################################################

clean:
	rm -rf $(BUILD_DIR) $(NEW_PROCEDURE_DIR) $(PROCEDURE_DIR) $(MAPPING_DIR) $(LOGGINGS_DIR)

