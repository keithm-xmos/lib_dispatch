// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "queue_metal.h"

#include "dispatch_config.h"

#define PUSH_READY_EVT (0x1)
#define PUSH_EXIT_EVT (0x2)
#define POP_READY_EVT (0x3)
#define POP_EXIT_EVT (0x4)

queue_t *queue_create(size_t length, lock_t lock) {
  dispatch_assert(length > 0);

  queue_t *cbuf = dispatch_malloc(sizeof(queue_t));
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

bool queue_full(queue_t *cbuf) { return cbuf->full; }

bool queue_empty(queue_t *cbuf) {
  return (!cbuf->full && (cbuf->head == cbuf->tail));
}

bool queue_send(queue_t *cbuf, void *item) {
  dispatch_assert(cbuf);
  dispatch_assert(item);
  dispatch_assert(cbuf->buffer);

  int evt;
  bool do_push = true;

  dispatch_lock_acquire(cbuf->lock);

  if (queue_full(cbuf)) {
    cbuf->push_waiting =
        true;  // tells pop function to send us the PUSH_READY_EVT signal
    // release the lock while we wait
    dispatch_lock_release(cbuf->lock);
    // TODO: create chanend
    //       add it to push_waiting list
    //       then...
    // wait for signal that a slot is available
    evt = chan_in_byte(cbuf->chan.end_a);
    if (evt == POP_EXIT_EVT) do_push = false;
    // re-acquire lock
    dispatch_lock_acquire(cbuf->lock);
    // TODO: remove from push_waiting list
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
    // TODO: pick chanend on pop_waiting list
    //       connect chanends
    //       send signal
    if (cbuf->pop_waiting) chan_out_byte(cbuf->chan.end_a, POP_READY_EVT);
  }
  dispatch_lock_release(cbuf->lock);
  return do_push;
}

bool queue_receive(queue_t *cbuf, void **item) {
  dispatch_assert(cbuf);
  dispatch_assert(cbuf->buffer);

  dispatch_lock_acquire(cbuf->lock);

  bool do_pop = true;
  int evt;

  if (queue_empty(cbuf)) {
    cbuf->pop_waiting =
        true;  // tells push function to send us the POP_READY_EVT signal
    // release the lock while we wait
    dispatch_lock_release(cbuf->lock);
    // TODO: create chanend
    //       add it to pop_waiting list
    //       then...
    // wait for signal that a item is available
    evt = chan_in_byte(cbuf->chan.end_b);
    if (evt == POP_EXIT_EVT) do_pop = false;
    // TODO: free chanend
    // re-acquire lock
    dispatch_lock_acquire(cbuf->lock);
    // TODO: remove from pop_waiting list
    cbuf->pop_waiting = false;  // no longer need the POP_READY_EVT signal
  }

  if (do_pop) {
    // set the item
    *item = cbuf->buffer[cbuf->tail];
    // backup the pointer
    cbuf->full = false;
    cbuf->tail = (cbuf->tail + 1) % cbuf->length;

    // notify any waiting push function
    // TODO: pick chanend on push_waiting list
    //       connect chanends
    //       send signal
    if (cbuf->push_waiting) chan_out_byte(cbuf->chan.end_b, PUSH_READY_EVT);
  }

  dispatch_lock_release(cbuf->lock);
  return do_pop;
}

void queue_destroy(queue_t *cbuf) {
  dispatch_assert(cbuf);
  dispatch_assert(cbuf->buffer);

  // notify any waiting functions that they can exit
  // TODO: implement me
  // if (cbuf->push_waiting) chan_out_byte(cbuf->chan.end_b, PUSH_EXIT_EVT);
  // if (cbuf->pop_waiting) chan_out_byte(cbuf->chan.end_a, POP_EXIT_EVT);

  chan_free(cbuf->chan);
  dispatch_free(cbuf->buffer);
  dispatch_free(cbuf);
}
