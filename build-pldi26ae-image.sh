#! /bin/bash -e

rm -rf reify_pldi26ae_dockerimage.tar
docker build . -t reify:pldi26ae
docker save -o reify_pldi26ae_dockerimage.tar reify:pldi26ae
