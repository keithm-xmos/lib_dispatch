// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef TEST_DISPATCH_QUEUE_HOST_H_
#define TEST_DISPATCH_QUEUE_HOST_H_

#include <pthread.h>
#include <time.h>

#include "dispatch_config.h"

#define QUEUE_LENGTH (10)
#define QUEUE_THREAD_COUNT (3)
#define QUEUE_THREAD_STACK_SIZE (0)  // ignored
#define QUEUE_THREAD_PRIORITY (0)    // ignored

typedef pthread_mutex_t *thread_mutex_t;

void look_busy(int milliseconds);
thread_mutex_t mutex_init();
void mutex_lock(thread_mutex_t lock);
void mutex_unlock(thread_mutex_t lock);
void mutex_destroy(thread_mutex_t lock);

inline void look_busy(int milliseconds) {
  int ms_remaining = milliseconds % 1000;
  long usec = ms_remaining * 1000;
  struct timespec ts_sleep;

  ts_sleep.tv_sec = milliseconds / 1000;
  ts_sleep.tv_nsec = 1000 * usec;
  nanosleep(&ts_sleep, NULL);
}

inline thread_mutex_t mutex_init() {
  pthread_mutex_t *mutex = dispatch_malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex, NULL);
  return mutex;
}

inline void mutex_lock(thread_mutex_t lock) { pthread_mutex_lock(lock); }

inline void mutex_unlock(thread_mutex_t lock) { pthread_mutex_unlock(lock); }

inline void mutex_destroy(thread_mutex_t lock) {
  pthread_mutex_destroy(lock);
  dispatch_free(lock);
}

#endif  // TEST_DISPATCH_QUEUE_HOST_H_
