// Copyright (c) 2020, XMOS Ltd, All rights reserved

#include <xcore/channel.h>

#include "dispatch_config.h"
#include "event_counter.h"

struct event_counter_struct {
  dispatch_lock_t lock;
  streaming_channel_t channel;
  size_t count;
};

event_counter_t *event_counter_create(size_t count, dispatch_lock_t lock) {
  event_counter_t *counter = dispatch_malloc(sizeof(event_counter_t));

  counter->count = count;
  counter->lock = lock;
  counter->channel = s_chan_alloc();
  return counter;
}

void event_counter_signal(event_counter_t *counter) {
  dispatch_assert(counter);

  int signal = 0;

  dispatch_lock_acquire(counter->lock);
  if (counter->count > 0) {
    if (--counter->count == 0) {
      signal = 1;
    }
  }
  dispatch_lock_release(counter->lock);

  if (signal) {
    chanend_out_end_token(counter->channel.end_b);
  }
}

void event_counter_wait(event_counter_t *counter) {
  dispatch_assert(counter);

  chanend_check_end_token(counter->channel.end_a);
}

void event_counter_destroy(event_counter_t *counter) {
  dispatch_assert(counter);

  s_chan_free(counter->channel);
  dispatch_free(counter);
}
