lib_dispatch
============

Library Info
############

Summary
-------

``lib_dispatch`` is an implementation of a dispatch queue for xCORE.

The xCORE multi-core architecture is well-suited for programmers familiar with concurrent programming models. However, programming using threads introduces complexity into your code. Often, you need to run tasks asynchronously without blocking the primary execution flow. With threads, you have to write code both for the task you want to perform and for the creation and management of the threads themselves. In a dispatch queue model, the function can simply be added to a work queue. The job of managing thread resources is simplified and localized to the dispatch queue library, allowing you to reduce your overall complexity. ``lib_dispatch`` lets you focus on the work you actually want to perform without having to worry about the thread creation and management.

Hello World Example
-------------------

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

Concepts
--------

**Tasks** are the basic unit of work that can be added to a dispatch queue. A task is a function with the following signature:

.. code:: console

    my_function(void *arg)

Task can be created using the dispatch task API defined in `dispatch_task.h <lib_dispatch/api/dispatch_task.h>`__. Additionaly, task can be created by the dispatch queue when you call the `dispatch_queue_function_add` API function. Tasks can be created with a **waitable** property and the dispatch queue has methods to wait for the completion of a task. Waiting on a task blocks in the callers thread until the dispatch queue is finished executing the specified task. 

**Groups** are containers for one or more tasks and can be added to the dispatch queue as a single, logical unit. Groups can be created and managed with the API functions defined in `dispatch_group.h <lib_dispatch/api/dispatch_group.h>`__. Like tasks, groups also have a waitable property. The dispatch queue has a method to wait for all of the tasks in a group to finish executing.

.. warning::

    Waitable tasks and groups use additional system resources. These resources are created when you add a waitable object to the dispatch queue and are freed when the caller waits on the object. However, you must wait on the task to free the resurces. Do not create waitable tasks or groups that are not intended to be waited on.

The dispatch queue API is defined in `dispatch_queue.h <lib_dispatch/api/dispatch_queue.h>`__. The tasks and groups added to a dispatch queue are executed in FIFO order by **worker** threads that are created and managed by the dispatch queue. The number of worker threads is specified by the caller when creating the dispatch queue. These workers wait for tasks to be added queue, take that work, and run the task's function in the worker's thread. If the task is waitable, the worker thread will signal that the task is complete. This will notify any current or future calls to the dispatch queue wait API functions. 

When creating the dispatch queue, the caller also specifies the **length** which is the maximum number of tasks that can be added to the dispatch queue. If the dispatch queue is full, attempting to add an additional task will block the caller's thread until a task is removed by a worker freeing up a position for the new task.

Implementations
---------------

Three implementations of the dispatch queue API are provided; bare-metal, FreeRTOS and x86. 

The bare-metal implementation uses bare-metal threads for workers and hardware resources to manage the worker threads and waitable objects. One hardware thread is allocated per worker and these hardware threads run on physical cores that can not be used for other tasks until the dispatch queue is destroyed. Details on the hardware resources used, see the Resource Usage section below.

The FreeRTOS implementation uses FreeRTOS threads for workers and uses only a subset of the hardware resources allocated to the RTOS. All resources used to manage the workers and waitable tasks are FreeRTOS concepts. When a worker is executing a task, the FreeRTOS scheduler will allocate it to a physical core. However, that physical core can be utilized to run other FreeRTOS threads if the dispatch queue worker threads are waiting for new tasks.

.. note::

For both the bare-metal and FreeRTOS implementations, all worker threads MUST be placed on the same tile.

The x86 implementation is intended for testing development only. If writing applications or libraries that can compile for the host PC, the x86 implementation provides a way for you to test your application logic without running on hardware. It is not intended to be used as a dispatch queue in applications that will not eventually run on hardware.

More Advanced Examples
----------------------

The "Hello World" example presented above is located in the `app_hello_world <examples/app_hello_world>`_ example folder. See the `README.md <examples/app_hello_world/README.md>`__ for instructions on building and running the example.

A more advanced example is located in the `app_matrix_multiply <examples/app_matrix_multiply>`__ example folder. Matrix multiplication is a data parallel operation. This means the input matrices can be partitioned and the multiplication operation run on the individual partitions in parallel. A dispatch queue is well suited for data parallel problems. 

.. note::

    The function used in this example to multiply two matrices is for illustrative use only. It is not the most efficient way to perform a matrix multiplication. XMOS has optimized libraries specifically for this purpose.

See the `README.md <examples/app_matrix_multiply/README.md>`__ for instructions on building and running the example.

Resource Usage
--------------

The bare-metal implementation uses the following hardware resources:

- N+1 chanends, where N equals the numebr of worker threads
- 1 hardware thread per worker
- 2 additional chanends for every waitable task or group. These 2 additional chanends are freed when the waitable task completes.

Using the library
#################

Some dependent components are included as git submodules. These can be obtained by cloning this repository with the following command:

.. code:: console

    git clone --recurse-submodules git@github.com:xmos/lib_dispatch.git

The AIoT SDK is required to build and run the FreeRTOS example applications or unit tests.  Follow the instructions in the `AIoT SDK Getting Started Guide <https://github.com/xmos/aiot_sdk/blob/develop/documents/quick_start/getting-started.rst>`__ to setup the SDK. 

**This remainder of the section is intentionally left blank.**
