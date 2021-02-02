// Copyright 2021 XMOS LIMITED. This Software is subject to the terms of the 
// XMOS Public License: Version 1
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <xcore/assert.h>
#include <xcore/chanend.h>
#include <xcore/channel.h>
#include <xcore/hwtimer.h>
#include <xcore/thread.h>

#include "dispatch_config.h"
#include "dispatch_group.h"
#include "dispatch_queue.h"
#include "dispatch_task.h"
#include "dispatch_types.h"
#include "event_counter.h"
#include "queue_metal.h"

#define DISPATCH_WORKER_BUSY_STATUS (0x0)
#define DISPATCH_WORKER_READY_STATUS (0x1)

//***********************
//***********************
//***********************
// Worker
//***********************
//***********************
//***********************

typedef struct dispatch_worker_data_struct dispatch_worker_data_t;
struct dispatch_worker_data_struct {
  volatile size_t *status;
  size_t parent;
  queue_t *queue;
};

static void run_task(dispatch_task_t *task) {
  dispatch_task_perform(task);

  if (task->waitable) {
    // signal the event counter
    event_counter_signal((event_counter_t *)task->private_data);
  } else {
    // the contract is that the worker must delete non-waitable tasks
    dispatch_task_delete(task);
  }
}

void dispatch_queue_worker(void *param) {
  dispatch_worker_data_t *worker_data = (dispatch_worker_data_t *)param;
  dispatch_task_t *task = NULL;
  chanend_t cend;

  queue_t *queue = worker_data->queue;
  volatile size_t *status = worker_data->status;
  cend = chanend_alloc();

  dispatch_printf("dispatch_queue_worker started: parent=%u\n",
                  worker_data->parent);

  for (;;) {
    if (queue_receive(queue, (void **)&task, cend)) {
      *status = DISPATCH_WORKER_BUSY_STATUS;
      run_task(task);
      *status = DISPATCH_WORKER_READY_STATUS;
    } else {
      chanend_free(cend);
      dispatch_printf("dispatch_queue_worker terminating: parent=%u\n",
                      worker_data->parent);
      break;
    }
  }
}

//***********************
//***********************
//***********************
// Queue struct
//***********************
//***********************
//***********************

typedef struct dispatch_xcore_struct dispatch_xcore_queue_t;
struct dispatch_xcore_struct {
  size_t length;
  size_t thread_count;
  size_t thread_stack_size;
  queue_t *queue;
  chanend_t cend;
  char *thread_stack;
  size_t *thread_status;
  dispatch_worker_data_t *worker_data;
};

static int busy_workers(dispatch_xcore_queue_t *dispatch_queue) {
  size_t busy_count = 0;

  for (int i = 0; i < dispatch_queue->thread_count; i++) {
    if (dispatch_queue->thread_status[i] == DISPATCH_WORKER_BUSY_STATUS)
      busy_count++;
  }

  return busy_count;
}

//***********************
//***********************
//***********************
// Queue implementation
//***********************
//***********************
//***********************

dispatch_queue_t *dispatch_queue_create(size_t length, size_t thread_count,
                                        size_t thread_stack_size,
                                        size_t thread_priority) {
  dispatch_xcore_queue_t *dispatch_queue;

  dispatch_printf("dispatch_queue_create: length=%d, thread_count=%d\n", length,
                  thread_count);

  dispatch_queue =
      (dispatch_xcore_queue_t *)dispatch_malloc(sizeof(dispatch_xcore_queue_t));

  dispatch_queue->length = length;
  dispatch_queue->thread_count = thread_count;
  dispatch_queue->cend = chanend_alloc();

  // allocate  queue
  dispatch_queue->queue = queue_create(length);

  // allocate thread status flags
  dispatch_queue->thread_status =
      dispatch_malloc(sizeof(size_t) * thread_count);

  // allocate thread data
  dispatch_queue->worker_data =
      dispatch_malloc(sizeof(dispatch_worker_data_t) * thread_count);

  // allocate thread stack
  dispatch_queue->thread_stack_size = thread_stack_size;
  dispatch_queue->thread_stack =
      dispatch_malloc(dispatch_queue->thread_stack_size * thread_count);

  // initialize the queue
  dispatch_queue_init(dispatch_queue, thread_priority);

  dispatch_printf("dispatch_queue_create: %u\n", (size_t)dispatch_queue);

  return dispatch_queue;
}

void dispatch_queue_init(dispatch_queue_t *ctx, size_t thread_priority) {
  dispatch_xcore_queue_t *dispatch_queue = (dispatch_xcore_queue_t *)ctx;
  dispatch_assert(dispatch_queue);

  dispatch_printf("dispatch_queue_init: %u\n", (size_t)dispatch_queue);

  int stack_offset = 0;

  // create workers
  for (int i = 0; i < dispatch_queue->thread_count; i++) {
    dispatch_queue->thread_status[i] = DISPATCH_WORKER_READY_STATUS;
    dispatch_queue->worker_data[i].status = &dispatch_queue->thread_status[i];
    dispatch_queue->worker_data[i].parent = (size_t)dispatch_queue;
    dispatch_queue->worker_data[i].queue = dispatch_queue->queue;
    // launch the thread worker
    run_async(dispatch_queue_worker, (void *)&dispatch_queue->worker_data[i],
              stack_base((void *)&dispatch_queue->thread_stack[stack_offset],
                         dispatch_queue->thread_stack_size));
    stack_offset += dispatch_queue->thread_stack_size * sizeof(int);
  }
}

void dispatch_queue_task_add(dispatch_queue_t *ctx, dispatch_task_t *task) {
  dispatch_xcore_queue_t *dispatch_queue = (dispatch_xcore_queue_t *)ctx;
  dispatch_assert(dispatch_queue);
  dispatch_assert(task);

  dispatch_printf("dispatch_queue_task_add: %u\n", (size_t)dispatch_queue);

  if (task->waitable) {
    // create event counter
    task->private_data = event_counter_create(1);
  }

#if defined(use_callers_thread)
  if (busy_workers(dispatch_queue) == dispatch_queue->thread_count) {
    // all the workers are busy and this thread is configured to pitch in
    run_task(task);
    return;
  }
#endif

  queue_send(dispatch_queue->queue, (void *)task, dispatch_queue->cend);
}

void dispatch_queue_group_add(dispatch_queue_t *ctx, dispatch_group_t *group) {
  dispatch_xcore_queue_t *dispatch_queue = (dispatch_xcore_queue_t *)ctx;
  dispatch_assert(dispatch_queue);
  dispatch_assert(group);

  dispatch_printf("dispatch_queue_group_add: %u   group=%u\n",
                  (size_t)dispatch_queue, (size_t)group);

  event_counter_t *counter = NULL;

  if (group->waitable) {
    // create event counter
    counter = event_counter_create(group->count);
  }

  for (int i = 0; i < group->count; i++) {
    group->tasks[i]->private_data = counter;

#if defined(use_callers_thread)
    if (busy_workers(dispatch_queue) == dispatch_queue->thread_count) {
      // all the workers are busy and this thread is configured to pitch in
      run_task(group->tasks[i]);
      continue;
    }
#endif

    queue_send(dispatch_queue->queue, (void *)group->tasks[i],
               dispatch_queue->cend);
  }
}

void dispatch_queue_task_wait(dispatch_queue_t *ctx, dispatch_task_t *task) {
  dispatch_assert(task);
  dispatch_assert(task->waitable);

  dispatch_printf("dispatch_queue_task_wait: %u   task=%u\n", (size_t)ctx,
                  (size_t)task);

  if (task->waitable) {
    // wait on the task's event counter to signal that it is complete
    event_counter_t *counter = (event_counter_t *)task->private_data;

    event_counter_wait(counter);
    // the contract is that the dispatch queue must delete waitable tasks
    event_counter_delete(counter);
    dispatch_task_delete(task);
  }
}

void dispatch_queue_group_wait(dispatch_queue_t *ctx, dispatch_group_t *group) {
  dispatch_assert(group);
  dispatch_assert(group->waitable);

  dispatch_printf("dispatch_queue_group_wait: %u   group=%u\n", (size_t)ctx,
                  (size_t)group);

  if (group->waitable) {
    event_counter_t *counter = (event_counter_t *)group->tasks[0]->private_data;
    event_counter_wait(counter);
    // the contract is that the dispatch queue must delete waitable tasks
    event_counter_delete(counter);
    for (int i = 0; i < group->count; i++) {
      dispatch_task_delete(group->tasks[i]);
    }
  }
}

void dispatch_queue_wait(dispatch_queue_t *ctx) {
  dispatch_xcore_queue_t *dispatch_queue = (dispatch_xcore_queue_t *)ctx;
  dispatch_assert(dispatch_queue);

  dispatch_printf("dispatch_queue_wait: %u\n", (size_t)dispatch_queue);

  size_t busy_count;
  size_t waiting_count;

  for (;;) {
    busy_count = busy_workers(dispatch_queue);
    waiting_count = queue_size(dispatch_queue->queue);
    if ((busy_count + waiting_count) == 0) return;
  }
}

void dispatch_queue_delete(dispatch_queue_t *ctx) {
  dispatch_assert(ctx);
  dispatch_xcore_queue_t *dispatch_queue = (dispatch_xcore_queue_t *)ctx;

  dispatch_assert(dispatch_queue);
  dispatch_assert(dispatch_queue->thread_status);
  dispatch_assert(dispatch_queue->worker_data);
  dispatch_assert(dispatch_queue->thread_stack);
  dispatch_assert(dispatch_queue->queue);

  dispatch_printf("dispatch_queue_delete: %u\n", (size_t)dispatch_queue);

  queue_delete(dispatch_queue->queue, dispatch_queue->cend);

  chanend_free(dispatch_queue->cend);

  // free memory
  dispatch_free((void *)dispatch_queue->thread_stack);
  dispatch_free((void *)dispatch_queue->worker_data);
  dispatch_free((void *)dispatch_queue->thread_status);
  dispatch_free((void *)dispatch_queue);
}
