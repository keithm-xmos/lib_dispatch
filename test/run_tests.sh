#!/usr/bin/bash

set -e

# run freertos build and tests
rm -rf build
cmake -B build -DFREERTOS=ON
cmake --build build --target install
xrun --xscope --args bin/lib_dispatch_tests.xe -v

# run xcore build and tests
rm -rf build
cmake -B build -DXCORE=ON
cmake --build build --target install
xrun --xscope --args bin/lib_dispatch_tests.xe -v

# run host build and tests
rm -rf build
cmake -B build -DHOST=ON
cmake --build build --target install
./bin/lib_dispatch_tests -v
