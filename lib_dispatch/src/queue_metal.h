// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef DISPATCH_CIRCULAR_BUFFER_H_
#define DISPATCH_CIRCULAR_BUFFER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <xcore/channel.h>

#include "dispatch_config.h"

typedef struct queue_struct queue_t;
struct queue_struct {
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

queue_t *queue_create(size_t length, dispatch_lock_t lock);
bool queue_empty(queue_t *cbuf);
bool queue_full(queue_t *cbuf);
bool queue_send(queue_t *cbuf, void *item);
bool queue_receive(queue_t *cbuf, void **item);
void queue_destroy(queue_t *cbuf);

#endif  // DISPATCH_CIRCULAR_BUFFER_H_
