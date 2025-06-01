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

.PHONY: all clean genfunc geninterp genintrap

########################################################################
## Buliding Targets
########################################################################

all: fgen interpgen

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

fgen: $(OBJ_DIR)/func_gen.o
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $(BIN_DIR)/fgen $^ $(LDFLAGS) 

interpgen: $(OBJ_DIR)/interp_gen.o
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $(BIN_DIR)/interpgen $^ $(LDFLAGS) 

intrapgen: $(OBJ_DIR)/intrap_gen.o
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $(BIN_DIR)/intrapgen $^ $(LDFLAGS) 

########################################################################
## Generation Targets
########################################################################

NEW_PROCEDURE_DIR := new_procedures
PROCEDURE_DIR := procedures
MAPPING_DIR   := mappings

genfuncset: fgen
	@mkdir -p $(PROCEDURE_DIR) $(MAPPING_DIR)
	@python3 scripts/fgen.py

geninterpset: interpgen
	@mkdir -p $(NEW_PROCEDURE_DIR)
	@python3 scripts/retouch.py  # cleanup
	$(BIN_DIR)/interpgen $(PROCEDURE_DIR) $(MAPPING_DIR) $(shell uuidgen)

genintrapset: intrapgen
	@echo "Not implemented yet"

########################################################################
## Other Targets
########################################################################

clean:
	rm -rf $(BUILD_DIR) $(NEW_PROCEDURE_DIR) $(PROCEDURE_DIR) $(MAPPING_DIR)

