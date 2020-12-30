// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef TEST_DISPATCH_QUEUE_XCORE_H_
#define TEST_DISPATCH_QUEUE_XCORE_H_

#include <xcore/hwtimer.h>

#define QUEUE_LENGTH (3)
#define QUEUE_THREAD_COUNT (3)
#define QUEUE_STACK_SIZE (1024)  // more than enough

typedef int thread_mutex_t;

void keep_busy();
void mutex_init(thread_mutex_t *lock);
void mutex_lock(thread_mutex_t *lock);
void mutex_unlock(thread_mutex_t *lock);
void mutex_destroy(thread_mutex_t *lock);

inline void keep_busy() {
  unsigned magic_duration = 100000000;
  hwtimer_t timer = hwtimer_alloc();
  unsigned time = hwtimer_get_time(timer);
  hwtimer_wait_until(timer, time + magic_duration);
  hwtimer_free(timer);
}

inline void mutex_init(thread_mutex_t *lock) {}

inline void mutex_lock(thread_mutex_t *lock) {}

inline void mutex_unlock(thread_mutex_t *lock) {}

inline void mutex_destroy(thread_mutex_t *lock) {}

#endif  // TEST_DISPATCH_QUEUE_XCORE_H_
