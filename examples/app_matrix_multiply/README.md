# lib_dispatch Matrix Multiply Example

This example application demonstrates how to create a dispatch queue to parallelize the multiplication of two matrices.

## Building & running the application

Run the following commands to build and run the mat_mul firmware:

    $ cmake -B build
    $ cmake --build build --target install
    $ xrun --xscope bin/mat_mul.xe

