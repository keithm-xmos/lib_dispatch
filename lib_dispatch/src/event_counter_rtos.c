// Copyright (c) 2020, XMOS Ltd, All rights reserved

#include "FreeRTOS.h"
#include "dispatch_config.h"
#include "event_counter.h"
#include "semphr.h"

struct event_counter_struct {
  SemaphoreHandle_t semaphore;
  size_t count;
};

event_counter_t *event_counter_create(size_t count, dispatch_lock_t lock) {
  event_counter_t *counter = dispatch_malloc(sizeof(event_counter_t));

  counter->count = count;
  counter->semaphore = xSemaphoreCreateBinary();
  return counter;
}

void event_counter_signal(event_counter_t *counter) {
  dispatch_assert(counter);

  int signal = 0;

  taskENTER_CRITICAL();
  if (counter->count > 0) {
    if (--counter->count == 0) {
      signal = 1;
    }
  }
  taskEXIT_CRITICAL();

  if (signal) {
    xSemaphoreGive(counter->semaphore);
  }
}

void event_counter_wait(event_counter_t *counter) {
  dispatch_assert(counter);

  xSemaphoreTake(counter->semaphore, portMAX_DELAY);
}

void event_counter_destroy(event_counter_t *counter) {
  dispatch_assert(counter);

  vSemaphoreDelete(counter->semaphore);
  dispatch_free(counter);
}
