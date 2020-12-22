#!/usr/bin/bash

set -e

mkdir -p build
cd build

# run freertos build and tests
rm -rf *
cmake ../ -DFREERTOS=ON
make
xrun --io --args lib_dispatch_tests.xe -v

# run xcore build and tests
rm -rf *
cmake ../ -DXCORE=ON
make
xrun --io --args lib_dispatch_tests.xe -v

# run host build and tests
rm -rf *
cmake ../ -DHOST=ON
make
./lib_dispatch_tests -v

cd ..