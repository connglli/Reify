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
FOUND_BITWUZLA := $(shell pkg-config --exists bitwuzla && echo y || echo n)
ifeq ($(FOUND_BITWUZLA),n)
$(error Bitwuzla not found! Please install it!)
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

# Generate and include header dependency files (.d) so incremental builds
# rebuild objects when headers change.
DEPFLAGS := -MMD -MP

CXXFLAGS := $(DBGFLAGS) -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -std=${CXXSTD} -frtti ${OPTFLAGS} $(DEPFLAGS) -I$(INC_DIR)
CFLAGS   := $(DBGFLAGS) -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -std=${CSTD} ${OPTFLAGS} $(DEPFLAGS) -I$(INC_DIR)
LDFLAGS  := $(DBGFLAGS) $(shell pkg-config --libs bitwuzla) -lpthread -lz

# Dependency files produced alongside object files.
DEP_FILES := $(LIB_OBJ:.o=.d) $(OBJ_DIR)/rysmith.d $(OBJ_DIR)/rylink.d $(OBJ_DIR)/symircc.d
-include $(DEP_FILES)


########################################################################
## Building Targets
########################################################################

.PHONY: clean all lib rysmith rylink symircc bins testrylink testrysmith

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

$(BIN_DIR)/rysmith: $(LIB_OBJ) $(OBJ_DIR)/rysmith.o
	@mkdir -p $(dir $@)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/rylink: $(LIB_OBJ) $(OBJ_DIR)/rylink.o
	@mkdir -p $(dir $@)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/symircc: $(LIB_OBJ) $(OBJ_DIR)/symircc.o
	@mkdir -p $(dir $@)
	$(CXX) -o $@ $^ $(LDFLAGS)

rysmith: $(BIN_DIR)/rysmith

rylink: $(BIN_DIR)/rylink

symircc: $(BIN_DIR)/symircc

bins: rysmith rylink symircc


########################################################################
## Generation Targets
########################################################################

RY_SEED  ?= -1
RY_LIMIT ?= 1000000
RY_DIR   ?= generated

## rysmith: Leaf Function Generation

RYSMITH_EXTRA := $(if $(RYSMITH_EXTRA),--extra="$(RYSMITH_EXTRA)",)

testrysmith: rysmith
	@mkdir -p $(RY_DIR)
	$(PY3) scripts/rysmith.py --output $(RY_DIR) --seed $(RY_SEED) --limit $(RY_LIMIT) $(RYSMITH_EXTRA) --check

## rylink: Whole Program Generation

RYLINK_EXTRA ?=

testrylink: rylink
	$(PY3) scripts/retouch.py $(RY_DIR)  # cleanup
	$(BIN_DIR)/rylink --input $(RY_DIR) --limit $(RY_LIMIT) --seed $(RY_SEED) $(RYLINK_EXTRA) --debug $(shell uuidgen)
	$(PY3) scripts/ubchk.py $(RY_DIR)


########################################################################
## Other Targets
########################################################################

clean:
	rm -rf $(BUILD_DIR)
