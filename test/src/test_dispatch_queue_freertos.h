// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef TEST_DISPATCH_QUEUE_FREERTOS_H_
#define TEST_DISPATCH_QUEUE_FREERTOS_H_

#include "FreeRTOS.h"
#include "semphr.h"

#define QUEUE_LENGTH (10)
#define QUEUE_THREAD_COUNT (3)
#define QUEUE_STACK_SIZE (1024)  // more than enough

typedef SemaphoreHandle_t thread_mutex_t;

void keep_busy();
void mutex_init(thread_mutex_t *lock);
void mutex_lock(thread_mutex_t *lock);
void mutex_unlock(thread_mutex_t *lock);
void mutex_destroy(thread_mutex_t *lock);

inline void keep_busy() {
  unsigned magic_duration = 1000;
  vTaskDelay(magic_duration);
}

inline void mutex_init(thread_mutex_t *lock) {
  *lock = xSemaphoreCreateMutex();
}

inline void mutex_lock(thread_mutex_t *lock) {
  xSemaphoreTake(*lock, portMAX_DELAY);
}

inline void mutex_unlock(thread_mutex_t *lock) { xSemaphoreGive(*lock); }

inline void mutex_destroy(thread_mutex_t *lock) { vSemaphoreDelete(*lock); }

#endif  // TEST_DISPATCH_QUEUE_FREERTOS_H_
