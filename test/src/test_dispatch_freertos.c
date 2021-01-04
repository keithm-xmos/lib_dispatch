// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include <string.h>
#include <xcore/hwtimer.h>

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
  // create queue with data on the stack
  dispatch_freertos_queue_t queue_s;
  size_t thread_task_ids[TEST_STATIC_THREAD_COUNT];
  dispatch_thread_data_t thread_data[TEST_STATIC_THREAD_COUNT];

  queue_s.length = TEST_STATIC_LENGTH;
  queue_s.thread_count = TEST_STATIC_THREAD_COUNT;
  queue_s.thread_stack_size = DISPATCHER_STACK_SIZE;
  queue_s.thread_task_ids = &thread_task_ids[0];
  queue_s.thread_data = &thread_data[0];
  queue_s.xQueue = xQueueCreate(TEST_STATIC_LENGTH, sizeof(dispatch_task_t));

#if DEBUG_PRINT_ENABLE
  strncpy(queue_s.name, "test_static", 32);
#endif

  dispatch_queue_t* queue = &queue_s;
  dispatch_queue_init(queue);

  // now use the static queue
  test_work_arg_t arg;
  int task_count = 4;

  arg.count = 0;
  for (int i = 0; i < task_count; i++) {
    dispatch_queue_add_function(queue, do_standard_work, &arg);
  }
  dispatch_queue_wait(queue);

  TEST_ASSERT_EQUAL_INT(task_count, arg.count);
}

TEST(dispatch_queue_freertos, test_parallel) {
  int thread_count = 4;
  int num_values = thread_count * 1000000;
  test_parallel_work_arg args[thread_count];
  hwtimer_t hwtimer;
  int single_thread_ticks;

  // do single thread timing
  args[0].count = 0;
  args[0].begin = 0;
  args[0].end = num_values;

  hwtimer = hwtimer_alloc();
  single_thread_ticks = hwtimer_get_time(hwtimer);

  do_parallel_work(&args[0]);

  single_thread_ticks = hwtimer_get_time(hwtimer) - single_thread_ticks;
  hwtimer_free(hwtimer);

  TEST_ASSERT_EQUAL_INT(num_values, args[0].count);

  // do multi thread timing
  int queue_length = 4;
  dispatch_queue_t* queue;
  dispatch_group_t* group;
  int num_values_in_chunk = num_values / thread_count;
  int multi_thread_ticks;

  // create the dispatch queue
  queue =
      dispatch_queue_create(queue_length, thread_count, 1024, "test_parallel");

  // create the dispatch group
  group = dispatch_group_create(thread_count);

  // initialize thread_count tasks, add them to the group
  for (int i = 0; i < thread_count; i++) {
    args[i].count = 0;
    args[i].begin = i * num_values_in_chunk;
    args[i].end = args[i].begin + num_values_in_chunk;
    dispatch_group_add(group, do_parallel_work, &args[i]);
  }

  hwtimer = hwtimer_alloc();
  multi_thread_ticks = hwtimer_get_time(hwtimer);

  // add group to dispatch queue
  dispatch_queue_add_group(queue, group);
  // wait for all tasks in the group to finish executing
  dispatch_group_wait(group);

  multi_thread_ticks = hwtimer_get_time(hwtimer) - multi_thread_ticks;
  hwtimer_free(hwtimer);

  for (int i = 0; i < thread_count; i++) {
    TEST_ASSERT_EQUAL_INT(num_values_in_chunk, args[i].count);
  }

  // destroy the dispatch group and queue
  dispatch_group_destroy(group);
  dispatch_queue_destroy(queue);

  // now test that the multi thread was ~thread_count times faster
  float speedup = (float)single_thread_ticks / (float)multi_thread_ticks;
  TEST_ASSERT_FLOAT_WITHIN(0.1, (float)thread_count, speedup);
}

TEST_GROUP_RUNNER(dispatch_queue_freertos) {
  RUN_TEST_CASE(dispatch_queue_freertos, test_static);
  RUN_TEST_CASE(dispatch_queue_freertos, test_parallel);
}