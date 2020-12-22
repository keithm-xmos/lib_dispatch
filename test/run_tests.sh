#!/usr/bin/bash

set -e

# run host build and tests
cd build && rm -rf
cd build && cmake ../ -DHOST=ON
cd build && make
cd build && ./lib_dispatch_tests -v

# run xcore build and tests
cd build && rm -rf
cd build && cmake ../ -DXCORE=ON
cd build && make
cd build && xrun --io --args lib_dispatch_tests.xe -v