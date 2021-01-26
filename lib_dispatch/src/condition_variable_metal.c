#include "condition_variable_metal.h"

#define CONDITION_SIGNAL_EVT 0x1
#define CONDITION_TERMINATE_EVT 0x2

condition_variable_t* condition_variable_create() {
  condition_variable_t* cv = dispatch_malloc(sizeof(condition_variable_t));
  cv->waiters = NULL;
  swlock_init(&cv->swlock);

  return cv;
}

bool condition_variable_wait(condition_variable_t* cv, dispatch_lock_t lock,
                             chanend_t dest) {
  dispatch_assert(cv);

  swlock_acquire(&cv->swlock);
  condition_node_t waiter;
  waiter.cend = dest;
  waiter.next = cv->waiters;
  cv->waiters = &waiter;
  swlock_release(&cv->swlock);

  // release lock while we wait
  dispatch_lock_release(lock);

  // wait for signal
  int evt = chan_in_byte(waiter.cend);

  if (evt == CONDITION_SIGNAL_EVT) {
    // re-acquire lock so the caller can hold it
    dispatch_lock_acquire(lock);
    return true;
  }

  // we must have received the terminate signal
  //  returning false informs the caller that additional waiting is pointless
  return false;
}

static void condition_variable_send(chanend_t source, chanend_t dest,
                                    int8_t signal) {
  chanend_set_dest(source, dest);
  chanend_set_dest(dest, source);
  chan_out_byte(source, signal);
}

void condition_variable_signal(condition_variable_t* cv, chanend_t source) {
  dispatch_assert(cv);

  swlock_acquire(&cv->swlock);

  // send the next waiter the signal event
  if (cv->waiters != NULL) {
    condition_variable_send(source, cv->waiters->cend, CONDITION_SIGNAL_EVT);
    cv->waiters = cv->waiters->next;
  }

  swlock_release(&cv->swlock);
}

void condition_variable_broadcast(condition_variable_t* cv, chanend_t source) {
  dispatch_assert(cv);

  swlock_acquire(&cv->swlock);

  // send all waiter the signal event
  while (cv->waiters != NULL) {
    condition_variable_send(source, cv->waiters->cend, CONDITION_SIGNAL_EVT);
    cv->waiters = cv->waiters->next;
  }

  swlock_release(&cv->swlock);
}

void condition_variable_terminate(condition_variable_t* cv, chanend_t source) {
  dispatch_assert(cv);

  swlock_acquire(&cv->swlock);

  // send all waiter the terminate event
  while (cv->waiters != NULL) {
    condition_variable_send(source, cv->waiters->cend, CONDITION_TERMINATE_EVT);
    cv->waiters = cv->waiters->next;
  }

  swlock_release(&cv->swlock);
}

void condition_variable_destroy(condition_variable_t* cv) {
  dispatch_assert(cv);

  dispatch_free(cv);
}
