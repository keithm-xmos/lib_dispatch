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

#include "dispatch_config.h"
#include "dispatch_event_counter.h"
#include "dispatch_group.h"
#include "dispatch_queue.h"
#include "dispatch_task.h"
#include "dispatch_types.h"
#include "queue_metal.h"

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
  queue_t *queue;
  //  chanend_t cend;
};

// void dispatch_queue_worker(void *param) {
//   dispatch_worker_data_t *worker_data = (dispatch_worker_data_t *)param;
//   uint8_t evt;
//   dispatch_task_t *task = NULL;

//   chanend_t cend = worker_data->cend;
//   volatile size_t *flag = worker_data->flag;

//   dispatch_printf("dispatch_queue_worker started: parent=%u cend=%u\n",
//                   worker_data->parent, (int)cend);

//   for (;;) {
//     evt = chan_in_byte(cend);
//     if (evt == DISPATCH_WAKE_EVT) {
//       *flag = DISPATCH_BUSY_FLAG;
//       dispatch_printf("dispatch_queue_worker wake event: cend=%u\n",
//       (int)cend);
//       // read the task
//       task = (dispatch_task_t *)chan_in_word(cend);

//       // run the task
//       dispatch_task_perform(task);

//       if (task->waitable) {
//         // signal the event counter
//         dispatch_event_counter_signal(
//             (dispatch_event_counter_t *)task->private_data);
//       } else {
//         // the contract is that the worker must destroy non-waitable tasks
//         dispatch_task_destroy(task);
//       }

//       *flag = DISPATCH_READY_FLAG;

//     } else if (evt == DISPATCH_EXIT_EVT) {
//       dispatch_printf("dispatch_queue_worker exit event: cend=%u\n",
//       (int)cend);
//       // exit forever loop
//       break;
//     }
//   }
// }

void dispatch_queue_worker(void *param) {
  dispatch_worker_data_t *worker_data = (dispatch_worker_data_t *)param;
  // uint8_t evt;
  dispatch_task_t *task = NULL;

  queue_t *queue = worker_data->queue;
  // volatile size_t *flag = worker_data->flag;

  dispatch_printf("dispatch_queue_worker started: parent=%u\n",
                  worker_data->parent);

  for (;;) {
    if (queue_receive(queue, (void **)&task)) {
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
  // chanend_t thread_chanend;  // FIXME
  queue_t *queue;
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

// static void task_add(dispatch_xcore_queue_t *dispatch_queue,
// dispatch_task_t *task,
//                      dispatch_event_counter_t *counter) {
//   // find ready worker
//   int worker_index = -1;
//   for (;;) {
//     for (int i = 0; i < dispatch_queue->thread_count; i++) {
//       if (dispatch_queue->thread_flags[i] == DISPATCH_READY_FLAG) {
//         worker_index = i;
//         break;
//       }
//     }
//     if (worker_index >= 0) break;
//   }

//   // set private data
//   if (counter) {
//     // create event counter
//     task->private_data = counter;
//   }

//   // wire up route
//   chanend_set_dest(dispatch_queue->thread_chanend,
//                    dispatch_queue->worker_data[worker_index].cend);
//   chanend_set_dest(dispatch_queue->worker_data[worker_index].cend,
//                    dispatch_queue->thread_chanend);
//   // signal worker to wake up (blocks waiting for worker)
//   chan_out_byte(dispatch_queue->thread_chanend, DISPATCH_WAKE_EVT);
//   // send task to worker
//   chan_out_word(dispatch_queue->thread_chanend, (int)task);
// }

dispatch_queue_t *dispatch_queue_create(size_t length, size_t thread_count,
                                        size_t thread_stack_size,
                                        size_t thread_priority) {
  dispatch_xcore_queue_t *dispatch_queue;

  dispatch_printf("dispatch_queue_create: length=%d, thread_count=%d\n", length,
                  thread_count);

  dispatch_queue =
      (dispatch_xcore_queue_t *)malloc(sizeof(dispatch_xcore_queue_t));

  dispatch_queue->length = length;
  dispatch_queue->thread_count = thread_count;
  dispatch_queue->lock = lock_alloc();  // lock used for all synchronization

  // allocate  queue
  dispatch_queue->queue = queue_create(length, dispatch_queue->lock);

  // allocate thread task ids
  dispatch_queue->thread_flags = malloc(sizeof(size_t) * thread_count);

  // allocate thread data
  dispatch_queue->worker_data =
      malloc(sizeof(dispatch_worker_data_t) * thread_count);

  // allocate thread stack
  dispatch_queue->thread_stack_size = thread_stack_size;
  dispatch_queue->thread_stack =
      malloc(dispatch_queue->thread_stack_size * thread_count);

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

  // dispatch_queue->thread_chanend = chanend_alloc();  // dispatch queue's
  // chanend

  // create workers
  for (int i = 0; i < dispatch_queue->thread_count; i++) {
    dispatch_printf("dispatch_queue_init: i=%d\n", i);
    dispatch_queue->thread_flags[i] = DISPATCH_READY_FLAG;
    dispatch_queue->worker_data[i].flag = &dispatch_queue->thread_flags[i];
    // create and setup chanends
    dispatch_queue->worker_data[i].parent = (size_t)dispatch_queue;
    // dispatch_queue->worker_data[i].cend = chanend_alloc();  // worker's
    // chanend
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

  // dispatch_event_counter_t *counter = NULL;

  if (task->waitable) {
    // create event counter
    //   re-use the queue's lock given hardware lock are so scarce
    task->private_data = dispatch_event_counter_create(1, dispatch_queue->lock);
  }

  // task_add(queue, task, counter);
  queue_send(dispatch_queue->queue, (void *)task);
}

void dispatch_queue_group_add(dispatch_queue_t *ctx, dispatch_group_t *group) {
  dispatch_xcore_queue_t *dispatch_queue = (dispatch_xcore_queue_t *)ctx;
  dispatch_assert(dispatch_queue);
  dispatch_assert(group);

  dispatch_printf("dispatch_queue_group_add: %u   group=%u\n",
                  (size_t)dispatch_queue, (size_t)group);

  dispatch_event_counter_t *counter = NULL;

  if (group->waitable) {
    // create event counter
    counter = dispatch_event_counter_create(group->count, dispatch_queue->lock);
  }

  for (int i = 0; i < group->count; i++) {
    // task_add(queue, group->tasks[i], counter);
    group->tasks[i]->private_data = counter;
    queue_send(dispatch_queue->queue, (void *)group->tasks[i]);
  }
}

void dispatch_queue_task_wait(dispatch_queue_t *ctx, dispatch_task_t *task) {
  dispatch_assert(task);
  dispatch_assert(task->waitable);

  dispatch_printf("dispatch_queue_task_wait: %u   task=%u\n", (size_t)ctx,
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

  dispatch_printf("dispatch_queue_group_wait: %u   group=%u\n", (size_t)ctx,
                  (size_t)group);

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
  dispatch_xcore_queue_t *dispatch_queue = (dispatch_xcore_queue_t *)ctx;
  dispatch_assert(dispatch_queue);

  dispatch_printf("dispatch_queue_wait: %u\n", (size_t)dispatch_queue);

  // int busy_count;

  // for (;;) {
  //   busy_count = 0;
  //   for (int i = 0; i < dispatch_queue->thread_count; i++) {
  //     if (dispatch_queue->thread_flags[i] == DISPATCH_BUSY_FLAG)
  //     busy_count++;
  //   }
  //   if (busy_count == 0) return;
  // }
}

void dispatch_queue_destroy(dispatch_queue_t *ctx) {
  dispatch_assert(ctx);
  dispatch_xcore_queue_t *dispatch_queue = (dispatch_xcore_queue_t *)ctx;

  dispatch_assert(dispatch_queue);
  dispatch_assert(dispatch_queue->thread_flags);
  dispatch_assert(dispatch_queue->worker_data);
  dispatch_assert(dispatch_queue->thread_stack);
  dispatch_assert(dispatch_queue->queue);

  dispatch_printf("dispatch_queue_destroy: %u\n", (size_t)dispatch_queue);

  // send all thread workers the EXIT event
  // for (int i = 0; i < dispatch_queue->thread_count; i++) {
  //   chanend_set_dest(dispatch_queue->thread_chanend,
  //                    dispatch_queue->worker_data[i].cend);
  //   chanend_set_dest(dispatch_queue->worker_data[i].cend,
  //                    dispatch_queue->thread_chanend);
  //   chan_out_byte(dispatch_queue->thread_chanend, DISPATCH_EXIT_EVT);
  // }

  // need to give task handlers time to exit
  // unsigned magic_duration = 10000000;
  // hwtimer_t timer = hwtimer_alloc();
  // unsigned time = hwtimer_get_time(timer);
  // hwtimer_wait_until(timer, time + magic_duration);
  // hwtimer_free(timer);

  // now safe to free the chanends
  // chanend_free(dispatch_queue->thread_chanend);
  // for (int i = 0; i < dispatch_queue->thread_count; i++) {
  //   chanend_free(dispatch_queue->worker_data[i].cend);
  // }

  lock_free(dispatch_queue->lock);

  queue_destroy(dispatch_queue->queue);

  // free memory
  free((void *)dispatch_queue->thread_stack);
  free((void *)dispatch_queue->worker_data);
  free((void *)dispatch_queue->thread_flags);
}
