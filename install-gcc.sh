#!/bin/bash -e

git clone --depth 1 https://gcc.gnu.org/git/gcc.git /gcc-trunk
cd /gcc-trunk

contrib/download_prerequisites
contrib/gcc_build -d /gcc-trunk \
    -o /gcc-trunk/build \
    -c "--enable-multilib --enable-checking --disable-bootstrap --prefix=/gcctk" \
    configure
cd /gcc-trunk/build
make -j128
make -j128 install

rm -rf /gcc-trunk
