// Copyright (c) 2020, XMOS Ltd, All rights reserved

#include "dispatch_event_counter.h"

#include "dispatch_config.h"

#if BARE_METAL

#include <xcore/channel.h>

#elif FREERTOS

#include "FreeRTOS.h"
#include "semphr.h"

#endif

struct dispatch_event_counter_struct {
#if BARE_METAL
  dispatch_lock_t lock;
  streaming_channel_t channel;
#elif FREERTOS
  SemaphoreHandle_t semaphore;
#endif
  size_t count;
};

dispatch_event_counter_t *dispatch_event_counter_create(size_t count,
                                                        dispatch_lock_t lock) {
  dispatch_event_counter_t *counter =
      dispatch_malloc(sizeof(dispatch_event_counter_t));

  counter->count = count;
#if BARE_METAL
  counter->lock = lock;
  counter->channel = s_chan_alloc();
#elif FREERTOS
  counter->semaphore = xSemaphoreCreateBinary();
#endif
  return counter;
}

void dispatch_event_counter_signal(dispatch_event_counter_t *counter) {
  dispatch_assert(counter);

#if BARE_METAL
  dispatch_lock_acquire(counter->lock);
  if (counter->count > 0) --counter->count;
  dispatch_lock_release(counter->lock);

  if (counter->count == 0) {
    chanend_out_end_token(counter->channel.end_b);
  }

#elif FREERTOS
  taskENTER_CRITICAL();
  if (counter->count > 0) --counter->count;
  taskEXIT_CRITICAL();

  if (counter->count == 0) {
    xSemaphoreGive(counter->semaphore);
  }

#endif
}

void dispatch_event_counter_wait(dispatch_event_counter_t *counter) {
  dispatch_assert(counter);

#if BARE_METAL
  chanend_check_end_token(counter->channel.end_a);
#elif FREERTOS
  xSemaphoreTake(counter->semaphore, portMAX_DELAY);
#endif
}

void dispatch_event_counter_destroy(dispatch_event_counter_t *counter) {
  dispatch_assert(counter);

#if BARE_METAL
  s_chan_free(counter->channel);
#elif FREERTOS
  vSemaphoreDelete(counter->semaphore);
#endif
  dispatch_free(counter);
}
