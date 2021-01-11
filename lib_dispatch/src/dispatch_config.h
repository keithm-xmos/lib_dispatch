// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef DISPATCH_CONFIG_H_
#define DISPATCH_CONFIG_H_

#if BARE_METAL

#include <stdlib.h>
#include <xcore/assert.h>
#include <xcore/lock.h>

#include "debug_print.h"

typedef lock_t dispatch_lock_t;

#define dispatch_assert(A) xassert(A)

#define dispatch_malloc(A) malloc(A)
#define dispatch_free(A) free(A)

#define dispatch_printf(...) debug_printf(__VA_ARGS__)

static inline dispatch_lock_t dispatch_lock_alloc() { return lock_alloc(); }
static inline void dispatch_lock_acquire(dispatch_lock_t lock) {
  lock_acquire(lock);
}
static inline void dispatch_lock_release(dispatch_lock_t lock) {
  lock_release(lock);
}
static inline void dispatch_lock_free(dispatch_lock_t lock) { lock_free(lock); }

#elif FREERTOS

#include <xcore/assert.h>

#include "FreeRTOS.h"
#include "rtos_printf.h"
#include "semphr.h"

typedef SemaphoreHandle_t dispatch_lock_t;

#define dispatch_assert(A) xassert(A)

#define dispatch_malloc(A) pvPortMalloc(A)
#define dispatch_free(A) vPortFree(A)

#define dispatch_printf(...) debug_printf(__VA_ARGS__)

static inline dispatch_lock_t dispatch_lock_alloc() {
  return xSemaphoreCreateMutex();
}
static inline void dispatch_lock_acquire(dispatch_lock_t lock) {
  xSemaphoreTake(lock, portMAX_DELAY);
}
static inline void dispatch_lock_release(dispatch_lock_t lock) {
  xSemaphoreGive(lock);
}
static inline void dispatch_lock_free(dispatch_lock_t lock) {
  vSemaphoreDelete(lock);
}

#elif HOST

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef pthread_mutex_t *dispatch_lock_t;

#define dispatch_assert(A) assert(A)

#define dispatch_malloc(A) malloc(A)
#define dispatch_free(A) free(A)

#define dispatch_printf(...) printf(__VA_ARGS__)

static inline dispatch_lock_t dispatch_lock_alloc() {
  pthread_mutex_t *mutex =
      (pthread_mutex_t *)dispatch_malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex, NULL);
  return mutex;
}
static inline void dispatch_lock_acquire(dispatch_lock_t lock) {
  pthread_mutex_lock(lock);
}
static inline void dispatch_lock_release(dispatch_lock_t lock) {
  pthread_mutex_unlock(lock);
}
static inline void dispatch_lock_free(dispatch_lock_t lock) {
  pthread_mutex_destroy(lock);
  dispatch_free(lock);
}

#endif

#endif  // DISPATCH_CONFIG_H_