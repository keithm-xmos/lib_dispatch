// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "lib_dispatch/api/dispatch_xcore.h"

#include <assert.h>
#include <stdlib.h>
#include <xcore/chanend.h>
#include <xcore/channel.h>
#include <xcore/hwtimer.h>
#include <xcore/thread.h>

#include "debug_print.h"

#define DISPATCH_FLAG_EVT (0x1)
#define DISPATCH_WAKE_EVT (0x2)
#define DISPATCH_EXIT_EVT (0x3)

struct dispatch_thread_handler_data {
  volatile dispatch_worker_flag_t flag;
  chanend_t cend;
};

void dispatch_thread_handler(void *param) {
  uint8_t evt;
  dispatch_task_t task;
  chanend_t cend = *((chanend_t *)(param));
  volatile dispatch_worker_flag_t *flag;

  debug_printf("dispatch_thread_handler started: cend=%u\n", (int)cend);

  for (;;) {
    evt = chan_in_byte(cend);
    if (evt == DISPATCH_WAKE_EVT) {
      debug_printf("dispatch_thread_handler wake event: cend=%u\n", (int)cend);
      chan_in_buf_byte(cend, (void *)&task, sizeof(dispatch_task_t));
      dispatch_task_wait(&task);
      // clear flag
      *flag = DISPATCH_WORKER_READY;
    } else if (evt == DISPATCH_FLAG_EVT) {
      debug_printf("dispatch_thread_handler flag event: cend=%u\n", (int)cend);
      flag = (dispatch_worker_flag_t *)chan_in_word(cend);
    } else if (evt == DISPATCH_EXIT_EVT) {
      debug_printf("dispatch_thread_handler exit event: cend=%u\n", (int)cend);
      break;
    }
  }
}

dispatch_queue_t *dispatch_queue_create(size_t length, size_t thread_count,
                                        size_t stack_size, char *name) {
  assert(length <= (thread_count + 1));  // NOTE: this is true for now
  dispatch_xcore_t *queue;

  debug_printf("dispatch_queue_create: length=%d, thread_count=%d\n", length,
               thread_count);

  queue = (dispatch_xcore_t *)malloc(sizeof(dispatch_xcore_t));

  queue->length = length;
  queue->thread_count = thread_count;
#if DEBUG_PRINT_ENABLE
  if (name)
    queue->name = name;
  else
    queue->name = "null";
#endif

  // allocate flags
  queue->flags = malloc(sizeof(int) * thread_count);

  // allocate channels
  queue->channels = malloc(sizeof(channel_t) * thread_count);

  // allocate stack
  queue->stack_size = stack_size;
  queue->stack = malloc(queue->stack_size * thread_count);

  // initialize the queue
  dispatch_queue_init(queue);

  debug_printf("dispatch_queue_create: name=%s\n", queue->name);

  return queue;
}

void dispatch_queue_init(dispatch_queue_t *ctx) {
  assert(ctx);
  dispatch_xcore_t *queue = (dispatch_xcore_t *)ctx;

  debug_printf("dispatch_queue_init: name=%s\n", queue->name);

  int stack_offset = 0;

  // create workers
  for (int i = 0; i < queue->thread_count; i++) {
    queue->channels[i] = chan_alloc();
    queue->flags[i] = DISPATCH_WORKER_READY;
    run_async(dispatch_thread_handler, (void *)&queue->channels[i].end_b,
              stack_base((void *)&queue->stack[stack_offset],
                         (queue->stack_size / sizeof(int))));
    stack_offset += queue->stack_size;
  }
  // tell workers where their status flag is
  for (int i = 0; i < queue->thread_count; i++) {
    chan_out_byte(queue->channels[i].end_a, DISPATCH_FLAG_EVT);
    // send the reference to the flag
    chan_out_word(queue->channels[i].end_a, (uint32_t)&queue->flags[i]);
  }
}

void dispatch_queue_async(dispatch_queue_t *ctx, dispatch_task_t *task) {
  assert(ctx);
  assert(task);
  dispatch_xcore_t *queue = (dispatch_xcore_t *)ctx;

  debug_printf("dispatch_queue_async: name=%s\n", queue->name);

  // lookup READY task
  int worker_index = -1;
  for (int i = 0; i < queue->thread_count; i++) {
    if (queue->flags[i] == DISPATCH_WORKER_READY) {
      worker_index = i;
      break;
    }
  }

  if (worker_index >= 0) {
    chanend_t cend = queue->channels[worker_index].end_a;

    // signal worker to wake up
    queue->flags[worker_index] = DISPATCH_WORKER_BUSY;
    chan_out_byte(cend, DISPATCH_WAKE_EVT);
    // send task to worker
    chan_out_buf_byte(cend, (void *)&task[0], sizeof(dispatch_task_t));
  } else {
    // run in callers thread
    dispatch_task_wait(task);
  }
}

void dispatch_queue_for(dispatch_queue_t *ctx, int N, dispatch_task_t *task) {
  assert(ctx);
  assert(task);
  dispatch_xcore_t *queue = (dispatch_xcore_t *)ctx;

  debug_printf("dispatch_queue_for: name=%s\n", queue->name);

  for (int i = 0; i < N; i++) {
    dispatch_queue_async(ctx, task);
  }
}

void dispatch_queue_sync(dispatch_queue_t *ctx, dispatch_task_t *task) {
  assert(ctx);
  assert(task);
  dispatch_xcore_t *queue = (dispatch_xcore_t *)ctx;

  debug_printf("dispatch_queue_sync: name=%s\n", queue->name);

  // run in callers thread
  dispatch_task_wait(task);
}

void dispatch_queue_wait(dispatch_queue_t *ctx) {
  assert(ctx);
  dispatch_xcore_t *queue = (dispatch_xcore_t *)ctx;

  debug_printf("dispatch_queue_wait: name=%s\n", queue->name);

  int busy_count = 0;

  for (;;) {
    busy_count = 0;
    for (int i = 0; i < queue->thread_count; i++) {
      if (queue->flags[i] == DISPATCH_WORKER_BUSY) busy_count++;
    }
    if (busy_count == 0) return;
  }
}

void dispatch_queue_destroy(dispatch_queue_t *ctx) {
  assert(ctx);
  dispatch_xcore_t *queue = (dispatch_xcore_t *)ctx;

  assert(queue);
  assert(queue->flags);
  assert(queue->channels);
  assert(queue->stack);

  debug_printf("dispatch_queue_async: name=%s\n", queue->name);

  // send all thread workers the EXIT event
  for (int i = 0; i < queue->thread_count; i++) {
    chan_out_byte(queue->channels[i].end_a, DISPATCH_EXIT_EVT);
  }

  // need to give task handlers time to exit
  unsigned magic_duration = 10000000;
  hwtimer_t timer = hwtimer_alloc();
  unsigned time = hwtimer_get_time(timer);
  hwtimer_wait_until(timer, time + magic_duration);
  hwtimer_free(timer);

  // now safe to free the channels
  for (int i = 0; i < queue->thread_count; i++) {
    chan_free(queue->channels[i]);
  }

  // free memory
  free((void *)queue->channels);
  free((void *)queue->flags);
  free((void *)queue->stack);
  free((void *)queue);
}
