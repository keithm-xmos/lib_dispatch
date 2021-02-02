// Copyright 2021 XMOS LIMITED. This Software is subject to the terms of the 
// XMOS Public License: Version 1
#ifndef TEST_DISPATCH_QUEUE_XCORE_H_
#define TEST_DISPATCH_QUEUE_XCORE_H_

#include <platform.h>  // for PLATFORM_REFERENCE_MHZ
#include <xcore/hwtimer.h>

#define QUEUE_LENGTH (5)
#define QUEUE_THREAD_COUNT (3)
#define QUEUE_THREAD_STACK_SIZE (1024)  // more than enough
#define QUEUE_THREAD_PRIORITY (0)       // ignored

void look_busy(int milliseconds);

inline void look_busy(int milliseconds) {
  uint32_t ticks = milliseconds * 1000 * PLATFORM_REFERENCE_MHZ;
  hwtimer_t timer = hwtimer_alloc();
  hwtimer_delay(timer, ticks);
  hwtimer_free(timer);
}

#endif  // TEST_DISPATCH_QUEUE_XCORE_H_
