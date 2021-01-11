// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef TEST_DISPATCH_QUEUE_HOST_H_
#define TEST_DISPATCH_QUEUE_HOST_H_

#include <time.h>

#include "dispatch_config.h"

#define QUEUE_LENGTH (10)
#define QUEUE_THREAD_COUNT (3)
#define QUEUE_THREAD_STACK_SIZE (0)  // ignored
#define QUEUE_THREAD_PRIORITY (0)    // ignored

void look_busy(int milliseconds);

inline void look_busy(int milliseconds) {
  int ms_remaining = milliseconds % 1000;
  long usec = ms_remaining * 1000;
  struct timespec ts_sleep;

  ts_sleep.tv_sec = milliseconds / 1000;
  ts_sleep.tv_nsec = 1000 * usec;
  nanosleep(&ts_sleep, NULL);
}

#endif  // TEST_DISPATCH_QUEUE_HOST_H_
