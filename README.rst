lib_dispatch
============

Library Info
############

Summary
-------

``lib_dispatch`` is an implementation of a dispatch queue for xCORE.

The xCORE multi-core architecture is well-suited for programmers familiar with concurrent programming models.  However, programming using threads introduces complexity into your code.  Often, you need to run tasks asynchronously without blocking the primary execution flow. With threads, you have to write code both for the task you want to perform and for the creation and management of the threads themselves. In a dispatch model, the function can simply be added to a work queue. The job of managing thread resources is simplified and localized to the dispatch library, allowing you to reduce your overall complexity by reducing thread management overhead.  ``lib_dispatch`` lets you focus on the work you actually want to perform without having to worry about the thread creation and management.

Simple Example
--------------

Example function that simply prints "Hello World".

.. code:: c

    void do_work(void* arg) {
        printf("Hello World from task %d\n", *(int*)arg);
    }

Code to create the dispatch queue and add a function:

.. code:: c

    // create a dispatch queue with length 5 and 5 worker threads
    queue = dispatch_queue_create(5, 5, 1024, 0);

    // add 5 functions to the dispatch queue
    for (int i = 0; i < 5; i++) {
        dispatch_queue_function_add(queue, do_work, &i, false);
    }

    // wait for all functions to finish executing
    dispatch_queue_wait(queue);

    // destroy the dispatch queue
    dispatch_queue_destroy(queue);

Output from the code above:

.. code:: console

    Hello World from task 0
    Hello World from task 1
    Hello World from task 2
    Hello World from task 3
    Hello World from task 4



