// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "queue_metal.h"

#include "dispatch_config.h"

queue_t *queue_create(size_t length, lock_t lock) {
  dispatch_assert(length > 0);

  queue_t *queue = dispatch_malloc(sizeof(queue_t));
  queue->ring_buffer = dispatch_malloc(sizeof(void *) * length);
  queue->cv = condition_variable_create();
  queue->length = length;
  queue->lock = lock;
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

  dispatch_lock_acquire(queue->lock);

  size_t size = queue->length;

  if (!queue->full) {
    if (queue->head >= queue->tail) {
      size = (queue->head - queue->tail);
    } else {
      size = (queue->length + queue->head - queue->tail);
    }
  }

  dispatch_lock_release(queue->lock);

  return size;
}

bool queue_send(queue_t *queue, void *item, chanend_t cend) {
  dispatch_assert(queue);
  dispatch_assert(item);
  dispatch_assert(queue->ring_buffer);

  // acquire lock for initial predicate check
  dispatch_lock_acquire(queue->lock);

  while (queue_full(queue)) {
    // queue is full, wait on condition variable
    if (!condition_variable_wait(queue->cv, queue->lock, cend)) return false;
  }

  // NOTE: we are holding the lock now

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
  dispatch_lock_release(queue->lock);
  return true;
}

bool queue_receive(queue_t *queue, void **item, chanend_t cend) {
  dispatch_assert(queue);
  dispatch_assert(queue->ring_buffer);

  // acquire lock for initial predicate check
  dispatch_lock_acquire(queue->lock);

  while (queue_empty(queue)) {
    // queue is empty, wait on condition variable
    if (!condition_variable_wait(queue->cv, queue->lock, cend)) return false;
  }

  // NOTE: we are holding the lock now

  // set the item
  *item = queue->ring_buffer[queue->tail];

  // backup the ring buffer pointer
  queue->full = false;
  queue->tail = (queue->tail + 1) % queue->length;

  // the queue is guaranteed to be non-full, so
  // notify any threads waiting on the condition variable
  condition_variable_broadcast(queue->cv, cend);

  // we are done with queue
  dispatch_lock_release(queue->lock);
  return true;
}

void queue_destroy(queue_t *queue, chanend_t cend) {
  dispatch_assert(queue);
  dispatch_assert(queue->ring_buffer);

  // notify any waiters that they can stop waiting
  condition_variable_terminate(queue->cv, cend);
  condition_variable_destroy(queue->cv);

  dispatch_free(queue->ring_buffer);
  dispatch_free(queue);
}
