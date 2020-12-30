// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef TEST_DISPATCH_QUEUE_HOST_H_
#define TEST_DISPATCH_QUEUE_HOST_H_

#include <pthread.h>
#include <time.h>

#define QUEUE_LENGTH (10)
#define QUEUE_THREAD_COUNT (3)
#define QUEUE_STACK_SIZE (0)  // not necessary

typedef pthread_mutex_t thread_mutex_t;

void keep_busy();
void mutex_init(thread_mutex_t *lock);
void mutex_lock(thread_mutex_t *lock);
void mutex_unlock(thread_mutex_t *lock);
void mutex_destroy(thread_mutex_t *lock);

inline void keep_busy() {
  nanosleep((const struct timespec[]){{0, 1000 * 1000000000L}}, NULL);
}

inline void mutex_init(thread_mutex_t *lock) { pthread_mutex_init(lock, NULL); }

inline void mutex_lock(thread_mutex_t *lock) { pthread_mutex_lock(lock); }

inline void mutex_unlock(thread_mutex_t *lock) { pthread_mutex_unlock(lock); }

inline void mutex_destroy(thread_mutex_t *lock) { pthread_mutex_destroy(lock); }

#endif  // TEST_DISPATCH_QUEUE_HOST_H_
