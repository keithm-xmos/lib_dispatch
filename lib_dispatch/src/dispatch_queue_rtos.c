// Copyright 2021 XMOS LIMITED. This Software is subject to the terms of the 
// XMOS Public License: Version 1
//#include "dispatch_queue_rtos.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "dispatch_config.h"
#include "dispatch_group.h"
#include "dispatch_queue.h"
#include "dispatch_task.h"
#include "dispatch_types.h"
#include "event_counter.h"
#include "event_groups.h"
#include "queue.h"
#include "task.h"

//***********************
//***********************
//***********************
// Worker
//***********************
//***********************
//***********************
typedef struct dispatch_worker_data_struct dispatch_worker_data_t;
struct dispatch_worker_data_struct {
  size_t parent;
  QueueHandle_t xQueue;
  EventGroupHandle_t xEventGroup;
  EventBits_t xReadyBit;
};

void dispatch_queue_worker(void *param) {
  dispatch_worker_data_t *worker_data = (dispatch_worker_data_t *)param;

  QueueHandle_t xQueue = worker_data->xQueue;
  EventGroupHandle_t xEventGroup = worker_data->xEventGroup;
  EventBits_t xReadyBit = worker_data->xReadyBit;

  dispatch_task_t *task = NULL;

  dispatch_printf("dispatch_queue_worker started: parent=%u\n",
                  worker_data->parent);

  for (;;) {
    if (xQueueReceive(xQueue, &task, portMAX_DELAY)) {
      // unset ready bit
      xEventGroupClearBits(xEventGroup, xReadyBit);
      // run task
      dispatch_task_perform(task);
      if (task->waitable) {
        // signal the event counter
        event_counter_signal((event_counter_t *)task->private_data);
        // clear semaphore
      } else {
        // the contract is that the worker must delete non-waitable tasks
        dispatch_task_delete(task);
      }
      // set ready bit
      xEventGroupSetBits(xEventGroup, xReadyBit);
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

typedef struct dispatch_freertos_struct dispatch_freertos_queue_t;
struct dispatch_freertos_struct {
  size_t thread_count;
  size_t thread_stack_size;
  QueueHandle_t xQueue;
  EventGroupHandle_t xEventGroup;
  EventBits_t xReadyBits;
  dispatch_worker_data_t *worker_data;
  TaskHandle_t *threads;
};

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
  dispatch_freertos_queue_t *dispatch_queue;

  dispatch_printf("dispatch_queue_create: length=%d, thread_count=%d\n", length,
                  thread_count);

  dispatch_queue = pvPortMalloc(sizeof(dispatch_freertos_queue_t));

  dispatch_queue->thread_count = thread_count;
  dispatch_queue->thread_stack_size = thread_stack_size;

  // allocate FreeRTOS queue
  dispatch_queue->xQueue = xQueueCreate(length, sizeof(dispatch_task_t *));

  // allocate threads
  dispatch_queue->threads = pvPortMalloc(sizeof(TaskHandle_t) * thread_count);

  // allocate thread data
  dispatch_queue->worker_data =
      pvPortMalloc(sizeof(dispatch_worker_data_t) * thread_count);

  dispatch_queue->xEventGroup = xEventGroupCreate();

  // initialize the queue
  dispatch_queue_init(dispatch_queue, thread_priority);

  dispatch_printf("dispatch_queue_create: %u\n", (size_t)dispatch_queue);

  return dispatch_queue;
}

void dispatch_queue_init(dispatch_queue_t *ctx, size_t thread_priority) {
  dispatch_freertos_queue_t *dispatch_queue = (dispatch_freertos_queue_t *)ctx;
  dispatch_assert(dispatch_queue);

  dispatch_printf("dispatch_queue_init: %u\n", (size_t)dispatch_queue);

  // create workers
  for (int i = 0; i < dispatch_queue->thread_count; i++) {
    dispatch_queue->worker_data[i].parent = (size_t)dispatch_queue;
    dispatch_queue->worker_data[i].xQueue = dispatch_queue->xQueue;
    dispatch_queue->worker_data[i].xEventGroup = dispatch_queue->xEventGroup;
    dispatch_queue->worker_data[i].xReadyBit = 1 << i;
    // create task
    xTaskCreate(dispatch_queue_worker, "", dispatch_queue->thread_stack_size,
                (void *)&dispatch_queue->worker_data[i], thread_priority,
                &dispatch_queue->threads[i]);
  }
  // set the ready bits
  dispatch_queue->xReadyBits =
      0xFFFFFFFF >> (32 - dispatch_queue->thread_count);
  xEventGroupSetBits(dispatch_queue->xEventGroup, dispatch_queue->xReadyBits);
}

void dispatch_queue_task_add(dispatch_queue_t *ctx, dispatch_task_t *task) {
  dispatch_freertos_queue_t *dispatch_queue = (dispatch_freertos_queue_t *)ctx;
  dispatch_assert(dispatch_queue);
  dispatch_assert(task);

  dispatch_printf("dispatch_queue_add_task: %u   task=%u\n",
                  (size_t)dispatch_queue, (size_t)task);

  if (task->waitable) {
    task->private_data = event_counter_create(1);
  }

  // send to queue
  xQueueSend(dispatch_queue->xQueue, (void *)&task, portMAX_DELAY);
}

void dispatch_queue_group_add(dispatch_queue_t *ctx, dispatch_group_t *group) {
  dispatch_freertos_queue_t *dispatch_queue = (dispatch_freertos_queue_t *)ctx;
  dispatch_assert(dispatch_queue);
  dispatch_assert(group);

  dispatch_printf("dispatch_queue_group_add: %u   group=%u\n",
                  (size_t)dispatch_queue, (size_t)group);

  event_counter_t *counter = NULL;

  if (group->waitable) {
    // create event counter
    counter = event_counter_create(group->count);
  }

  // send to queue
  for (int i = 0; i < group->count; i++) {
    group->tasks[i]->private_data = counter;
    xQueueSend(dispatch_queue->xQueue, (void *)&group->tasks[i], portMAX_DELAY);
  }
}

void dispatch_queue_task_wait(dispatch_queue_t *ctx, dispatch_task_t *task) {
  dispatch_assert(task);
  dispatch_assert(task->waitable);

  dispatch_printf("dispatch_queue_task_wait: %u   task=%u\n", (size_t)ctx,
                  (size_t)task);

  if (task->waitable) {
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
    event_counter_delete(counter);
    for (int i = 0; i < group->count; i++) {
      dispatch_task_delete(group->tasks[i]);
    }
  }
}

void dispatch_queue_wait(dispatch_queue_t *ctx) {
  dispatch_freertos_queue_t *dispatch_queue = (dispatch_freertos_queue_t *)ctx;
  dispatch_assert(dispatch_queue);

  dispatch_printf("dispatch_queue_wait: %u\n", (size_t)dispatch_queue);

  int waiting_count;

  // busywait for xQueue to empty
  for (;;) {
    waiting_count = uxQueueMessagesWaiting(dispatch_queue->xQueue);
    if (waiting_count == 0) break;
  }
  // wait for all ready bits to be set
  xEventGroupWaitBits(dispatch_queue->xEventGroup, dispatch_queue->xReadyBits,
                      pdFALSE, pdTRUE, portMAX_DELAY);
}

void dispatch_queue_delete(dispatch_queue_t *ctx) {
  dispatch_freertos_queue_t *dispatch_queue = (dispatch_freertos_queue_t *)ctx;
  dispatch_assert(dispatch_queue);

  dispatch_printf("dispatch_queue_delete: %u\n", (size_t)dispatch_queue);

  // delete all threads
  for (int i = 0; i < dispatch_queue->thread_count; i++) {
    vTaskDelete(dispatch_queue->threads[i]);
  }

  // free memory
  vPortFree((void *)dispatch_queue->worker_data);
  vPortFree((void *)dispatch_queue->threads);
  vEventGroupDelete(dispatch_queue->xEventGroup);
  vQueueDelete(dispatch_queue->xQueue);
}
