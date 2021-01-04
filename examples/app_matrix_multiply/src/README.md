# lib_dispatch Matrix Multiply Example

This example application demonstrates how to create a dispatch queue, a dispatch group, and utilize them to parallelize the multiplication of two matrices.  The resulting performance is, as one might expect, four times faster using a dispatch queue with four worker threads.

Note, the function used in this example to multiply two matrices is for illustrative use only.  It is not the most efficient way to perform a matrix multiplication.  XMOS has optimized libraries specifically for this purpose.

## Building & running tests for FreeRTOS

Run the following commands to build the mat_mul firmware:

    $ cmake -B build -DFREERTOS=ON
    $ cmake --build build --target install
    $ xrun --xscope bin/mat_mul.xe

## Building & running tests for bare-metal

Run the following commands to build the mat_mul firmware:

    $ cmake -B build -DBARE_METAL=ON
    $ cmake --build build --target install
    $ xrun --xscope bin/mat_mul.xe

