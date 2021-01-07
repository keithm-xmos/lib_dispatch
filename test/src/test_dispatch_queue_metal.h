// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef TEST_DISPATCH_QUEUE_XCORE_H_
#define TEST_DISPATCH_QUEUE_XCORE_H_

#include <platform.h>  // for PLATFORM_REFERENCE_MHZ
#include <xcore/hwtimer.h>
#include <xcore/lock.h>

#define QUEUE_LENGTH (3)
#define QUEUE_THREAD_COUNT (3)
#define QUEUE_THREAD_STACK_SIZE (1024)  // more than enough
#define QUEUE_THREAD_PRIORITY (0)       // ignored

typedef lock_t thread_mutex_t;

void look_busy(int milliseconds);
thread_mutex_t mutex_init();
void mutex_lock(thread_mutex_t lock);
void mutex_unlock(thread_mutex_t lock);
void mutex_destroy(thread_mutex_t lock);

inline void look_busy(int milliseconds) {
  uint32_t ticks = milliseconds * 1000 * PLATFORM_REFERENCE_MHZ;
  hwtimer_t timer = hwtimer_alloc();
  hwtimer_delay(timer, ticks);
  hwtimer_free(timer);
}

inline thread_mutex_t mutex_init() { return lock_alloc(); }

inline void mutex_lock(thread_mutex_t lock) { lock_acquire(lock); }

inline void mutex_unlock(thread_mutex_t lock) { lock_release(lock); }

inline void mutex_destroy(thread_mutex_t lock) { lock_free(lock); }

#endif  // TEST_DISPATCH_QUEUE_XCORE_H_
