#!/usr/bin/bash

set -e

# run bare-metal build
rm -rf build
cmake -B build -DBARE_METAL=ON
cmake --build build --target install

# run host build and tests
rm -rf build
cmake -B build -DHOST=ON
cmake --build build --target install
./bin/lib_dispatch_tests -v
