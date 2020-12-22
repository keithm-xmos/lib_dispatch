#!/usr/bin/bash

set -e

mkdir -p build

# run host build and tests
cd build
rm -rf *
cmake ../ -DHOST=ON
make
./lib_dispatch_tests -v

# run xcore build and tests
rm -rf *
cmake ../ -DXCORE=ON
make
xrun --io --args lib_dispatch_tests.xe -v
