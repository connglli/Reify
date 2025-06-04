CXX := $(shell command -v g++ || command -v clang++)

# Complain if we don't have any available compilers
ifeq ($(CXX),)
	$(error Neither g++ nor clang++ found! Please install one of them.)
endif

# TODO: Complain if our dependencies are not installed for example z3

INC_DIR := include
LIB_DIR := lib
SRC_DIR := src

BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin
OBJ_DIR := $(BUILD_DIR)/obj
LIB_OBJ_DIR := $(OBJ_DIR)/lib

ifdef NDEBUG
	DBGFLAGS :=
else
	DBGFLAGS := -g
endif
CXXFLAGS := $(DBGFALGS) -Wall -Wextra -std=c++20 -O0 -I$(INC_DIR)
LDFLAGS := -lz3 -lpthread

LIB_SRC := $(wildcard $(LIB_DIR)/*.cpp)
LIB_OBJ := $(patsubst $(LIB_DIR)/%.cpp,$(LIB_OBJ_DIR)/%.o,$(LIB_SRC))

.PHONY: all clean lib gen-func-set gen-func-set-check-ubs gen-prog-set

########################################################################
## Building Targets
########################################################################

all: lib fgen pgen

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(LIB_OBJ_DIR)/%.o: $(LIB_DIR)/%.cpp
	@mkdir -p $(LIB_OBJ_DIR)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

lib: $(LIB_OBJ)

fgen: $(LIB_OBJ) $(OBJ_DIR)/func_gen.o
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $(BIN_DIR)/fgen $^ $(LDFLAGS) 

pgen: $(LIB_OBJ) $(OBJ_DIR)/interp_gen.o
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

gen-func-set-check-ubs: fgen
	@mkdir -p $(PROCEDURE_DIR) $(MAPPING_DIR) $(LOGGINGS_DIR)
	@python3 scripts/fgen.py --check --limit 1_000_000

gen-prog-set: pgen
	@mkdir -p $(NEW_PROCEDURE_DIR)
	@python3 scripts/retouch.py  # cleanup
	$(BIN_DIR)/pgen $(PROCEDURE_DIR) $(MAPPING_DIR) $(shell uuidgen)

########################################################################
## Other Targets
########################################################################

clean:
	rm -rf $(BUILD_DIR) $(NEW_PROCEDURE_DIR) $(PROCEDURE_DIR) $(MAPPING_DIR) $(LOGGINGS_DIR)

