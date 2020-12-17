// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include <xcore/hwtimer.h>

#include "lib_dispatch/api/dispatch.h"
#include "lib_dispatch/api/dispatch_xcore.h"
#include "unity.h"
#include "unity_fixture.h"

#define TEST_STATIC_LENGTH (3)
#define TEST_STATIC_THREAD_COUNT (3)
#define DISPATCHER_STACK_SIZE (1024)  // more than enough

typedef struct test_work_params {
  int count;
} test_work_params_t;

DISPATCH_TASK_FUNCTION
void do_dispatch_xcore_work(void *p) {
  test_work_params_t *params = (test_work_params_t *)p;

  // keep busy doing stuff
  unsigned magic_duration = 100000000;
  hwtimer_t timer = hwtimer_alloc();
  unsigned time = hwtimer_get_time(timer);
  hwtimer_wait_until(timer, time + magic_duration);
  hwtimer_free(timer);

  params->count++;
}

TEST_GROUP(dispatch_xcore);

TEST_SETUP(dispatch_xcore) {}

TEST_TEAR_DOWN(dispatch_xcore) {}

TEST(dispatch_xcore, test_async) {
  dispatch_handle_t *queue;
  dispatch_task_t task;
  test_work_params_t params;
  int length = 3;
  int thread_count = 3;
  int task_count = 8;

  queue = dispatch_queue_create(length, thread_count, DISPATCHER_STACK_SIZE,
                                "test_async");
  dispatch_task_create(&task, do_dispatch_xcore_work, &params, "test_async");

  params.count = 0;
  for (int i = 0; i < task_count; i++) {
    dispatch_queue_async(queue, &task);
  }
  dispatch_queue_wait(queue);

  TEST_ASSERT_EQUAL_INT(task_count, params.count);

  dispatch_queue_destroy(queue);
}

TEST(dispatch_xcore, test_for) {
  dispatch_handle_t *queue;
  dispatch_task_t task;
  test_work_params_t params;
  int length = 3;
  int thread_count = 3;
  int for_count = 5;

  queue = dispatch_queue_create(length, thread_count, DISPATCHER_STACK_SIZE,
                                "test_for");
  dispatch_task_create(&task, do_dispatch_xcore_work, &params, "test_for");

  params.count = 0;
  dispatch_queue_for(queue, for_count, &task);
  dispatch_queue_wait(queue);

  TEST_ASSERT_EQUAL_INT(for_count, params.count);

  dispatch_queue_destroy(queue);
}

TEST(dispatch_xcore, test_sync) {
  dispatch_handle_t *queue;
  dispatch_task_t task;
  test_work_params_t params;

  int length = 3;
  int thread_count = 3;
  int task_count = 4;

  queue = dispatch_queue_create(length, thread_count, DISPATCHER_STACK_SIZE,
                                "test_sync");
  dispatch_task_create(&task, do_dispatch_xcore_work, &params, "test_sync");

  params.count = 0;
  for (int i = 0; i < task_count; i++) {
    dispatch_queue_sync(queue, &task);
    TEST_ASSERT_EQUAL_INT(i + 1, params.count);
  }

  dispatch_queue_destroy(queue);
}

TEST(dispatch_xcore, test_static) {
  // create queue with data on the stack
  dispatch_xcore_t queue;
  volatile dispatch_worker_flag_t flags[TEST_STATIC_THREAD_COUNT];
  channel_t channels[TEST_STATIC_THREAD_COUNT];
  __attribute__((aligned(8))) static char
      static_stack[DISPATCHER_STACK_SIZE * TEST_STATIC_THREAD_COUNT];

  queue.length = TEST_STATIC_LENGTH;
  queue.thread_count = TEST_STATIC_THREAD_COUNT;
  queue.flags = &flags[0];
  queue.channels = &channels[0];
  queue.stack_size = DISPATCHER_STACK_SIZE;
  queue.stack = static_stack;
#if DEBUG_PRINT_ENABLE
  queue.name = "test_static";
#endif

  dispatch_handle_t handle = (dispatch_handle_t)&queue;
  dispatch_queue_init(handle);

  // now use the static queue
  dispatch_task_t task;
  test_work_params_t params;
  int task_count = 4;

  params.count = 0;
  dispatch_task_create(&task, do_dispatch_xcore_work, &params, "test_static");
  for (int i = 0; i < task_count; i++) {
    dispatch_queue_async(handle, &task);
  }
  dispatch_queue_wait(handle);

  TEST_ASSERT_EQUAL_INT(task_count, params.count);
}

TEST_GROUP_RUNNER(dispatch_xcore) {
  RUN_TEST_CASE(dispatch_xcore, test_static);
  RUN_TEST_CASE(dispatch_xcore, test_sync);
  RUN_TEST_CASE(dispatch_xcore, test_async);
  RUN_TEST_CASE(dispatch_xcore, test_for);
}