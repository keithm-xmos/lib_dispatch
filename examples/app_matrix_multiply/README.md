# lib_dispatch Matrix Multiply Example

## Building & running tests for XCORE bare-metal

Run the following commands to build the test firmware:

    $ cmake -B build -DBARE_METAL=ON
    $ cmake --build build --target install
    $ xrun --xscope bin/mat_mul.xe

## Building & running tests for XCORE FreeRTOS

Run the following commands to build the test firmware:

    $ cmake -B build -DFREERTOS=ON
    $ cmake --build build --target install
    $ xrun --xscope bin/mat_mul.xe
