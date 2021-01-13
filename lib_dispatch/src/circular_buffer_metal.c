// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "circular_buffer_metal.h"

#include "dispatch_config.h"

#define PUSH_READY_EVT (0x1)
#define PUSH_EXIT_EVT (0x2)
#define POP_READY_EVT (0x3)
#define POP_EXIT_EVT (0x4)

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

bool circular_buffer_push(circular_buffer_t *cbuf, void *item) {
  dispatch_assert(cbuf);
  dispatch_assert(item);
  dispatch_assert(cbuf->buffer);

  int evt;
  bool do_push = true;

  dispatch_lock_acquire(cbuf->lock);

  if (circular_buffer_full(cbuf)) {
    cbuf->push_waiting =
        true;  // tells pop function to send us the PUSH_READY_EVT signal
    // release the lock while we wait
    dispatch_lock_release(cbuf->lock);
    // wait for signal that a slot is available
    evt = chan_in_byte(cbuf->chan.end_a);
    if (evt == POP_EXIT_EVT) do_push = false;
    // re-acquire lock
    dispatch_lock_acquire(cbuf->lock);
    cbuf->push_waiting = false;  // no longer need the PUSH_READY_EVT signal
  }

  if (do_push) {
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
  }
  dispatch_lock_release(cbuf->lock);
  return do_push;
}

bool circular_buffer_pop(circular_buffer_t *cbuf, void **item) {
  dispatch_assert(cbuf);
  dispatch_assert(cbuf->buffer);

  dispatch_lock_acquire(cbuf->lock);

  bool do_pop = true;
  int evt;

  if (circular_buffer_empty(cbuf)) {
    cbuf->pop_waiting =
        true;  // tells push function to send us the POP_READY_EVT signal
    // release the lock while we wait
    dispatch_lock_release(cbuf->lock);
    // wait for signal that a item is available
    evt = chan_in_byte(cbuf->chan.end_b);
    if (evt == POP_EXIT_EVT) do_pop = false;
    // re-acquire lock
    dispatch_lock_acquire(cbuf->lock);
    cbuf->pop_waiting = false;  // no longer need the POP_READY_EVT signal
  }

  if (do_pop) {
    // set the item
    *item = cbuf->buffer[cbuf->tail];
    // backup the pointer
    cbuf->full = false;
    cbuf->tail = (cbuf->tail + 1) % cbuf->length;

    // notify any waiting push function
    if (cbuf->push_waiting) chan_out_byte(cbuf->chan.end_b, PUSH_READY_EVT);
  }

  dispatch_lock_release(cbuf->lock);
  return do_pop;
}

void circular_buffer_destroy(circular_buffer_t *cbuf) {
  dispatch_assert(cbuf);
  dispatch_assert(cbuf->buffer);

  // notify any waiting functions that they can exit
  if (cbuf->push_waiting) chan_out_byte(cbuf->chan.end_b, PUSH_EXIT_EVT);
  if (cbuf->pop_waiting) chan_out_byte(cbuf->chan.end_a, POP_EXIT_EVT);

  chan_free(cbuf->chan);
  dispatch_free(cbuf->buffer);
  dispatch_free(cbuf);
}
