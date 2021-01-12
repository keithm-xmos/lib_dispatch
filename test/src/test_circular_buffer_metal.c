// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include <platform.h>
#include <stdlib.h>
#include <string.h>
#include <xcore/hwtimer.h>
#include <xcore/thread.h>

#include "circular_buffer_metal.h"
#include "dispatch_config.h"
#include "dispatch_task.h"
#include "unity.h"
#include "unity_fixture.h"

#define THREAD_STACK_SIZE (1024)  // more than enough

typedef struct test_source_params {
  int iters;
  int max_delay;
  circular_buffer_t *cbuf;
} test_source_params;

static dispatch_lock_t lock;
static char thread_stack[THREAD_STACK_SIZE];

DISPATCH_TASK_FUNCTION
void do_simple_source(void *p) {
  // NOTE: the "volatile" is needed here or the compiler may optimize this away
  test_source_params volatile *params = (test_source_params volatile *)p;

  int *pushed = dispatch_malloc(sizeof(int) * params->iters);

  for (int i = 0; i < params->iters; i++) {
    pushed[i] = i;
    circular_buffer_push(params->cbuf, (void *)&pushed[i]);
  }

  dispatch_free(pushed);
}

DISPATCH_TASK_FUNCTION
void do_random_source(void *p) {
  // NOTE: the "volatile" is needed here or the compiler may optimize this away
  test_source_params volatile *params = (test_source_params volatile *)p;

  int *pushed = dispatch_malloc(sizeof(int) * params->iters);
  hwtimer_t timer = hwtimer_alloc();
  int delay;

  for (int i = 0; i < params->iters; i++) {
    pushed[i] = i;
    circular_buffer_push(params->cbuf, (void *)&pushed[i]);
    delay = rand() % params->max_delay;
    hwtimer_delay(timer, delay * 1000 * PLATFORM_REFERENCE_MHZ);
  }

  hwtimer_free(timer);
  dispatch_free(pushed);
}

TEST_GROUP(circular_buffer_metal);

TEST_SETUP(circular_buffer_metal) {
  srand(0);
  lock = dispatch_lock_alloc();
}

TEST_TEAR_DOWN(circular_buffer_metal) { dispatch_lock_free(lock); }

TEST(circular_buffer_metal, test_full_capacity) {
  const size_t kLength = 5;
  size_t n_items = kLength;
  circular_buffer_t *cbuf = circular_buffer_create(kLength, lock);

  int pushed[n_items];
  for (int i = 0; i < n_items; i++) {
    pushed[i] = 0;
    circular_buffer_push(cbuf, &pushed[i]);
  }

  for (int i = 0; i < n_items; i++) {
    int popped = *((int *)circular_buffer_pop(cbuf));
    TEST_ASSERT_EQUAL_INT(pushed[i], popped);
  }

  circular_buffer_destroy(cbuf);
}

TEST(circular_buffer_metal, test_under_capacity) {
  const size_t kLength = 10;
  size_t n_items = kLength - 1;
  circular_buffer_t *cbuf = circular_buffer_create(kLength, lock);

  int pushed[n_items];
  for (int i = 0; i < n_items; i++) {
    pushed[i] = i;
    circular_buffer_push(cbuf, &pushed[i]);
  }

  for (int i = 0; i < n_items; i++) {
    int popped = *((int *)circular_buffer_pop(cbuf));
    TEST_ASSERT_EQUAL_INT(pushed[i], popped);
  }

  circular_buffer_destroy(cbuf);
}

TEST(circular_buffer_metal, test_fill_drain) {
  const size_t kLength = 10;
  const size_t kItems = 12;
  const size_t kIters = 3;
  circular_buffer_t *cbuf = circular_buffer_create(kLength, lock);

  // launch the buffer source thread
  test_source_params thread_data;
  thread_data.iters = kItems;
  thread_data.cbuf = cbuf;

  for (int j = 0; j < kIters; j++) {
    dispatch_printf("test_fill_drain iter=%d\n", j);

    run_async(do_simple_source, (void *)&thread_data,
              stack_base((void *)&thread_stack[0], THREAD_STACK_SIZE));

    // busywait for it to fill up
    for (;;)
      if (circular_buffer_full(cbuf)) break;

    // drain the buffer sink in this thread
    for (int i = 0; i < kItems; i++) {
      int popped = *((int *)circular_buffer_pop(cbuf));
      TEST_ASSERT_EQUAL_INT(i, popped);
    }
  }

  circular_buffer_destroy(cbuf);
}

TEST(circular_buffer_metal, test_random_arrival) {
  const size_t kLength = 10;
  const size_t kItems = 500;
  const size_t kMaxDelay = 50;
  circular_buffer_t *cbuf = circular_buffer_create(kLength, lock);

  // launch the buffer source thread
  test_source_params thread_data;
  thread_data.iters = kItems;
  thread_data.max_delay = kMaxDelay;
  thread_data.cbuf = cbuf;
  run_async(do_random_source, (void *)&thread_data,
            stack_base((void *)&thread_stack[0], THREAD_STACK_SIZE));

  hwtimer_t timer = hwtimer_alloc();
  int delay;

  // implement the buffer sink in this thread
  for (int i = 0; i < kItems; i++) {
    if (i % 100 == 0) dispatch_printf("test_random_capacity iter=%d\n", i);
    int popped = *((int *)circular_buffer_pop(cbuf));
    TEST_ASSERT_EQUAL_INT(i, popped);
    delay = rand() % kMaxDelay;
    hwtimer_delay(timer, delay * 1000 * PLATFORM_REFERENCE_MHZ);
  }

  hwtimer_free(timer);
  circular_buffer_destroy(cbuf);
}

TEST_GROUP_RUNNER(circular_buffer_metal) {
  RUN_TEST_CASE(circular_buffer_metal, test_full_capacity);
  RUN_TEST_CASE(circular_buffer_metal, test_under_capacity);
  RUN_TEST_CASE(circular_buffer_metal, test_fill_drain);
  RUN_TEST_CASE(circular_buffer_metal, test_random_arrival);
}