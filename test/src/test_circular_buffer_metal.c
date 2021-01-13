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

typedef struct test_sink_params {
  bool shutdown_flag;
  int count;
  circular_buffer_t *cbuf;
} test_sink_params;

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

DISPATCH_TASK_FUNCTION
void do_counting_sink(void *p) {
  // NOTE: the "volatile" is needed here or the compiler may optimize this away
  test_sink_params volatile *params = (test_sink_params volatile *)p;

  void *item = NULL;
  for (;;) {
    if (circular_buffer_pop(params->cbuf, &item)) {
      dispatch_lock_acquire(lock);
      params->count++;
      dispatch_lock_release(lock);
    } else {
      dispatch_lock_acquire(lock);
      params->shutdown_flag = true;
      dispatch_lock_release(lock);
      break;
    }
  }
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

  int *popped;
  for (int i = 0; i < n_items; i++) {
    circular_buffer_pop(cbuf, (void *)&popped);
    TEST_ASSERT_EQUAL_INT(pushed[i], *popped);
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

  int *popped;
  for (int i = 0; i < n_items; i++) {
    circular_buffer_pop(cbuf, (void *)&popped);
    TEST_ASSERT_EQUAL_INT(pushed[i], *popped);
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
    int *popped;
    for (int i = 0; i < kItems; i++) {
      circular_buffer_pop(cbuf, (void *)&popped);
      TEST_ASSERT_EQUAL_INT(i, *popped);
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
  int *popped;
  for (int i = 0; i < kItems; i++) {
    if (i % 100 == 0) dispatch_printf("test_random_capacity iter=%d\n", i);
    circular_buffer_pop(cbuf, (void *)&popped);
    TEST_ASSERT_EQUAL_INT(i, *popped);
    delay = rand() % kMaxDelay;
    hwtimer_delay(timer, delay * 1000 * PLATFORM_REFERENCE_MHZ);
  }

  hwtimer_free(timer);
  circular_buffer_destroy(cbuf);
}

TEST(circular_buffer_metal, test_shutdown) {
  const size_t kLength = 10;
  const size_t kItems = 3;
  int items[kItems];
  circular_buffer_t *cbuf = circular_buffer_create(kLength, lock);
  hwtimer_t timer = hwtimer_alloc();
  const unsigned kMagicDuration = 10000000;

  // launch the buffer sink thread
  test_sink_params thread_data;
  thread_data.count = 0;
  thread_data.shutdown_flag = false;
  thread_data.cbuf = cbuf;

  run_async(do_counting_sink, (void *)&thread_data,
            stack_base((void *)&thread_stack[0], THREAD_STACK_SIZE));

  // add items to the queue
  for (int i = 0; i < kItems; i++) {
    items[i] = i;
    circular_buffer_push(cbuf, &items[i]);
  }

  // for (;;) {
  //   dispatch_printf("%d\n", thread_data.count);
  //   if (thread_data.count == kItems) break;
  // }

  // need to give sink time to pop all items
  hwtimer_delay(timer, kMagicDuration);

  TEST_ASSERT_EQUAL_INT(kItems, thread_data.count);
  TEST_ASSERT_FALSE(thread_data.shutdown_flag);

  // shutdown the circular buffer and make sure the counting sink thread exited
  circular_buffer_destroy(cbuf);

  // need to give sink time to exit
  hwtimer_delay(timer, kMagicDuration);

  // TODO: ensure it never arrived
  TEST_ASSERT_TRUE(thread_data.shutdown_flag);

  hwtimer_free(timer);
}

TEST_GROUP_RUNNER(circular_buffer_metal) {
  RUN_TEST_CASE(circular_buffer_metal, test_shutdown);
  RUN_TEST_CASE(circular_buffer_metal, test_full_capacity);
  RUN_TEST_CASE(circular_buffer_metal, test_under_capacity);
  RUN_TEST_CASE(circular_buffer_metal, test_fill_drain);
  RUN_TEST_CASE(circular_buffer_metal, test_random_arrival);
}