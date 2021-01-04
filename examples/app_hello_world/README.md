# lib_dispatch Hello World Example

This example application demonstrates how to create a dispatch queue and add functions that print out "Hello World".

## Building & running for FreeRTOS

Run the following commands to build and run the hello_world firmware:

    $ cmake -B build -DFREERTOS=ON
    $ cmake --build build --target install
    $ xrun --xscope bin/hello_world.xe

## Building & running for bare-metal

Run the following commands to build and run the hello_world firmware:

    $ cmake -B build -DBARE_METAL=ON
    $ cmake --build build --target install
    $ xrun --xscope bin/hello_world.xe
