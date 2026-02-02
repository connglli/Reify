CXX ?= $(shell command -v g++ || command -v clang++)
CC  ?= $(shell command -v gcc || command -v clang)
PY3 ?= $(shell command -v python3 || shell command -v python)

# Complain if we don't have any available compilers
ifeq ($(CXX),)
$(error Neither g++ nor clang++ found! Please install one of them.)
else ifeq ($(CC),)
$(error Neither gcc nor clang found! Please install one of them.)
else ifeq ($(PY3),)
$(error python3 not found! Please install it.)
endif

# Check if our dependencies are not installed
FOUND_Z3 := $(if $(shell ldconfig -p | grep z3),y,n)
ifeq ($(FOUND_Z3),n)
$(error libz3.so not found! Please install it)
endif

CSTD   := c17
CXXSTD := c++20

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

LIB_SRC_CXX := $(shell find $(LIB_DIR) -type f -name "*.cpp")
LIB_SRC_C   := $(shell find $(LIB_DIR) -type f -name "*.c")
LIB_SRC     := $(LIB_SRC_CXX) $(LIB_SRC_C)

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
OPTFLAGS := $(if $(DEBUG),-O0,-O2)

CXXFLAGS := $(DBGFLAGS) -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -std=${CXXSTD} -frtti ${OPTFLAGS} -I$(INC_DIR)
CFLAGS   := $(DBGFLAGS) -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -std=${CSTD} ${OPTFLAGS} -I$(INC_DIR)
LDFLAGS  := $(DBGFLAGS) -lz3 -lpthread -lz


########################################################################
## Building Targets
########################################################################

.PHONY: clean all lib fgen pgen symircc bins test testprogs testfuncs genfuncs genprogs

all: lib bins

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(LIB_OBJ_DIR)/%.o: $(LIB_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

$(LIB_OBJ_DIR)/%.o: $(LIB_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

# Compile checksum implementation specifically as it requires the macro
$(LIB_OBJ_DIR)/chksum.o: $(LIB_DIR)/chksum.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -D$(CHKSUM_CODE_MACRO)=$(CHKSUM_CODE_VALUE) -o $@ -c $<

lib: $(LIB_OBJ)

$(BIN_DIR)/fgen: $(LIB_OBJ) $(OBJ_DIR)/func_gen.o
	@mkdir -p $(dir $@)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/pgen: $(LIB_OBJ) $(OBJ_DIR)/prog_gen.o
	@mkdir -p $(dir $@)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/symircc: $(LIB_OBJ) $(OBJ_DIR)/symircc.o
	@mkdir -p $(dir $@)
	$(CXX) -o $@ $^ $(LDFLAGS)

fgen: $(BIN_DIR)/fgen

pgen: $(BIN_DIR)/pgen

symircc: $(BIN_DIR)/symircc

bins: fgen pgen symircc


########################################################################
## Generation Targets
########################################################################

GEN_SEED ?= -1

## Function Generation

FGEN_OUT_DIR  ?= generated
FGEN_LIMIT    ?= 1000000
FGEN_SEXP_OPT := $(if $(FGEN_GEN_SEXP),--sexp,)
FGEN_MAIN_OPT := $(if $(FGEN_GEN_MAIN),--main,)
FGEN_AOPS_OPT := $(if $(FGEN_ALL_OPS),--allops,)
FGEN_UBIJ_OPT := $(if $(FGEN_INJ_UBS),--injubs,)
FGEN_EXT_OPTS  := $(if $(FGEN_EXT_OPTS),--extra="$(FGEN_EXT_OPTS)",)

genfuncs: fgen
	@mkdir -p $(FGEN_OUT_DIR)
	$(PY3) scripts/fgen.py --output $(FGEN_OUT_DIR) --seed $(GEN_SEED) --limit $(FGEN_LIMIT) $(FGEN_MAIN_OPT) $(FGEN_SEXP_OPT) $(FGEN_AOPS_OPT) $(FGEN_UBIJ_OPT) $(FGEN_EXT_OPTS)

testfuncs: fgen
	@mkdir -p $(FGEN_OUT_DIR)
	$(PY3) scripts/fgen.py --output $(FGEN_OUT_DIR) --seed $(GEN_SEED) --limit $(FGEN_LIMIT) $(FGEN_MAIN_OPT) $(FGEN_SEXP_OPT) $(FGEN_AOPS_OPT) $(FGEN_UBIJ_OPT) $(FGEN_EXT_OPTS) --check

## Program Generation

PGEN_IN_DIR    ?= $(FGEN_OUT_DIR)
PGEN_LIMIT     ?= 100000
PGEN_EX_OPTS   ?=

genprogs: pgen
	$(PY3) scripts/retouch.py $(PGEN_IN_DIR)  # cleanup
	$(BIN_DIR)/pgen --input $(PGEN_IN_DIR) --limit $(PGEN_LIMIT) --seed $(GEN_SEED) $(PGEN_EX_OPTS) $(shell uuidgen)

testprogs: pgen
	$(PY3) scripts/retouch.py $(PGEN_IN_DIR)  # cleanup
	$(BIN_DIR)/pgen --input $(PGEN_IN_DIR) --limit $(PGEN_LIMIT) --seed $(GEN_SEED) $(PGEN_EX_OPTS) --debug $(shell uuidgen)
	$(PY3) scripts/ubchk.py $(PGEN_IN_DIR)


########################################################################
## Other Targets
########################################################################

clean:
	rm -rf $(BUILD_DIR)
