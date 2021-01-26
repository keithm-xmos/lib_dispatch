// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "spinlock_metal.h"

void swlock_init(swlock_t *_lock) {
  volatile swlock_t *lock = _lock;
  *lock = 0;
}

extern int swlock_try_acquire(swlock_t *lock);

void swlock_acquire(swlock_t *lock) {
  int value;
  do {
    value = swlock_try_acquire(lock);
  } while (value == SWLOCK_NOT_ACQUIRED);
}

void swlock_release(swlock_t *lock) { *lock = 0; }
