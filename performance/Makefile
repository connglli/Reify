# =============================================================================
# Makefile — program generation benchmarks
# Drives performance.sh (generates N programs from csmith, yarpgen,
# rysmith, rylink and computes token + CFG stats).
#
# Usage:
#   make performance        # short run (default)
#   make performance MODE=--full   # long run
#   make short              # explicit short run  (N=20)
#   make full               # explicit full run   (N=1000)
# =============================================================================

SHELL    := /bin/bash
SCRIPT   := ./performance.sh
DATA_DIR := /reify/data/performance
VENV_DIR := /reify/performance/.venv

# Default mode — override with: make performance MODE=--full
MODE ?= --short

.PHONY: all performance short full clean help

all: performance

## performance: Generate programs and compute stats. MODE=--short (default) or MODE=--full.
performance:
	@chmod +x $(SCRIPT)
	@$(SCRIPT) $(MODE)

## short: Run in short mode (N=20).
short:
	@chmod +x $(SCRIPT)
	@$(SCRIPT) --short

## full: Run in full mode (N=1000).
full:
	@chmod +x $(SCRIPT)
	@$(SCRIPT) --full

## clean: Remove all generated data, the venv, and Python cache files.
clean:
	@echo "Removing generated data at $(DATA_DIR)..."
	@rm -rf $(DATA_DIR)/csmith \
	         $(DATA_DIR)/yarpgen \
	         $(DATA_DIR)/rysmith \
	         $(DATA_DIR)/rylink
	@echo "Removing Python virtual environment at $(VENV_DIR)..."
	@rm -rf $(VENV_DIR)
	@echo "Removing Python cache files..."
	@find /reify/performance -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
	@find /reify/performance -type f \( -name "*.pyc" -o -name "*.pyo" \) -delete 2>/dev/null || true
	@echo "Clean done."

## help: Show this help message.
help:
	@echo ""
	@echo "Usage: make [TARGET]"
	@echo ""
	@echo "Targets:"
	@grep -E '^##' Makefile | sed 's/^## /  /'
	@echo ""
	@echo "Examples:"
	@echo "  make performance              # short run (N=20, default)"
	@echo "  make performance MODE=--full  # long run (N=1000)"
	@echo "  make short                    # explicit short run"
	@echo "  make full                     # explicit full run"
	@echo "  make clean                    # delete all outputs and venv"
	@echo ""
