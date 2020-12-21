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
#include "lib_dispatch/api/dispatch_group.h"
#include "lib_dispatch/api/dispatch_queue.h"
#include "lib_dispatch/api/dispatch_task.h"

#define DISPATCH_WAKE_EVT (0x1)
#define DISPATCH_EXIT_EVT (0x2)

#define DISPATCH_TASK_NONE (0x0)

void dispatch_thread_handler(void *param) {
  dispatch_thread_data_t *thread_data = (dispatch_thread_data_t *)param;
  uint8_t evt;
  dispatch_task_t task;

  chanend_t cend = thread_data->cend;
  volatile dispatch_thread_task_t *task_id = thread_data->task;

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

  // allocate thread tasks
  queue->thread_tasks = malloc(sizeof(dispatch_thread_task_t) * thread_count);

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
    queue->thread_tasks[i] = DISPATCH_TASK_NONE;
    queue->thread_data[i].task = &queue->thread_tasks[i];
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

void dispatch_queue_async_task(dispatch_queue_t *ctx, dispatch_task_t *task) {
  assert(ctx);
  assert(task);
  dispatch_xcore_t *queue = (dispatch_xcore_t *)ctx;

  debug_printf("dispatch_queue_async_task: name=%s\n", queue->name);

  // lookup READY task
  int worker_index = -1;
  for (int i = 0; i < queue->thread_count; i++) {
    if (queue->thread_tasks[i] == DISPATCH_TASK_NONE) {
      worker_index = i;
      break;
    }
  }

  if (worker_index >= 0) {
    // assign to this queue
    task->queue = ctx;
    // signal worker to wake up (blocks waiting for worker)
    chan_out_byte(queue->thread_chanends[worker_index], DISPATCH_WAKE_EVT);
    // set thread task to the task ID it is about to execute
    queue->thread_tasks[worker_index] = (dispatch_thread_task_t)task;
    // send task to worker
    chan_out_buf_byte(queue->thread_chanends[worker_index], (void *)&task[0],
                      sizeof(dispatch_task_t));
  } else {
    // run in callers thread
    dispatch_task_perform(task);
  }
}

void dispatch_queue_wait(dispatch_queue_t *ctx) {
  assert(ctx);
  dispatch_xcore_t *queue = (dispatch_xcore_t *)ctx;

  debug_printf("dispatch_queue_wait: name=%s\n", queue->name);

  int busy_count = 0;

  for (;;) {
    busy_count = 0;
    for (int i = 0; i < queue->thread_count; i++) {
      if (queue->thread_tasks[i] != DISPATCH_TASK_NONE) busy_count++;
    }
    if (busy_count == 0) return;
  }
}

dispatch_queue_status_t dispatch_queue_task_status(dispatch_queue_t *ctx,
                                                   dispatch_task_t *task) {
  assert(ctx);
  dispatch_xcore_t *queue = (dispatch_xcore_t *)ctx;

  for (int i = 0; i < queue->thread_count; i++) {
    if (queue->thread_tasks[i] == (dispatch_thread_task_t)task) {
      return DISPATCH_QUEUE_EXECUTING;
    }
  }
  return DISPATCH_QUEUE_DONE;
}

dispatch_queue_status_t dispatch_queue_group_status(dispatch_queue_t *ctx,
                                                    dispatch_group_t *group) {
  assert(ctx);
  dispatch_xcore_t *queue = (dispatch_xcore_t *)ctx;

  for (int i = 0; i < group->length; i++) {
    dispatch_task_t *task = &group->tasks[i];
    for (int j = 0; j < queue->thread_count; j++) {
      if (queue->thread_tasks[j] == (dispatch_thread_task_t)task) {
        return DISPATCH_QUEUE_EXECUTING;
      }
    }
  }
  return DISPATCH_QUEUE_DONE;
}

void dispatch_queue_destroy(dispatch_queue_t *ctx) {
  assert(ctx);
  dispatch_xcore_t *queue = (dispatch_xcore_t *)ctx;

  assert(queue);
  assert(queue->thread_chanends);
  assert(queue->thread_tasks);
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
  free((void *)queue->thread_tasks);
  free((void *)queue->thread_chanends);
  free((void *)queue);
}
