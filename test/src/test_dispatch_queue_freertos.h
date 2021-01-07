// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef TEST_DISPATCH_QUEUE_FREERTOS_H_
#define TEST_DISPATCH_QUEUE_FREERTOS_H_

#include "FreeRTOS.h"
#include "semphr.h"

#define QUEUE_LENGTH (10)
#define QUEUE_THREAD_COUNT (3)
#define QUEUE_THREAD_STACK_SIZE (1024)  // more than enough
#define QUEUE_THREAD_PRIORITY (configMAX_PRIORITIES - 1)

typedef SemaphoreHandle_t thread_mutex_t;

void look_busy(int milliseconds);
thread_mutex_t mutex_init();
void mutex_lock(thread_mutex_t lock);
void mutex_unlock(thread_mutex_t lock);
void mutex_destroy(thread_mutex_t lock);

inline void look_busy(int milliseconds) {
  const TickType_t xDelay = milliseconds / portTICK_PERIOD_MS;
  vTaskDelay(xDelay);
}

inline thread_mutex_t mutex_init() { return xSemaphoreCreateMutex(); }

inline void mutex_lock(thread_mutex_t lock) {
  xSemaphoreTake(lock, portMAX_DELAY);
}

inline void mutex_unlock(thread_mutex_t lock) { xSemaphoreGive(lock); }

inline void mutex_destroy(thread_mutex_t lock) { vSemaphoreDelete(lock); }

#endif  // TEST_DISPATCH_QUEUE_FREERTOS_H_
