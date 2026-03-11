#! /bin/bash -e

git clone --depth 1 https://github.com/bitwuzla/bitwuzla.git /bitwuzla-src

cd /bitwuzla-src
./configure.py
ninja -C build install

rm -rf /bitwuzla-src
