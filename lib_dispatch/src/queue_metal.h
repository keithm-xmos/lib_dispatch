// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef DISPATCH_CIRCULAR_BUFFER_H_
#define DISPATCH_CIRCULAR_BUFFER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <xcore/channel.h>

#include "condition_variable_metal.h"
#include "dispatch_config.h"

typedef struct queue_struct queue_t;
struct queue_struct {
  void **ring_buffer;
  condition_variable_t *cv;
  size_t head;
  size_t tail;
  size_t length;
  dispatch_lock_t lock;
  bool full;
};

queue_t *queue_create(size_t length, dispatch_lock_t lock);
bool queue_empty(queue_t *queue);
bool queue_full(queue_t *queue);
size_t queue_size(queue_t *queue);
bool queue_send(queue_t *queue, void *item, chanend_t cend);
bool queue_receive(queue_t *queue, void **item, chanend_t cend);
void queue_destroy(queue_t *queue, chanend_t cend);

#endif  // DISPATCH_CIRCULAR_BUFFER_H_
