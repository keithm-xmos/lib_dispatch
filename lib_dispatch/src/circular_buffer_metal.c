// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "circular_buffer_metal.h"

#include "dispatch_config.h"

#define PUSH_READY_EVT (0x1)
#define POP_READY_EVT (0x2)

circular_buffer_t *circular_buffer_create(size_t length, lock_t lock) {
  dispatch_assert(length > 0);

  circular_buffer_t *cbuf = dispatch_malloc(sizeof(circular_buffer_t));
  cbuf->buffer = dispatch_malloc(sizeof(void *) * length);
  cbuf->length = length;
  cbuf->lock = lock;
  cbuf->chan = chan_alloc();
  cbuf->head = 0;
  cbuf->tail = 0;
  cbuf->full = false;
  cbuf->push_waiting = false;
  cbuf->pop_waiting = false;

  return cbuf;
}

bool circular_buffer_full(circular_buffer_t *cbuf) { return cbuf->full; }

bool circular_buffer_empty(circular_buffer_t *cbuf) {
  return (!cbuf->full && (cbuf->head == cbuf->tail));
}

void circular_buffer_push(circular_buffer_t *cbuf, void *item) {
  dispatch_assert(cbuf);
  dispatch_assert(item);
  dispatch_assert(cbuf->buffer);

  dispatch_lock_acquire(cbuf->lock);

  if (circular_buffer_full(cbuf)) {
    // tell pop function to send us the signal
    cbuf->push_waiting = true;
    // release the lock while we wait
    dispatch_lock_release(cbuf->lock);
    // wait for signal that a slot is available
    chan_in_byte(cbuf->chan.end_a);
    // re-acquire lock
    dispatch_lock_acquire(cbuf->lock);
    cbuf->push_waiting = false;
  }

  // set the item
  cbuf->buffer[cbuf->head] = item;

  // advance pointer
  if (cbuf->full) {
    cbuf->tail = (cbuf->tail + 1) % cbuf->length;
  }

  cbuf->head = (cbuf->head + 1) % cbuf->length;
  cbuf->full = (cbuf->head == cbuf->tail);

  // notify waiting pop function
  if (cbuf->pop_waiting) chan_out_byte(cbuf->chan.end_a, POP_READY_EVT);

  dispatch_lock_release(cbuf->lock);
}

void *circular_buffer_pop(circular_buffer_t *cbuf) {
  dispatch_assert(cbuf);
  dispatch_assert(cbuf->buffer);

  dispatch_lock_acquire(cbuf->lock);

  void *item = NULL;

  if (circular_buffer_empty(cbuf)) {
    cbuf->pop_waiting = true;
    // release the lock while we wait
    dispatch_lock_release(cbuf->lock);
    // wait for signal that a item is available
    chan_in_byte(cbuf->chan.end_b);
    // re-acquire lock
    dispatch_lock_acquire(cbuf->lock);
    cbuf->pop_waiting = false;
  }

  // set the item
  item = cbuf->buffer[cbuf->tail];
  // backup the pointer
  cbuf->full = false;
  cbuf->tail = (cbuf->tail + 1) % cbuf->length;

  // notify any waiting push function
  if (cbuf->push_waiting) chan_out_byte(cbuf->chan.end_b, PUSH_READY_EVT);

  dispatch_lock_release(cbuf->lock);

  return item;
}

void circular_buffer_destroy(circular_buffer_t *cbuf) {
  dispatch_assert(cbuf);
  dispatch_assert(cbuf->buffer);

  chan_free(cbuf->chan);
  dispatch_free(cbuf->buffer);
  dispatch_free(cbuf);
}
