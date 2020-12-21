// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include <string.h>
#include <xcore/hwtimer.h>

#include "lib_dispatch/api/dispatch.h"
#include "lib_dispatch/api/dispatch_xcore.h"
#include "unity.h"
#include "unity_fixture.h"

#define TEST_STATIC_LENGTH (3)
#define TEST_STATIC_THREAD_COUNT (3)
#define DISPATCHER_STACK_SIZE (1024)  // more than enough

typedef struct test_work_arg {
  int count;
} test_work_arg_t;

DISPATCH_TASK_FUNCTION
void do_dispatch_xcore_work(void *p) {
  test_work_arg_t *arg = (test_work_arg_t *)p;

  // keep busy doing stuff
  unsigned magic_duration = 100000000;
  hwtimer_t timer = hwtimer_alloc();
  unsigned time = hwtimer_get_time(timer);
  hwtimer_wait_until(timer, time + magic_duration);
  hwtimer_free(timer);

  arg->count++;
}

TEST_GROUP(dispatch_xcore);

TEST_SETUP(dispatch_xcore) {}

TEST_TEAR_DOWN(dispatch_xcore) {}

TEST(dispatch_xcore, test_async) {
  dispatch_queue_t *queue;
  dispatch_task_t task;
  test_work_arg_t arg;
  int length = 3;
  int thread_count = 3;
  int task_count = 8;

  queue = dispatch_queue_create(length, thread_count, DISPATCHER_STACK_SIZE,
                                "test_async");
  dispatch_task_init(&task, do_dispatch_xcore_work, &arg, "test_async");

  arg.count = 0;
  for (int i = 0; i < task_count; i++) {
    dispatch_queue_async(queue, &task);
  }
  dispatch_queue_wait(queue);

  TEST_ASSERT_EQUAL_INT(task_count, arg.count);

  dispatch_queue_destroy(queue);
}

TEST(dispatch_xcore, test_for) {
  dispatch_queue_t *queue;
  dispatch_task_t task;
  test_work_arg_t arg;
  int length = 3;
  int thread_count = 3;
  int for_count = 5;

  queue = dispatch_queue_create(length, thread_count, DISPATCHER_STACK_SIZE,
                                "test_for");
  dispatch_task_init(&task, do_dispatch_xcore_work, &arg, "test_for");

  arg.count = 0;
  dispatch_queue_for(queue, for_count, &task);
  dispatch_queue_wait(queue);

  TEST_ASSERT_EQUAL_INT(for_count, arg.count);

  dispatch_queue_destroy(queue);
}

TEST(dispatch_xcore, test_sync) {
  dispatch_queue_t *queue;
  dispatch_task_t task;
  test_work_arg_t arg;

  int length = 3;
  int thread_count = 3;
  int task_count = 4;

  queue = dispatch_queue_create(length, thread_count, DISPATCHER_STACK_SIZE,
                                "test_sync");
  dispatch_task_init(&task, do_dispatch_xcore_work, &arg, "test_sync");

  arg.count = 0;
  for (int i = 0; i < task_count; i++) {
    dispatch_queue_sync(queue, &task);
    TEST_ASSERT_EQUAL_INT(i + 1, arg.count);
  }

  dispatch_queue_destroy(queue);
}

TEST(dispatch_xcore, test_static) {
  // create queue with data on the stack
  dispatch_xcore_t queue_s;
  dispatch_thread_task_t thread_tasks[TEST_STATIC_THREAD_COUNT];
  dispatch_thread_data_t thread_data[TEST_STATIC_THREAD_COUNT];
  chanend_t chanends[TEST_STATIC_THREAD_COUNT];
  __attribute__((aligned(8))) static char
      static_stack[DISPATCHER_STACK_SIZE * TEST_STATIC_THREAD_COUNT];

  queue_s.length = TEST_STATIC_LENGTH;
  queue_s.thread_count = TEST_STATIC_THREAD_COUNT;
  queue_s.thread_chanends = &chanends[0];
  queue_s.thread_stack_size = DISPATCHER_STACK_SIZE;
  queue_s.thread_stack = static_stack;
  queue_s.thread_tasks = &thread_tasks[0];
  queue_s.thread_data = &thread_data[0];
#if DEBUG_PRINT_ENABLE
  strncpy(queue_s.name, "test_static", 32);
#endif

  dispatch_queue_t *queue = &queue_s;
  dispatch_queue_init(queue);

  // now use the static queue
  dispatch_task_t task;
  test_work_arg_t arg;
  int task_count = 4;

  arg.count = 0;
  dispatch_task_init(&task, do_dispatch_xcore_work, &arg, "test_static");
  for (int i = 0; i < task_count; i++) {
    dispatch_queue_async(queue, &task);
  }
  dispatch_queue_wait(queue);

  TEST_ASSERT_EQUAL_INT(task_count, arg.count);
}

TEST_GROUP_RUNNER(dispatch_xcore) {
  RUN_TEST_CASE(dispatch_xcore, test_static);
  RUN_TEST_CASE(dispatch_xcore, test_sync);
  RUN_TEST_CASE(dispatch_xcore, test_async);
  RUN_TEST_CASE(dispatch_xcore, test_for);
}