CXX := $(shell command -v g++ || command -v clang++)
CC  := $(shell command -v gcc || command -v clang)

# Complain if we don't have any available compilers
ifeq ($(CXX),)
	$(error Neither g++ nor clang++ found! Please install one of them.)
else ifeq ($(CC),)
	$(error Neither gc nor clang found! Please install one of them.)
endif

# TODO: Complain if our dependencies are not installed for example z3


INC_DIR := include
LIB_DIR := lib
SRC_DIR := src
RES_DIR := res

BUILD_DIR   := build
BIN_DIR     := $(BUILD_DIR)/bin
OBJ_DIR     := $(BUILD_DIR)/obj
LIB_OBJ_DIR := $(OBJ_DIR)/lib


########################################################################
## Sources, Objects, and Resources
########################################################################

LIB_SRC_CXX := $(wildcard $(LIB_DIR)/*.cpp)
LIB_SRC_C   := $(wildcard $(LIB_DIR)/*.c)

LIB_OBJ_CXX := $(patsubst $(LIB_DIR)/%.cpp,$(LIB_OBJ_DIR)/%.o,$(LIB_SRC_CXX))
LIB_OBJ_C   += $(patsubst $(LIB_DIR)/%.c,$(LIB_OBJ_DIR)/%.o,$(LIB_SRC_C))
LIB_OBJ     := $(LIB_OBJ_CXX) $(LIB_OBJ_C)


########################################################################
## Checksum
########################################################################

# We are using the Macro trick to make the maintaining of checksum easier.
# That said, we can write checksum code without taking care of how these
# code are transformed into plain text (string literal) in our generated
# code. This is accomplished collaboratedly by the following script and
# the code in checksum.{h,c}pp. Specifically, some macros shown below.
CHKSUM_RES := $(RES_DIR)/cchksum.txt

# Since make will replace all newlines and remove quotes, we should escape them
CHKSUM_CODE_MACRO := STATELESS_CHECKSUM_CODE
CHKSUM_CODE_VALUE := "\"$(shell (awk '{ printf "%s\\n", $$0 }' $(CHKSUM_RES) | sed 's/"/\\\\\\"/g'))\""


########################################################################
## Building Flags
########################################################################

DBGFLAGS := $(if $(DEBUG),-g,)
CXXFLAGS := $(DBGFLAGS) -Wall -Wextra -std=c++20 -O0 -I$(INC_DIR)
CFLAGS   := $(DBGFLAGS) -Wall -Wextra -std=c17 -O0
LDFLAGS  := $(DBGFLAGS) -lz3 -lpthread


.PHONY: all clean lib gen-func-set gen-func-set-check-ubs gen-prog-set gen-prog-set-check


########################################################################
## Building Targets
########################################################################

all: lib fgen pgen

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(LIB_OBJ_DIR)/%.o: $(LIB_DIR)/%.c
	@mkdir -p $(LIB_OBJ_DIR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(LIB_OBJ_DIR)/%.o: $(LIB_DIR)/%.cpp
	@mkdir -p $(LIB_OBJ_DIR)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

# Compile checksum implementation specifically as it requires the macro
$(LIB_OBJ_DIR)/chksum.o: $(LIB_DIR)/chksum.cpp
	@mkdir -p $(LIB_OBJ_DIR)
	$(CXX) $(CXXFLAGS) -D$(CHKSUM_CODE_MACRO)=$(CHKSUM_CODE_VALUE) -o $@ -c $<

lib: $(LIB_OBJ)

fgen: $(LIB_OBJ) $(OBJ_DIR)/func_gen.o
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $(BIN_DIR)/fgen $^ $(LDFLAGS) 

pgen: $(LIB_OBJ) $(OBJ_DIR)/prog_gen.o
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $(BIN_DIR)/pgen $^ $(LDFLAGS) 


########################################################################
## Generation Targets
########################################################################

GEN_SEED ?= -1

## Function Generation

FGEN_OUT_DIR  ?= generated
FGEN_FUNS_DIR := $(FGEN_OUT_DIR)/functions
FGEN_MAPS_DIR := $(FGEN_OUT_DIR)/mappings
FGEN_LOGS_DIR := $(FGEN_OUT_DIR)/loggings
FGEN_LIMIT    ?= 1000000

gen-func-set: fgen
	@mkdir -p $(FGEN_FUNS_DIR) $(FGEN_MAPS_DIR)
	@python3 scripts/fgen.py --output $(FGEN_OUT_DIR) --seed $(GEN_SEED) --limit $(FGEN_LIMIT)

gen-func-set-check-ubs: fgen
	@mkdir -p $(FGEN_FUNS_DIR) $(FGEN_MAPS_DIR) $(FGEN_LOGS_DIR)
	@python3 scripts/fgen.py --output $(FGEN_OUT_DIR) --seed $(GEN_SEED) --limit $(FGEN_LIMIT) --check

## Program Generation

PGEN_IN_DIR    ?= $(FGEN_OUT_DIR)
PGEN_PROGS_DIR := $(PGEN_IN_DIR)/programs
PGEN_LIMIT     ?= 100000

gen-prog-set: pgen
	@mkdir -p $(PGEN_PROGS_DIR)
	@python3 scripts/retouch.py $(PGEN_IN_DIR)  # cleanup
	$(BIN_DIR)/pgen --input $(PGEN_IN_DIR) --limit $(PGEN_LIMIT) --seed $(GEN_SEED) $(shell uuidgen)

gen-prog-set-check: pgen
	@mkdir -p $(PGEN_PROGS_DIR)
	@python3 scripts/retouch.py $(PGEN_IN_DIR)  # cleanup
	$(BIN_DIR)/pgen --input $(PGEN_IN_DIR) --limit $(PGEN_LIMIT) --seed $(GEN_SEED) --debug $(shell uuidgen)
	@python3 scripts/ubchk.py $(PGEN_PROGS_DIR)


########################################################################
## Other Targets
########################################################################

clean:
	rm -rf $(BUILD_DIR) $(FGEN_OUT_DIR) $(PGEN_IN_DIR)
