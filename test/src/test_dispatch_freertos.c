// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include <string.h>
#include <xcore/hwtimer.h>

#include "FreeRTOSConfig.h"
#include "dispatch.h"
#include "test_dispatch_queue.h"
#include "unity.h"
#include "unity_fixture.h"

#define DISPATCHER_STACK_SIZE (1024)  // more than enough

TEST_GROUP(dispatch_queue_freertos);

TEST_SETUP(dispatch_queue_freertos) {}

TEST_TEAR_DOWN(dispatch_queue_freertos) {}

TEST(dispatch_queue_freertos, test_parallel) {
  const int kThreadCount = 4;
  int num_values = kThreadCount * 1000000;
  test_parallel_work_arg args[kThreadCount];
  hwtimer_t hwtimer;
  int single_thread_ticks;

  // do single thread timing
  args[0].count = 0;
  args[0].begin = 0;
  args[0].end = num_values;

  hwtimer = hwtimer_alloc();
  single_thread_ticks = hwtimer_get_time(hwtimer);

  // do the work in this (single) thread
  do_parallel_work(&args[0]);

  single_thread_ticks = hwtimer_get_time(hwtimer) - single_thread_ticks;
  hwtimer_free(hwtimer);

  TEST_ASSERT_EQUAL_INT(num_values, args[0].count);

  // do multi thread timing
  const int kQueueLength = 4;
  dispatch_queue_t* queue;
  dispatch_group_t* group;
  int num_values_in_chunk = num_values / kThreadCount;
  int multi_thread_ticks;

  // create the dispatch queue
  queue =
      dispatch_queue_create(kQueueLength, kThreadCount, DISPATCHER_STACK_SIZE,
                            (configMAX_PRIORITIES - 1));

  // create the dispatch group
  group = dispatch_group_create(kThreadCount, true);

  // initialize kThreadCount tasks, add them to the group
  for (int i = 0; i < kThreadCount; i++) {
    args[i].count = 0;
    args[i].begin = i * num_values_in_chunk;
    args[i].end = args[i].begin + num_values_in_chunk;
    dispatch_group_function_add(group, do_parallel_work, &args[i]);
  }

  hwtimer = hwtimer_alloc();
  multi_thread_ticks = hwtimer_get_time(hwtimer);

  // add group to dispatch queue
  dispatch_queue_group_add(queue, group);
  // wait for all tasks in the group to finish executing
  dispatch_queue_group_wait(queue, group);

  multi_thread_ticks = hwtimer_get_time(hwtimer) - multi_thread_ticks;
  hwtimer_free(hwtimer);

  for (int i = 0; i < kThreadCount; i++) {
    TEST_ASSERT_EQUAL_INT(num_values_in_chunk, args[i].count);
  }

  // destroy the dispatch group and queue
  dispatch_group_destroy(group);
  dispatch_queue_destroy(queue);

  // now test that the multi thread was ~kThreadCount times faster
  float speedup = (float)single_thread_ticks / (float)multi_thread_ticks;
  TEST_ASSERT_FLOAT_WITHIN(0.1, (float)kThreadCount, speedup);
}

TEST_GROUP_RUNNER(dispatch_queue_freertos) {
  RUN_TEST_CASE(dispatch_queue_freertos, test_parallel);
}