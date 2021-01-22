// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <xcore/assert.h>
#include <xcore/chanend.h>
#include <xcore/channel.h>
#include <xcore/hwtimer.h>
#include <xcore/lock.h>
#include <xcore/thread.h>

#include "circular_buffer_metal.h"
#include "dispatch_config.h"
#include "dispatch_event_counter.h"
#include "dispatch_group.h"
#include "dispatch_queue.h"
#include "dispatch_task.h"
#include "dispatch_types.h"

#define DISPATCH_WAKE_EVT (0x1)
#define DISPATCH_EXIT_EVT (0x2)

#define DISPATCH_BUSY_FLAG (0x0)
#define DISPATCH_READY_FLAG (0x1)

//***********************
//***********************
//***********************
// Worker
//***********************
//***********************
//***********************

typedef struct dispatch_worker_data_struct dispatch_worker_data_t;
struct dispatch_worker_data_struct {
  volatile size_t *flag;
  size_t parent;
  circular_buffer_t *cbuf;
  chanend_t cend;  // FIXME
};

void dispatch_queue_worker(void *param) {
  dispatch_worker_data_t *worker_data = (dispatch_worker_data_t *)param;
  uint8_t evt;
  dispatch_task_t *task = NULL;

  chanend_t cend = worker_data->cend;
  volatile size_t *flag = worker_data->flag;

  dispatch_printf("dispatch_queue_worker started: queue=%u cend=%u\n",
                  worker_data->parent, (int)cend);

  for (;;) {
    evt = chan_in_byte(cend);
    if (evt == DISPATCH_WAKE_EVT) {
      *flag = DISPATCH_BUSY_FLAG;
      dispatch_printf("dispatch_queue_worker wake event: cend=%u\n", (int)cend);
      // read the task
      task = (dispatch_task_t *)chan_in_word(cend);

      // run the task
      dispatch_task_perform(task);

      if (task->waitable) {
        // signal the event counter
        dispatch_event_counter_signal(
            (dispatch_event_counter_t *)task->private_data);
      } else {
        // the contract is that the worker must destroy non-waitable tasks
        dispatch_task_destroy(task);
      }

      *flag = DISPATCH_READY_FLAG;

    } else if (evt == DISPATCH_EXIT_EVT) {
      dispatch_printf("dispatch_queue_worker exit event: cend=%u\n", (int)cend);
      // exit forever loop
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
  chanend_t thread_chanend;  // FIXME
  lock_t lock;
  char *thread_stack;
  size_t *thread_flags;
  dispatch_worker_data_t *worker_data;
};

//***********************
//***********************
//***********************
// Queue implementation
//***********************
//***********************
//***********************

static void task_add(dispatch_xcore_queue_t *queue, dispatch_task_t *task,
                     dispatch_event_counter_t *counter) {
  // find ready worker
  int worker_index = -1;
  for (;;) {
    for (int i = 0; i < queue->thread_count; i++) {
      if (queue->thread_flags[i] == DISPATCH_READY_FLAG) {
        worker_index = i;
        break;
      }
    }
    if (worker_index >= 0) break;
  }

  // set private data
  if (counter) {
    // create event counter
    task->private_data = counter;
  }

  // wire up route
  chanend_set_dest(queue->thread_chanend,
                   queue->worker_data[worker_index].cend);
  chanend_set_dest(queue->worker_data[worker_index].cend,
                   queue->thread_chanend);
  // signal worker to wake up (blocks waiting for worker)
  chan_out_byte(queue->thread_chanend, DISPATCH_WAKE_EVT);
  // send task to worker
  chan_out_word(queue->thread_chanend, (int)task);
}

dispatch_queue_t *dispatch_queue_create(size_t length, size_t thread_count,
                                        size_t thread_stack_size,
                                        size_t thread_priority) {
  dispatch_xcore_queue_t *queue;

  dispatch_printf("dispatch_queue_create: length=%d, thread_count=%d\n", length,
                  thread_count);

  queue = (dispatch_xcore_queue_t *)malloc(sizeof(dispatch_xcore_queue_t));

  queue->length = length;
  queue->thread_count = thread_count;

  // allocate thread task ids
  queue->thread_flags = malloc(sizeof(size_t) * thread_count);

  // allocate thread data
  queue->worker_data = malloc(sizeof(dispatch_worker_data_t) * thread_count);

  // allocate thread stack
  queue->thread_stack_size = thread_stack_size;
  queue->thread_stack = malloc(queue->thread_stack_size * thread_count);

  // initialize the queue
  dispatch_queue_init(queue, thread_priority);

  dispatch_printf("dispatch_queue_create: created queue=%u\n", (size_t)queue);

  return queue;
}

void dispatch_queue_init(dispatch_queue_t *ctx, size_t thread_priority) {
  dispatch_xcore_queue_t *queue = (dispatch_xcore_queue_t *)ctx;
  dispatch_assert(queue);

  dispatch_printf("dispatch_queue_init: queue=%u\n", (size_t)queue);

  int stack_offset = 0;

  queue->thread_chanend = chanend_alloc();  // dispatch queue's chanend
  queue->lock = lock_alloc();               // lock used for all synchronization

  // create workers
  for (int i = 0; i < queue->thread_count; i++) {
    dispatch_printf("dispatch_queue_init: i=%d\n", i);
    queue->thread_flags[i] = DISPATCH_READY_FLAG;
    queue->worker_data[i].flag = &queue->thread_flags[i];
    // create and setup chanends
    queue->worker_data[i].parent = (size_t)queue;
    queue->worker_data[i].cend = chanend_alloc();  // worker's chanend
    // launch the thread worker
    run_async(dispatch_queue_worker, (void *)&queue->worker_data[i],
              stack_base((void *)&queue->thread_stack[stack_offset],
                         queue->thread_stack_size));
    stack_offset += queue->thread_stack_size * sizeof(int);
  }
}

void dispatch_queue_task_add(dispatch_queue_t *ctx, dispatch_task_t *task) {
  dispatch_xcore_queue_t *queue = (dispatch_xcore_queue_t *)ctx;
  dispatch_assert(queue);
  dispatch_assert(task);

  dispatch_printf("dispatch_queue_task_add: queue=%u\n", (size_t)queue);

  dispatch_event_counter_t *counter = NULL;

  if (task->waitable) {
    // create event counter
    //   re-use the queue's lock given hardware lock are so scarce
    counter = dispatch_event_counter_create(1, queue->lock);
  }
  task_add(queue, task, counter);
}

void dispatch_queue_group_add(dispatch_queue_t *ctx, dispatch_group_t *group) {
  dispatch_xcore_queue_t *queue = (dispatch_xcore_queue_t *)ctx;
  dispatch_assert(queue);
  dispatch_assert(group);

  dispatch_printf("dispatch_queue_group_add: queue=%u   group=%u\n",
                  (size_t)queue, (size_t)group);

  dispatch_event_counter_t *counter = NULL;

  if (group->waitable) {
    // create event counter
    counter = dispatch_event_counter_create(group->count, queue->lock);
  }

  for (int i = 0; i < group->count; i++) {
    task_add(queue, group->tasks[i], counter);
  }
}

void dispatch_queue_task_wait(dispatch_queue_t *ctx, dispatch_task_t *task) {
  dispatch_assert(task);
  dispatch_assert(task->waitable);

  dispatch_printf("dispatch_queue_task_wait: queue=%u   task=%u\n", (size_t)ctx,
                  (size_t)task);

  if (task->waitable) {
    // wait on the task's event counter to signal that it is complete
    dispatch_event_counter_t *counter =
        (dispatch_event_counter_t *)task->private_data;
    dispatch_event_counter_wait(counter);
    // the contract is that the dispatch queue must destroy waitable tasks
    dispatch_event_counter_destroy(counter);
    dispatch_task_destroy(task);
  }
}

void dispatch_queue_group_wait(dispatch_queue_t *ctx, dispatch_group_t *group) {
  dispatch_assert(group);
  dispatch_assert(group->waitable);

  dispatch_printf("dispatch_queue_group_wait: queue=%u   group=%u\n",
                  (size_t)ctx, (size_t)group);

  if (group->waitable) {
    dispatch_event_counter_t *counter =
        (dispatch_event_counter_t *)group->tasks[0]->private_data;
    dispatch_event_counter_wait(counter);
    // the contract is that the dispatch queue must destroy waitable tasks
    dispatch_event_counter_destroy(counter);
    for (int i = 0; i < group->count; i++) {
      dispatch_task_destroy(group->tasks[i]);
    }
  }
}

void dispatch_queue_wait(dispatch_queue_t *ctx) {
  dispatch_xcore_queue_t *queue = (dispatch_xcore_queue_t *)ctx;
  dispatch_assert(queue);

  dispatch_printf("dispatch_queue_wait: queue=%u\n", (size_t)queue);

  int busy_count;

  for (;;) {
    busy_count = 0;
    for (int i = 0; i < queue->thread_count; i++) {
      if (queue->thread_flags[i] == DISPATCH_BUSY_FLAG) busy_count++;
    }
    if (busy_count == 0) return;
  }
}

void dispatch_queue_destroy(dispatch_queue_t *ctx) {
  dispatch_assert(ctx);
  dispatch_xcore_queue_t *queue = (dispatch_xcore_queue_t *)ctx;

  dispatch_assert(queue);
  dispatch_assert(queue->thread_flags);
  dispatch_assert(queue->worker_data);
  dispatch_assert(queue->thread_stack);

  dispatch_printf("dispatch_queue_destroy: queue=%u\n", (size_t)queue);

  // send all thread workers the EXIT event
  for (int i = 0; i < queue->thread_count; i++) {
    chanend_set_dest(queue->thread_chanend, queue->worker_data[i].cend);
    chanend_set_dest(queue->worker_data[i].cend, queue->thread_chanend);
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
    chanend_free(queue->worker_data[i].cend);
  }

  lock_free(queue->lock);

  // free memory
  free((void *)queue->thread_stack);
  free((void *)queue->worker_data);
  free((void *)queue->thread_flags);
  free((void *)queue);
}
