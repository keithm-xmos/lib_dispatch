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

typedef struct test_producer_params {
  int iters;
  int max_delay;
  queue_t *queue;
} test_producer_params;

typedef struct test_consumer_params {
  bool shutdown_flag;
  int count;
  queue_t *queue;
} test_consumer_params;

static dispatch_lock_t lock;
static char thread_stack[THREAD_STACK_SIZE * 16];

DISPATCH_TASK_FUNCTION
void do_simple_producer(void *p) {
  // NOTE: the "volatile" is needed here or the compiler may optimize this away
  test_producer_params volatile *params = (test_producer_params volatile *)p;

  int *pushed = dispatch_malloc(sizeof(int) * params->iters);

  chanend_t cend = chanend_alloc();

  for (int i = 0; i < params->iters; i++) {
    pushed[i] = i;
    queue_send(params->queue, (void *)&pushed[i], cend);
  }

  dispatch_free(pushed);
  chanend_free(cend);
}

DISPATCH_TASK_FUNCTION
void do_random_producer(void *p) {
  // NOTE: the "volatile" is needed here or the compiler may optimize this away
  test_producer_params volatile *params = (test_producer_params volatile *)p;

  int *pushed = dispatch_malloc(sizeof(int) * params->iters);
  hwtimer_t timer = hwtimer_alloc();
  chanend_t cend = chanend_alloc();

  int delay;

  for (int i = 0; i < params->iters; i++) {
    pushed[i] = i;
    queue_send(params->queue, (void *)&pushed[i], cend);
    delay = rand() % params->max_delay;
    hwtimer_delay(timer, delay * 1000 * PLATFORM_REFERENCE_MHZ);
  }

  hwtimer_free(timer);
  chanend_free(cend);
  dispatch_free(pushed);
}

DISPATCH_TASK_FUNCTION
void do_counting_consumer(void *p) {
  // NOTE: the "volatile" is needed here or the compiler may optimize this away
  test_consumer_params volatile *params = (test_consumer_params volatile *)p;

  void *item = NULL;
  chanend_t cend = chanend_alloc();

  for (;;) {
    if (queue_receive(params->queue, &item, cend)) {
      // dispatch_lock_acquire(lock);
      params->count++;
      // dispatch_lock_release(lock);
    } else {
      // dispatch_lock_acquire(lock);
      params->shutdown_flag = true;
      // dispatch_lock_release(lock);
      chanend_free(cend);
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
  queue_t *queue = queue_create(kLength, lock);
  chanend_t cend = chanend_alloc();

  int pushed[n_items];
  for (int i = 0; i < n_items; i++) {
    pushed[i] = 0;
    queue_send(queue, &pushed[i], cend);
  }

  int *item;
  for (int i = 0; i < n_items; i++) {
    queue_receive(queue, (void *)&item, cend);
    TEST_ASSERT_EQUAL_INT(pushed[i], *item);
  }

  queue_destroy(queue, cend);
  chanend_free(cend);
}

TEST(queue_metal, test_under_capacity) {
  const size_t kLength = 10;
  size_t n_items = kLength - 1;
  queue_t *queue = queue_create(kLength, lock);
  chanend_t cend = chanend_alloc();

  int pushed[n_items];
  for (int i = 0; i < n_items; i++) {
    pushed[i] = i;
    queue_send(queue, &pushed[i], cend);
  }

  int *item;
  for (int i = 0; i < n_items; i++) {
    queue_receive(queue, (void *)&item, cend);
    TEST_ASSERT_EQUAL_INT(pushed[i], *item);
  }

  queue_destroy(queue, cend);
  chanend_free(cend);
}

TEST(queue_metal, test_fill_and_drain) {
  const size_t kLength = 10;
  const size_t kItems = 12;
  const size_t kIters = 3;
  queue_t *queue = queue_create(kLength, lock);
  chanend_t cend = chanend_alloc();

  // launch the producer thread
  test_producer_params thread_data;
  thread_data.iters = kItems;
  thread_data.queue = queue;

  for (int j = 0; j < kIters; j++) {
    dispatch_printf("test_fill_and_drain iter=%d\n", j);

    run_async(do_simple_producer, (void *)&thread_data,
              stack_base((void *)&thread_stack[0], THREAD_STACK_SIZE));

    // (busy)wait for the queue to fill up
    for (;;)
      if (queue_full(queue)) break;

    // consume in this thread
    int *item;
    for (int i = 0; i < kItems; i++) {
      queue_receive(queue, (void *)&item, cend);
      TEST_ASSERT_EQUAL_INT(i, *item);
    }
  }

  queue_destroy(queue, cend);
  chanend_free(cend);
}

TEST(queue_metal, test_random_arrival) {
  const size_t kLength = 10;
  const size_t kItems = 500;
  const size_t kMaxDelay = 50;
  queue_t *queue = queue_create(kLength, lock);
  chanend_t cend = chanend_alloc();

  // launch the producer thread
  test_producer_params thread_data;
  thread_data.iters = kItems;
  thread_data.max_delay = kMaxDelay;
  thread_data.queue = queue;
  run_async(do_random_producer, (void *)&thread_data,
            stack_base((void *)&thread_stack[0], THREAD_STACK_SIZE));

  hwtimer_t timer = hwtimer_alloc();
  int delay;

  // consume in this thread
  int *item;
  for (int i = 0; i < kItems; i++) {
    if (i % 100 == 0) dispatch_printf("test_random_arrival iter=%d\n", i);
    queue_receive(queue, (void *)&item, cend);
    TEST_ASSERT_EQUAL_INT(i, *item);
    delay = rand() % kMaxDelay;
    hwtimer_delay(timer, delay * 1000 * PLATFORM_REFERENCE_MHZ);
  }

  queue_destroy(queue, cend);

  hwtimer_free(timer);
  chanend_free(cend);
}

TEST(queue_metal, test_multiple_producers_and_consumers) {
  const size_t kLength = 10;
  const size_t kItems = 20;
  const size_t kMaxDelay = 50;
  const size_t kNumProducers = 2;
  const size_t kNumConsumers = 3;
  int stack_offset = 0;

  queue_t *queue = queue_create(kLength, lock);

  // launch producer threads
  test_producer_params producer_thread_data[kNumProducers];

  for (int i = 0; i < kNumProducers; i++) {
    producer_thread_data[i].iters = kItems;
    producer_thread_data[i].max_delay = kMaxDelay;
    producer_thread_data[i].queue = queue;
    run_async(
        do_random_producer, (void *)&producer_thread_data[i],
        stack_base((void *)&thread_stack[stack_offset], THREAD_STACK_SIZE));
    stack_offset += THREAD_STACK_SIZE * sizeof(int);
  }

  // launch consumer threads
  test_consumer_params consumer_thread_data[kNumConsumers];

  for (int i = 0; i < kNumConsumers; i++) {
    consumer_thread_data[i].count = 0;
    consumer_thread_data[i].shutdown_flag = false;
    consumer_thread_data[i].queue = queue;
    run_async(
        do_counting_consumer, (void *)&consumer_thread_data[i],
        stack_base((void *)&thread_stack[stack_offset], THREAD_STACK_SIZE));
    stack_offset += THREAD_STACK_SIZE * sizeof(int);
  }

  // need to give consumer threads time to receive all items
  hwtimer_t timer = hwtimer_alloc();
  const unsigned kMagicDuration = 100000000;
  hwtimer_delay(timer, kMagicDuration);
  hwtimer_free(timer);

  size_t total_produced = kNumProducers * kItems;
  size_t total_consumed = 0;
  for (int i = 0; i < kNumConsumers; i++) {
    total_consumed += consumer_thread_data[i].count;
  }

  TEST_ASSERT_EQUAL_INT(total_produced, total_consumed);

  chanend_t cend = chanend_alloc();
  queue_destroy(queue, cend);
  chanend_free(cend);
}

TEST(queue_metal, test_shutdown) {
  const size_t kLength = 10;
  const size_t kItems = 3;
  int items[kItems];
  queue_t *queue = queue_create(kLength, lock);
  chanend_t cend = chanend_alloc();
  hwtimer_t timer = hwtimer_alloc();
  const unsigned kMagicDuration = 10000000;

  // launch the consumer thread
  test_consumer_params thread_data;
  thread_data.count = 0;
  thread_data.shutdown_flag = false;
  thread_data.queue = queue;

  run_async(do_counting_consumer, (void *)&thread_data,
            stack_base((void *)&thread_stack[0], THREAD_STACK_SIZE));

  // NOTE: producer runs in this thread

  // add items to the queue
  for (int i = 0; i < kItems; i++) {
    items[i] = i;
    queue_send(queue, &items[i], cend);
  }

  // need to give consumer time to receive all items
  hwtimer_delay(timer, kMagicDuration);

  TEST_ASSERT_EQUAL_INT(kItems, thread_data.count);
  TEST_ASSERT_FALSE(thread_data.shutdown_flag);

  // shutdown the queue and make sure the consumer thread exited
  queue_destroy(queue, cend);

  // need to give consumer to exit
  hwtimer_delay(timer, kMagicDuration);

  TEST_ASSERT_TRUE(thread_data.shutdown_flag);

  chanend_free(cend);
  hwtimer_free(timer);
}

TEST_GROUP_RUNNER(queue_metal) {
  RUN_TEST_CASE(queue_metal, test_full_capacity);
  RUN_TEST_CASE(queue_metal, test_under_capacity);
  RUN_TEST_CASE(queue_metal, test_fill_and_drain);
  RUN_TEST_CASE(queue_metal, test_random_arrival);
  RUN_TEST_CASE(queue_metal, test_multiple_producers_and_consumers);
  RUN_TEST_CASE(queue_metal, test_shutdown);
}