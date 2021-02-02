// Copyright 2021 XMOS LIMITED. This Software is subject to the terms of the 
// XMOS Public License: Version 1
#include "dispatch_config.h"
#include "spinlock.h"

#define SPINLOCK_INITIAL_VALUE 0

enum { SPINLOCK_NOT_ACQUIRED = 0 };

spinlock_t *spinlock_create() {
  spinlock_t *lock = dispatch_malloc(sizeof(spinlock_t));
  *lock = SPINLOCK_INITIAL_VALUE;

  return lock;
}

extern int spinlock_try_acquire(spinlock_t *lock);

void spinlock_acquire(spinlock_t *lock) {
  int value;
  do {
    value = spinlock_try_acquire(lock);
  } while (value == SPINLOCK_NOT_ACQUIRED);
}

void spinlock_release(spinlock_t *lock) { *lock = SPINLOCK_INITIAL_VALUE; }

void spinlock_delete(spinlock_t *lock) { dispatch_free(lock); }