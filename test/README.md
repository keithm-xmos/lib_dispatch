# lib_dispatch Unit Tests

## Building Tests for XCORE

Run the following commands to build the test firmware:

    $ mkdir build
    $ cd build
    $ cmake ../ -DXCORE=ON
    $ make install

## Building for FreeRTOS

    $ mkdir build
    $ cd build
    $ cmake ../ -DFREERTOS=ON
    $ make install

### Running Tests on XCORE

Run the following commands to run the test firmware:

    $ xrun --io bin/lib_dispatch_tests.xe

For more options:

    $ xrun --io --args bin/lib_dispatch_tests.xe -h

## Building for x86

    $ cmake ../ -DHOST=ON

### Running Tests on x86

    S ./lib_dispatch_tests
