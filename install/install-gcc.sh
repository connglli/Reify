#! /bin/bash -e

git clone -b releases/gcc-12.1.0 --depth 1 https://gcc.gnu.org/git/gcc.git /gcc12-src
cd /gcc12-src

contrib/download_prerequisites
contrib/gcc_build -d /gcc12-src \
    -o /gcc12-src/build \
    -c "--enable-multilib --enable-checking --disable-bootstrap --prefix=/gcc12" \
    configure
cd /gcc12-src/build
make -j128
make -j128 install

rm -rf /gcc12-src
