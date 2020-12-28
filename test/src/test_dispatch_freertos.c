// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include <string.h>

#include "lib_dispatch/api/dispatch.h"
#include "lib_dispatch/api/dispatch_queue_freertos.h"
#include "test_dispatch_queue.h"
#include "unity.h"
#include "unity_fixture.h"

#define TEST_STATIC_LENGTH (3)
#define TEST_STATIC_THREAD_COUNT (3)
#define DISPATCHER_STACK_SIZE (1024)  // more than enough

TEST_GROUP(dispatch_queue_freertos);

TEST_SETUP(dispatch_queue_freertos) {}

TEST_TEAR_DOWN(dispatch_queue_freertos) {}

TEST(dispatch_queue_freertos, test_static) {
  //   // create queue with data on the stack
  //   dispatch_xcore_queue_t queue_s;
  //   dispatch_thread_task_t thread_tasks[TEST_STATIC_THREAD_COUNT];
  //   dispatch_thread_data_t thread_data[TEST_STATIC_THREAD_COUNT];
  //   chanend_t chanends[TEST_STATIC_THREAD_COUNT];
  //   __attribute__((aligned(8))) static char
  //       static_stack[DISPATCHER_STACK_SIZE * TEST_STATIC_THREAD_COUNT];

  //   queue_s.length = TEST_STATIC_LENGTH;
  //   queue_s.thread_count = TEST_STATIC_THREAD_COUNT;
  //   queue_s.thread_chanends = &chanends[0];
  //   queue_s.thread_stack_size = DISPATCHER_STACK_SIZE;
  //   queue_s.thread_stack = static_stack;
  //   queue_s.thread_tasks = &thread_tasks[0];
  //   queue_s.thread_data = &thread_data[0];
  // #if DEBUG_PRINT_ENABLE
  //   strncpy(queue_s.name, "test_static", 32);
  // #endif

  //   dispatch_queue_t *queue = &queue_s;
  //   dispatch_queue_init(queue);

  //   // now use the static queue
  //   dispatch_task_t task;
  //   test_work_arg_t arg;
  //   int task_count = 4;

  //   arg.count = 0;
  //   dispatch_task_init(&task, do_dispatch_queue_freertos_work, &arg,
  //   "test_static"); for (int i = 0; i < task_count; i++) {
  //     dispatch_queue_async_task(queue, &task);
  //   }
  //   dispatch_queue_wait(queue);

  //   TEST_ASSERT_EQUAL_INT(task_count, arg.count);
  TEST_ASSERT_EQUAL_INT(1, 2);
}

TEST_GROUP_RUNNER(dispatch_queue_freertos) {
  RUN_TEST_CASE(dispatch_queue_freertos, test_static);
}