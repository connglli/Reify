#!/bin/bash
# =============================================================================
# build_llvm_instrumented.sh
# Builds instrumented LLVM/Clang from a specific commit, out-of-source.
# =============================================================================
set -euo pipefail

LLVM_COMMIT="1ab4113d0e0d30ba923ea070530a0e65910b26cb"
LLVM_REPO="https://github.com/llvm/llvm-project.git"
LLVM_SRC="/reify/coverage/builds/llvm-src"
LLVM_BUILD="/reify/coverage/builds/llvm-build"
LLVM_INSTALL="/reify/coverage/builds/llvm-instrumented"

PROCS=$(nproc)
(( PROCS > 64 )) && PROCS=64

RED='\033[0;31m'; GREEN='\033[0;32m'; NC='\033[0m'
info() { echo -e "${GREEN}[INFO]${NC} $*"; }
die()  { echo -e "${RED}[ERROR]${NC} $*" >&2; exit 1; }

# ensure ld is available (mold can masquerade as ld)
if ! command -v ld &>/dev/null; then
    info "ld not found, symlinking mold as ld..."
    ln -sf "$(which mold)" /usr/local/bin/ld
fi

# ── clone & checkout ─────────────────────────────────────────────────────────
if [ ! -d "$LLVM_SRC/.git" ]; then
    info "Cloning LLVM (this may take a while — it's a big repo)..."
    # Shallow clone first to save time, then fetch the specific commit
    git clone --filter=blob:none "$LLVM_REPO" "$LLVM_SRC"
else
    info "LLVM source already cloned, skipping."
fi

info "Checking out commit $LLVM_COMMIT..."
cd "$LLVM_SRC"
git fetch origin
git checkout "$LLVM_COMMIT"

# ── out-of-source build ───────────────────────────────────────────────────────
info "Configuring LLVM/Clang (out-of-source build)..."
rm -rf "$LLVM_BUILD"
mkdir -p "$LLVM_BUILD" "$LLVM_INSTALL"

cmake -S "$LLVM_SRC/llvm" \
      -B "$LLVM_BUILD" \
      -DCMAKE_INSTALL_PREFIX="$LLVM_INSTALL" \
      -DLLVM_ENABLE_PROJECTS="clang" \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_C_FLAGS="-O0 -fprofile-arcs -ftest-coverage" \
      -DCMAKE_CXX_FLAGS="-O0 -fprofile-arcs -ftest-coverage" \
      -DCMAKE_EXE_LINKER_FLAGS="-fprofile-arcs -ftest-coverage" \
      -DCMAKE_SHARED_LINKER_FLAGS="" \
      -DLLVM_ENABLE_ASSERTIONS=ON \
      -G "Ninja"

info "Building LLVM/Clang with $PROCS jobs (this will take ~2-3 hours)..."
ninja -C "$LLVM_BUILD" -j"$PROCS"
ninja -C "$LLVM_BUILD" install

info "✅ Instrumented Clang installed at $LLVM_INSTALL"
"$LLVM_INSTALL/bin/clang" --version
