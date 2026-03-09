#! /bin/bash -e

git clone --depth 1 https://github.com/bitwuzla/bitwuzla.git /bitwuzla

cd /bitwuzla
./configure.py
ninja -C build install

rm -rf /bitwuzla
