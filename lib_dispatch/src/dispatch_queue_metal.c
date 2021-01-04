// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "dispatch_queue_metal.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <xcore/assert.h>
#include <xcore/chanend.h>
#include <xcore/channel.h>
#include <xcore/hwtimer.h>
#include <xcore/thread.h>

#include "dispatch.h"

#define DISPATCH_WAKE_EVT (0x1)
#define DISPATCH_EXIT_EVT (0x2)

void dispatch_thread_handler(void *param) {
  dispatch_thread_data_t *thread_data = (dispatch_thread_data_t *)param;
  uint8_t evt;
  dispatch_task_t task;

  chanend_t cend = thread_data->cend;
  volatile size_t *task_id = thread_data->task_id;

  debug_printf("dispatch_thread_handler started: cend=%u\n", (int)cend);

  for (;;) {
    evt = chan_in_byte(cend);
    if (evt == DISPATCH_WAKE_EVT) {
      debug_printf("dispatch_thread_handler wake event: cend=%u\n", (int)cend);
      // read the task
      chan_in_buf_byte(cend, (void *)&task, sizeof(dispatch_task_t));
      // run the task
      dispatch_task_perform(&task);
      // clear task id
      *task_id = DISPATCH_TASK_NONE;
    } else if (evt == DISPATCH_EXIT_EVT) {
      debug_printf("dispatch_thread_handler exit event: cend=%u\n", (int)cend);
      // exit forever loop
      break;
    }
  }
}

dispatch_queue_t *dispatch_queue_create(size_t length, size_t thread_count,
                                        size_t thread_stack_size,
                                        size_t thread_priority,
                                        const char *name) {
  xassert(length <= (thread_count + 1));  // NOTE: this is true for now
  dispatch_xcore_queue_t *queue;

  debug_printf("dispatch_queue_create: length=%d, thread_count=%d\n", length,
               thread_count);

  queue = (dispatch_xcore_queue_t *)malloc(sizeof(dispatch_xcore_queue_t));

  queue->length = length;
  queue->thread_count = thread_count;
#if !NDEBUG
  if (name)
    strncpy(queue->name, name, 32);
  else
    strncpy(queue->name, "null", 32);
#endif

  // allocate thread task ids
  queue->thread_task_ids = malloc(sizeof(size_t) * thread_count);

  // allocate thread data
  queue->thread_data = malloc(sizeof(dispatch_thread_data_t) * thread_count);

  // allocate thread stack
  queue->thread_stack_size = thread_stack_size;
  queue->thread_stack = malloc(queue->thread_stack_size * thread_count);

  // initialize the queue
  dispatch_queue_init(queue, thread_priority);

  debug_printf("dispatch_queue_create: name=%s\n", queue->name);

  return queue;
}

void dispatch_queue_init(dispatch_queue_t *ctx, size_t thread_priority) {
  xassert(ctx);
  dispatch_xcore_queue_t *queue = (dispatch_xcore_queue_t *)ctx;

  debug_printf("dispatch_queue_init: name=%s\n", queue->name);

  int stack_offset = 0;

  queue->next_id = DISPATCH_TASK_NONE + 1;

  queue->thread_chanend = chanend_alloc();  // dispatch queue's chanend

  // create workers
  for (int i = 0; i < queue->thread_count; i++) {
    debug_printf("dispatch_queue_init: i=%d\n", i);
    queue->thread_task_ids[i] = DISPATCH_TASK_NONE;
    queue->thread_data[i].task_id = &queue->thread_task_ids[i];
    // create and setup chanends
    queue->thread_data[i].cend = chanend_alloc();  // worker's chanend
    // launch the thread worker
    run_async(dispatch_thread_handler, (void *)&queue->thread_data[i],
              stack_base((void *)&queue->thread_stack[stack_offset],
                         queue->thread_stack_size));
    stack_offset += queue->thread_stack_size * sizeof(int);
  }
}

size_t dispatch_queue_task_add(dispatch_queue_t *ctx, dispatch_task_t *task) {
  xassert(ctx);
  xassert(task);
  dispatch_xcore_queue_t *queue = (dispatch_xcore_queue_t *)ctx;

  debug_printf("dispatch_queue_add_task: name=%s\n", queue->name);

  // lookup READY task
  int worker_index = -1;
  for (;;) {
    for (int i = 0; i < queue->thread_count; i++) {
      if (INVALID_TASK_ID(queue->thread_task_ids[i])) {
        worker_index = i;
        break;
      }
    }
    if (worker_index >= 0) break;
  }

  if (worker_index >= 0) {
    // assign to this queue
    task->queue = ctx;
    task->id = queue->next_id++;

    // setup chanend route
    chanend_set_dest(queue->thread_chanend,
                     queue->thread_data[worker_index].cend);
    chanend_set_dest(queue->thread_data[worker_index].cend,
                     queue->thread_chanend);
    // signal worker to wake up (blocks waiting for worker)
    chan_out_byte(queue->thread_chanend, DISPATCH_WAKE_EVT);
    // set thread task to the task ID it is about to execute
    queue->thread_task_ids[worker_index] = task->id;
    // send task to worker
    chan_out_buf_byte(queue->thread_chanend, (void *)&task[0],
                      sizeof(dispatch_task_t));
  }

  return task->id;
}

void dispatch_queue_wait(dispatch_queue_t *ctx) {
  xassert(ctx);
  dispatch_xcore_queue_t *queue = (dispatch_xcore_queue_t *)ctx;

  debug_printf("dispatch_queue_wait: name=%s\n", queue->name);

  int busy_count = 0;

  for (;;) {
    busy_count = 0;
    for (int i = 0; i < queue->thread_count; i++) {
      if (VALID_TASK_ID(queue->thread_task_ids[i])) busy_count++;
    }
    if (busy_count == 0) return;
  }
}

void dispatch_queue_task_wait(dispatch_queue_t *ctx, int task_id) {
  xassert(ctx);
  xassert(task_id > 0);

  dispatch_xcore_queue_t *queue = (dispatch_xcore_queue_t *)ctx;

  bool task_is_running = false;

  for (;;) {
    task_is_running = false;
    for (int i = 0; i < queue->thread_count; i++) {
      if (queue->thread_task_ids[i] == task_id) {
        task_is_running = true;
        break;
      }
    }
    if (!task_is_running) return;
  }
}

void dispatch_queue_destroy(dispatch_queue_t *ctx) {
  xassert(ctx);
  dispatch_xcore_queue_t *queue = (dispatch_xcore_queue_t *)ctx;

  xassert(queue);
  xassert(queue->thread_task_ids);
  xassert(queue->thread_data);
  xassert(queue->thread_stack);

  debug_printf("dispatch_queue_destroy: name=%s\n", queue->name);

  // send all thread workers the EXIT event
  for (int i = 0; i < queue->thread_count; i++) {
    chanend_set_dest(queue->thread_chanend, queue->thread_data[i].cend);
    chanend_set_dest(queue->thread_data[i].cend, queue->thread_chanend);
    chan_out_byte(queue->thread_chanend, DISPATCH_EXIT_EVT);
  }

  // need to give task handlers time to exit
  unsigned magic_duration = 10000000;
  hwtimer_t timer = hwtimer_alloc();
  unsigned time = hwtimer_get_time(timer);
  hwtimer_wait_until(timer, time + magic_duration);
  hwtimer_free(timer);

  // now safe to free the chanends
  chanend_free(queue->thread_chanend);
  for (int i = 0; i < queue->thread_count; i++) {
    chanend_free(queue->thread_data[i].cend);
  }

  // free memory
  free((void *)queue->thread_stack);
  free((void *)queue->thread_data);
  free((void *)queue->thread_task_ids);
  free((void *)queue);
}
