#!/usr/bin/env bash
set -euo pipefail

ROOT="/reify/previous_work"
SRC_DIR="$ROOT/src"
INSTALL_DIR="$ROOT/install"

YARPGEN_REPO="https://github.com/intel/yarpgen.git"
CSMITH_REPO="https://github.com/csmith-project/csmith.git"

mkdir -p "$SRC_DIR" "$INSTALL_DIR"

########################################
# helper: clone or update
########################################
clone_or_update() {
    local repo_url=$1
    local name=$2

    if [ -d "$SRC_DIR/$name/.git" ]; then
        echo "[*] Updating $name"
        cd "$SRC_DIR/$name"
        git pull --ff-only
    else
        echo "[*] Cloning $name"
        git clone "$repo_url" "$SRC_DIR/$name"
    fi
}

########################################
# build yarpgen
########################################
build_yarpgen() {
    local name="yarpgen"
    local build_dir="$SRC_DIR/$name/build"

    echo "[*] Building yarpgen"

    mkdir -p "$build_dir"
    cd "$build_dir"

    cmake -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" ..
    make -j$(nproc)
}

########################################
# build csmith
########################################
build_csmith() {
    local name="csmith"
    local build_dir="$SRC_DIR/$name/build"

    echo "[*] Building csmith"

    mkdir -p "$build_dir"
    cd "$build_dir"

    cmake -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" ..
    make -j$(nproc)
    make install
}

########################################
# main
########################################
clone_or_update "$YARPGEN_REPO" "yarpgen"
clone_or_update "$CSMITH_REPO" "csmith"

build_yarpgen
build_csmith

echo "[✓] Everything installed to: $INSTALL_DIR"
echo "[✓] Add to PATH:"
echo "    export PATH=$INSTALL_DIR/bin:\$PATH"
#!/usr/bin/env bash
set -euo pipefail

ROOT="/reify/previous_work"
SRC_DIR="$ROOT/src"
INSTALL_DIR="$ROOT/install"

YARPGEN_REPO="https://github.com/intel/yarpgen.git"
CSMITH_REPO="https://github.com/csmith-project/csmith.git"

mkdir -p "$SRC_DIR" "$INSTALL_DIR"

########################################
# helper: clone or update
########################################
clone_or_update() {
    local repo_url=$1
    local name=$2

    if [ -d "$SRC_DIR/$name/.git" ]; then
        echo "[*] Updating $name"
        cd "$SRC_DIR/$name"
        git pull --ff-only
    else
        echo "[*] Cloning $name"
        git clone "$repo_url" "$SRC_DIR/$name"
    fi
}

########################################
# build yarpgen
########################################
build_yarpgen() {
    local name="yarpgen"
    local build_dir="$SRC_DIR/$name/build"

    echo "[*] Building yarpgen"

    mkdir -p "$build_dir"
    cd "$build_dir"

    cmake -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" ..
    make -j$(nproc)
}

########################################
# build csmith
########################################
build_csmith() {
    local name="csmith"
    local build_dir="$SRC_DIR/$name/build"

    echo "[*] Building csmith"

    mkdir -p "$build_dir"
    cd "$build_dir"

    cmake -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" ..
    make -j$(nproc)
    make install
}

########################################
# main
########################################
clone_or_update "$YARPGEN_REPO" "yarpgen"
clone_or_update "$CSMITH_REPO" "csmith"

build_yarpgen
build_csmith

echo "[✓] Everything installed to: $INSTALL_DIR"
echo "[✓] Add to PATH:"
echo "    export PATH=$INSTALL_DIR/bin:\$PATH"

