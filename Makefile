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

BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin
OBJ_DIR := $(BUILD_DIR)/obj
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
CHKSUM_RES    := $(RES_DIR)/cchksum.txt

# Since make will replace all newlines and remove quotes, we should escape them
CHKSUM_CODE_MACRO := STATELESS_CHECKSUM_CODE
CHKSUM_CODE_VALUE := "\"$(shell (awk '{ printf "%s\\n", $$0 }' $(CHKSUM_RES) | sed 's/"/\\\\\\"/g'))\""


########################################################################
## Building Flags
########################################################################

ifdef NDEBUG
	DBGFLAGS :=
else
	DBGFLAGS := -g
endif
CXXFLAGS := $(DBGFALGS) -Wall -Wextra -std=c++20 -O0 -I$(INC_DIR)
CFLAGS := $(DBGFLAGS) -Wall -Wextra -std=c17 -O0
LDFLAGS := -lz3 -lpthread


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
	$(BIN_DIR)/pgen --procedures $(PROCEDURE_DIR) --mappings $(MAPPING_DIR) $(shell uuidgen)

gen-prog-set-check: pgen
	@mkdir -p $(NEW_PROCEDURE_DIR)
	@python3 scripts/retouch.py  # cleanup
	$(BIN_DIR)/pgen --procedures $(PROCEDURE_DIR) --mappings $(MAPPING_DIR) --limit 10000 --debug $(shell uuidgen)


########################################################################
## Other Targets
########################################################################

clean:
	rm -rf $(BUILD_DIR) $(NEW_PROCEDURE_DIR) $(PROCEDURE_DIR) $(MAPPING_DIR) $(LOGGINGS_DIR)

