// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "lib_dispatch/api/dispatch_xcore.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <xcore/chanend.h>
#include <xcore/channel.h>
#include <xcore/hwtimer.h>
#include <xcore/thread.h>

#include "debug_print.h"

#define DISPATCH_WAKE_EVT (0x1)
#define DISPATCH_EXIT_EVT (0x2)

void dispatch_thread_handler(void *param) {
  dispatch_thread_data_t *thread_data = (dispatch_thread_data_t *)param;
  uint8_t evt;
  dispatch_task_t task;

  chanend_t cend = thread_data->cend;
  volatile dispatch_thread_status_t *status = thread_data->status;

  debug_printf("dispatch_thread_handler started: cend=%u\n", (int)cend);

  for (;;) {
    evt = chan_in_byte(cend);
    if (evt == DISPATCH_WAKE_EVT) {
      debug_printf("dispatch_thread_handler wake event: cend=%u\n", (int)cend);
      // read the task
      chan_in_buf_byte(cend, (void *)&task, sizeof(dispatch_task_t));
      // run the task
      dispatch_task_perform(&task);
      // clear status
      *status = DISPATCH_THREAD_READY;
    } else if (evt == DISPATCH_EXIT_EVT) {
      debug_printf("dispatch_thread_handler exit event: cend=%u\n", (int)cend);
      // exit forever loop
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
    strncpy(queue->name, name, 32);
  else
    strncpy(queue->name, "null", 32);
#endif

  // allocate channels
  queue->thread_chanends = malloc(sizeof(channel_t) * thread_count);

  // allocate thread status
  queue->thread_status =
      malloc(sizeof(dispatch_thread_status_t) * thread_count);

  // allocate thread data
  queue->thread_data = malloc(sizeof(dispatch_thread_data_t) * thread_count);

  // allocate thread stack
  queue->thread_stack_size = stack_size;
  queue->thread_stack = malloc(queue->thread_stack_size * thread_count);

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
  int stack_words = queue->thread_stack_size / sizeof(int);

  // create workers
  for (int i = 0; i < queue->thread_count; i++) {
    queue->thread_status[i] = DISPATCH_THREAD_READY;
    queue->thread_data[i].status = &queue->thread_status[i];
    // create and setup chanends
    queue->thread_chanends[i] = chanend_alloc();   // queue's chanend
    queue->thread_data[i].cend = chanend_alloc();  // worker's chanend
    chanend_set_dest(queue->thread_chanends[i], queue->thread_data[i].cend);
    chanend_set_dest(queue->thread_data[i].cend, queue->thread_chanends[i]);
    // launch the thread worker
    run_async(
        dispatch_thread_handler, (void *)&queue->thread_data[i],
        stack_base((void *)&queue->thread_stack[stack_offset], stack_words));
    stack_offset += queue->thread_stack_size;
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
    if (queue->thread_status[i] == DISPATCH_THREAD_READY) {
      worker_index = i;
      break;
    }
  }

  if (worker_index >= 0) {
    // assign to this queue
    task->queue = ctx;
    // signal worker to wake up (blocks waiting for worker)
    chan_out_byte(queue->thread_chanends[worker_index], DISPATCH_WAKE_EVT);
    // set thread status and task status to executing
    queue->thread_status[worker_index] = DISPATCH_THREAD_BUSY;
    // send task to worker
    chan_out_buf_byte(queue->thread_chanends[worker_index], (void *)&task[0],
                      sizeof(dispatch_task_t));
  } else {
    // run in callers thread
    dispatch_task_perform(task);
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
      if (queue->thread_status[i] == DISPATCH_THREAD_BUSY) busy_count++;
    }
    if (busy_count == 0) return;
  }
}

void dispatch_queue_destroy(dispatch_queue_t *ctx) {
  assert(ctx);
  dispatch_xcore_t *queue = (dispatch_xcore_t *)ctx;

  assert(queue);
  assert(queue->thread_chanends);
  // assert(queue->thread_status);
  assert(queue->thread_data);
  assert(queue->thread_stack);

  debug_printf("dispatch_queue_async: name=%s\n", queue->name);

  // send all thread workers the EXIT event
  for (int i = 0; i < queue->thread_count; i++) {
    chan_out_byte(queue->thread_chanends[i], DISPATCH_EXIT_EVT);
  }

  // need to give task handlers time to exit
  unsigned magic_duration = 10000000;
  hwtimer_t timer = hwtimer_alloc();
  unsigned time = hwtimer_get_time(timer);
  hwtimer_wait_until(timer, time + magic_duration);
  hwtimer_free(timer);

  // now safe to free the chanends
  for (int i = 0; i < queue->thread_count; i++) {
    chanend_free(queue->thread_data[i].cend);
    chanend_free(queue->thread_chanends[i]);
  }

  // free memory
  free((void *)queue->thread_stack);
  free((void *)queue->thread_data);
  // free((void *)queue->thread_status);
  free((void *)queue->thread_chanends);
  free((void *)queue);
}
