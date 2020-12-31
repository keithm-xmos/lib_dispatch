# lib_dispatch Unit Tests

## Building & running tests for XCORE bare-metal

Run the following commands to build the test firmware:

    $ cmake -B build -DBARE_METAL=ON
    $ cmake --build build --target install
    $ xrun --xscope --args bin/lib_dispatch_tests.xe -v

## Building & running tests for XCORE FreeRTOS

Run the following commands to build the test firmware:

    $ cmake -B build -DFREERTOS=ON
    $ cmake --build build --target install
    $ xrun --xscope --args bin/lib_dispatch_tests.xe -v

## Building & running tests for x86 host

Run the following commands to build the test firmware:

    $ cmake -B build -DHOST=ON
    $ cmake --build build --target install
    $ ./bin/lib_dispatch_tests -v

## For more unit test options

To run a single test group, run with the `-g` option.

    $ xrun --xscope --args bin/lib_dispatch_tests.xe -g {group name}

To run a single test, run with the `-g` and `-n` options.

    $ xrun --xscope --args bin/lib_dispatch_tests.xe -g {group name} -n {test name}

For more unit test options, run with the `-h` option.

    $ xrun --xscope --args bin/lib_dispatch_tests.xe -h
