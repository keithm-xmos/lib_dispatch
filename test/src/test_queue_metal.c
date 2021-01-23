// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include <platform.h>
#include <stdlib.h>
#include <string.h>
#include <xcore/hwtimer.h>
#include <xcore/thread.h>

#include "dispatch_config.h"
#include "dispatch_task.h"
#include "queue_metal.h"
#include "unity.h"
#include "unity_fixture.h"

#define THREAD_STACK_SIZE (1024)  // more than enough

typedef struct test_source_params {
  int iters;
  int max_delay;
  queue_t *cbuf;
} test_source_params;

typedef struct test_sink_params {
  bool shutdown_flag;
  int count;
  queue_t *cbuf;
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
    queue_send(params->cbuf, (void *)&pushed[i]);
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
    queue_send(params->cbuf, (void *)&pushed[i]);
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
    if (queue_receive(params->cbuf, &item)) {
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

TEST_GROUP(queue_metal);

TEST_SETUP(queue_metal) {
  srand(0);
  lock = dispatch_lock_alloc();
}

TEST_TEAR_DOWN(queue_metal) { dispatch_lock_free(lock); }

TEST(queue_metal, test_full_capacity) {
  const size_t kLength = 5;
  size_t n_items = kLength;
  queue_t *cbuf = queue_create(kLength, lock);

  int pushed[n_items];
  for (int i = 0; i < n_items; i++) {
    pushed[i] = 0;
    queue_send(cbuf, &pushed[i]);
  }

  int *popped;
  for (int i = 0; i < n_items; i++) {
    queue_receive(cbuf, (void *)&popped);
    TEST_ASSERT_EQUAL_INT(pushed[i], *popped);
  }

  queue_destroy(cbuf);
}

TEST(queue_metal, test_under_capacity) {
  const size_t kLength = 10;
  size_t n_items = kLength - 1;
  queue_t *cbuf = queue_create(kLength, lock);

  int pushed[n_items];
  for (int i = 0; i < n_items; i++) {
    pushed[i] = i;
    queue_send(cbuf, &pushed[i]);
  }

  int *popped;
  for (int i = 0; i < n_items; i++) {
    queue_receive(cbuf, (void *)&popped);
    TEST_ASSERT_EQUAL_INT(pushed[i], *popped);
  }

  queue_destroy(cbuf);
}

TEST(queue_metal, test_fill_drain) {
  const size_t kLength = 10;
  const size_t kItems = 12;
  const size_t kIters = 3;
  queue_t *cbuf = queue_create(kLength, lock);

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
      if (queue_full(cbuf)) break;

    // drain the buffer sink in this thread
    int *popped;
    for (int i = 0; i < kItems; i++) {
      queue_receive(cbuf, (void *)&popped);
      TEST_ASSERT_EQUAL_INT(i, *popped);
    }
  }

  queue_destroy(cbuf);
}

TEST(queue_metal, test_random_arrival) {
  const size_t kLength = 10;
  const size_t kItems = 500;
  const size_t kMaxDelay = 50;
  queue_t *cbuf = queue_create(kLength, lock);

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
    queue_receive(cbuf, (void *)&popped);
    TEST_ASSERT_EQUAL_INT(i, *popped);
    delay = rand() % kMaxDelay;
    hwtimer_delay(timer, delay * 1000 * PLATFORM_REFERENCE_MHZ);
  }

  hwtimer_free(timer);
  queue_destroy(cbuf);
}

TEST(queue_metal, test_shutdown) {
  const size_t kLength = 10;
  const size_t kItems = 3;
  int items[kItems];
  queue_t *cbuf = queue_create(kLength, lock);
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
    queue_send(cbuf, &items[i]);
  }

  // need to give sink time to pop all items
  hwtimer_delay(timer, kMagicDuration);

  TEST_ASSERT_EQUAL_INT(kItems, thread_data.count);
  TEST_ASSERT_FALSE(thread_data.shutdown_flag);

  // shutdown the circular buffer and make sure the counting sink thread exited
  queue_destroy(cbuf);

  // need to give sink time to exit
  hwtimer_delay(timer, kMagicDuration);

  // TODO: ensure it never arrived
  TEST_ASSERT_TRUE(thread_data.shutdown_flag);

  hwtimer_free(timer);
}

TEST_GROUP_RUNNER(queue_metal) {
  RUN_TEST_CASE(queue_metal, test_shutdown);
  RUN_TEST_CASE(queue_metal, test_full_capacity);
  RUN_TEST_CASE(queue_metal, test_under_capacity);
  RUN_TEST_CASE(queue_metal, test_fill_drain);
  RUN_TEST_CASE(queue_metal, test_random_arrival);
}