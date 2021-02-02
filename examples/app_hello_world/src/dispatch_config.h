// Copyright 2021 XMOS LIMITED. This Software is subject to the terms of the 
// XMOS Public License: Version 1
#ifndef DISPATCH_CONFIG_H_
#define DISPATCH_CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <xcore/assert.h>
#include <xcore/lock.h>

#include "spinlock.h"

typedef lock_t dispatch_mutex_t;
typedef spinlock_t* dispatch_spinlock_t;

#define dispatch_assert(A) xassert(A)

#define dispatch_malloc(A) malloc(A)
#define dispatch_free(A) free(A)

#define dispatch_printf(...)  // printf(__VA_ARGS__)

// Mutex functions
static inline dispatch_mutex_t dispatch_mutex_create() { return lock_alloc(); }
static inline void dispatch_mutex_get(dispatch_mutex_t mutex) {
  lock_acquire(mutex);
}
static inline void dispatch_mutex_put(dispatch_mutex_t mutex) {
  lock_release(mutex);
}
static inline void dispatch_mutex_delete(dispatch_mutex_t mutex) {
  lock_free(mutex);
}

// Spinlock functions
static inline dispatch_spinlock_t dispatch_spinlock_create() {
  return spinlock_create();
}
static inline void dispatch_spinlock_get(dispatch_spinlock_t lock) {
  spinlock_acquire(lock);
}
static inline void dispatch_spinlock_put(dispatch_spinlock_t lock) {
  spinlock_release(lock);
}
static inline void dispatch_spinlock_delete(dispatch_spinlock_t lock) {
  spinlock_delete(lock);
}

#endif  // DISPATCH_CONFIG_H_