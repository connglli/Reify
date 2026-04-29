#!/bin/bash
# =============================================================================
# build_gcc_instrumented.sh
# Builds instrumented GCC from a specific commit, out-of-source.
# =============================================================================
set -euo pipefail

GCC_COMMIT="19167808a6403843af1ff74410b6f324d7c6304d"
GCC_REPO="https://gcc.gnu.org/git/gcc.git"
GCC_SRC="/reify/coverage/builds/gcc-src"
GCC_BUILD="/reify/coverage/builds/gcc-build"
GCC_INSTALL="/reify/coverage/builds/gcc-instrumented"

PROCS=$(nproc)
(( PROCS > 64 )) && PROCS=64

RED='\033[0;31m'; GREEN='\033[0;32m'; NC='\033[0m'
info() { echo -e "${GREEN}[INFO]${NC} $*"; }
die()  { echo -e "${RED}[ERROR]${NC} $*" >&2; exit 1; }


# ── clone & checkout ─────────────────────────────────────────────────────────
if [ ! -d "$GCC_SRC/.git" ]; then
    info "Cloning GCC (this may take a while)..."
    git clone "$GCC_REPO" "$GCC_SRC"
else
    info "GCC source already cloned, skipping."
fi

info "Checking out commit $GCC_COMMIT..."
cd "$GCC_SRC"
git fetch origin
git checkout "$GCC_COMMIT"

./contrib/download_prerequisites
# ── out-of-source build ───────────────────────────────────────────────────────
info "Configuring GCC (out-of-source build)..."
rm -rf "$GCC_BUILD"
mkdir -p "$GCC_BUILD" "$GCC_INSTALL"
cd "$GCC_BUILD"
"$GCC_SRC/configure" \
    --prefix="$GCC_INSTALL" \
    --enable-languages=c,c++ \
    --enable-multilib \
    --disable-bootstrap \
    CFLAGS="-O0 -fprofile-arcs -ftest-coverage" \
    CXXFLAGS="-O0 -fprofile-arcs -ftest-coverage" \
    LDFLAGS="-fprofile-arcs -ftest-coverage"

info "Building GCC with $PROCS jobs (this will take ~1-2 hours)..."
make -j"$PROCS"
make install -j"$PROCS"

info "✅ Instrumented GCC installed at $GCC_INSTALL"
"$GCC_INSTALL/bin/gcc" --version

