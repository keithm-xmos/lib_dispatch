# lib_dispatch Hello World Example

This example application demonstrates how to create a dispatch queue and add functions that print out "Hello World".

## Building & running the application

Run the following commands to build and run the hello_world firmware:

    $ cmake -B build
    $ cmake --build build --target install
    $ xrun --xscope bin/hello_world.xe
