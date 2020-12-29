# lib_dispatch Unit Tests

## XCORE

### Building Tests

Run the following commands to build the test firmware:

    $ mkdir build
    $ cd build
    $ cmake ../ -DXCORE=ON
    $ make install

### Running Tests

Run the following commands to run the test firmware:

    $ xrun --xscope bin/lib_dispatch_tests.xe

For more options:

    $ xrun --xscope --args bin/lib_dispatch_tests.xe -h

## x86

### Building Tests

    $ mkdir build
    $ cd build
    $ cmake ../ -DHOST=ON
    $ make install

### Running Tests

    S ./lib_dispatch_tests

## FreeRTOS

### Building Tests

    $ mkdir build
    $ cd build
    $ cmake ../ -DFREERTOS=ON
    $ make install

### Running Tests

Run the following commands to run the test firmware:

    $ xrun --xscope bin/lib_dispatch_tests.xe

For more options:

    $ xrun --xscope --args bin/lib_dispatch_tests.xe -h
