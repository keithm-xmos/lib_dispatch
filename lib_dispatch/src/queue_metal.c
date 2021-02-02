// Copyright 2021 XMOS LIMITED. This Software is subject to the terms of the 
// XMOS Public License: Version 1
#include "queue_metal.h"

#include "dispatch_config.h"

queue_t *queue_create(size_t length) {
  dispatch_assert(length > 0);

  queue_t *queue = dispatch_malloc(sizeof(queue_t));
  queue->ring_buffer = dispatch_malloc(sizeof(void *) * length);
  queue->cv = condition_variable_create();
  queue->mutex = dispatch_mutex_create();

  queue->length = length;
  queue->head = 0;
  queue->tail = 0;
  queue->full = false;

  return queue;
}

bool queue_full(queue_t *queue) {
  dispatch_assert(queue);
  return queue->full;
}

bool queue_empty(queue_t *queue) {
  dispatch_assert(queue);
  return (!queue->full && (queue->head == queue->tail));
}

size_t queue_size(queue_t *queue) {
  dispatch_assert(queue);

  dispatch_mutex_get(queue->mutex);

  size_t size = queue->length;

  if (!queue->full) {
    if (queue->head >= queue->tail) {
      size = (queue->head - queue->tail);
    } else {
      size = (queue->length + queue->head - queue->tail);
    }
  }

  dispatch_mutex_put(queue->mutex);

  return size;
}

bool queue_send(queue_t *queue, void *item, chanend_t cend) {
  dispatch_assert(queue);
  dispatch_assert(item);
  dispatch_assert(queue->ring_buffer);

  // acquire mutex for initial predicate check
  dispatch_mutex_get(queue->mutex);

  while (queue_full(queue)) {
    // queue is full, wait on condition variable
    if (!condition_variable_wait(queue->cv, queue->mutex, cend)) return false;
  }

  // NOTE: we are holding the mutex now

  // set the item
  queue->ring_buffer[queue->head] = item;

  // advance ring buffer pointer
  if (queue->full) {
    queue->tail = (queue->tail + 1) % queue->length;
  }
  queue->head = (queue->head + 1) % queue->length;
  queue->full = (queue->head == queue->tail);

  // the queue is guaranteed to be non-empty, so
  // notify any threads waiting on the condition variable
  condition_variable_broadcast(queue->cv, cend);

  // we are done with queue
  dispatch_mutex_put(queue->mutex);
  return true;
}

bool queue_receive(queue_t *queue, void **item, chanend_t cend) {
  dispatch_assert(queue);
  dispatch_assert(queue->ring_buffer);

  // acquire mutex for initial predicate check
  dispatch_mutex_get(queue->mutex);

  while (queue_empty(queue)) {
    // queue is empty, wait on condition variable
    if (!condition_variable_wait(queue->cv, queue->mutex, cend)) return false;
  }

  // NOTE: we are holding the mutex now

  // set the item
  *item = queue->ring_buffer[queue->tail];

  // backup the ring buffer pointer
  queue->full = false;
  queue->tail = (queue->tail + 1) % queue->length;

  // the queue is guaranteed to be non-full, so
  // notify any threads waiting on the condition variable
  condition_variable_broadcast(queue->cv, cend);

  // we are done with queue
  dispatch_mutex_put(queue->mutex);
  return true;
}

void queue_delete(queue_t *queue, chanend_t cend) {
  dispatch_assert(queue);
  dispatch_assert(queue->ring_buffer);

  // notify any waiters that they can stop waiting
  condition_variable_terminate(queue->cv, cend);
  condition_variable_delete(queue->cv);

  dispatch_mutex_delete(queue->mutex);

  dispatch_free(queue->ring_buffer);
  dispatch_free(queue);
}
