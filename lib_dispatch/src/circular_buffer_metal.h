// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef DISPATCH_CIRCULAR_BUFFER_H_
#define DISPATCH_CIRCULAR_BUFFER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <xcore/channel.h>

#include "dispatch_config.h"

typedef struct circular_buffer_struct circular_buffer_t;
struct circular_buffer_struct {
  void **buffer;
  size_t head;
  size_t tail;
  size_t length;
  dispatch_lock_t lock;
  channel_t chan;
  bool full;
  bool push_waiting;
  bool pop_waiting;
};

circular_buffer_t *circular_buffer_create(size_t length, dispatch_lock_t lock);
bool circular_buffer_empty(circular_buffer_t *cbuf);
bool circular_buffer_full(circular_buffer_t *cbuf);
bool circular_buffer_push(circular_buffer_t *cbuf, void *item);
bool circular_buffer_pop(circular_buffer_t *cbuf, void **item);
void circular_buffer_destroy(circular_buffer_t *cbuf);

#endif  // DISPATCH_CIRCULAR_BUFFER_H_
