// Copyright (c) 2020, XMOS Ltd, All rights reserved
// clang-format off
#include "event_counter.h"
// clang-format on

#include <xcore/channel.h>

#include "dispatch_config.h"

struct event_counter_struct {
  dispatch_spinlock_t lock;
  streaming_channel_t cend;
  size_t count;
};

event_counter_t *event_counter_create(size_t count) {
  event_counter_t *counter = dispatch_malloc(sizeof(event_counter_t));

  counter->count = count;
  counter->lock = dispatch_spinlock_create();
  counter->cend = s_chan_alloc();
  return counter;
}

void event_counter_signal(event_counter_t *counter) {
  dispatch_assert(counter);

  int signal = 0;

  dispatch_spinlock_get(counter->lock);
  if (counter->count > 0) {
    if (--counter->count == 0) {
      signal = 1;
    }
  }
  dispatch_spinlock_put(counter->lock);

  if (signal) {
    chanend_out_end_token(counter->cend.end_b);
  }
}

void event_counter_wait(event_counter_t *counter) {
  dispatch_assert(counter);

  chanend_check_end_token(counter->cend.end_a);
}

void event_counter_delete(event_counter_t *counter) {
  dispatch_assert(counter);

  s_chan_free(counter->cend);
  dispatch_spinlock_delete(counter->lock);
  dispatch_free(counter);
}
