// Copyright (c) 2020, XMOS Ltd, All rights reserved
//#include "dispatch_queue_freertos.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "dispatch_config.h"
#include "dispatch_event_counter.h"
#include "dispatch_group.h"
#include "dispatch_queue.h"
#include "dispatch_task.h"
#include "dispatch_types.h"
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

  dispatch_printf("dispatch_queue_worker started: queue=%u\n",
                  worker_data->parent);

  for (;;) {
    if (xQueueReceive(xQueue, &task, portMAX_DELAY)) {
      // unset ready bit
      xEventGroupClearBits(xEventGroup, xReadyBit);
      // run task
      dispatch_task_perform(task);
      if (task->waitable) {
        // signal the event counter
        dispatch_event_counter_signal(
            (dispatch_event_counter_t *)task->private_data);
        // clear semaphore
      } else {
        // the contract is that the worker must destroy non-waitable tasks
        dispatch_task_destroy(task);
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
  size_t length;
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
  dispatch_freertos_queue_t *queue;

  dispatch_printf("dispatch_queue_create: length=%d, thread_count=%d\n", length,
                  thread_count);

  queue = pvPortMalloc(sizeof(dispatch_freertos_queue_t));

  queue->length = length;
  queue->thread_count = thread_count;
  queue->thread_stack_size = thread_stack_size;

  // allocate FreeRTOS queue
  queue->xQueue = xQueueCreate(length, sizeof(dispatch_task_t *));

  // allocate threads
  queue->threads = pvPortMalloc(sizeof(TaskHandle_t) * thread_count);

  // allocate thread data
  queue->worker_data =
      pvPortMalloc(sizeof(dispatch_worker_data_t) * thread_count);

  queue->xEventGroup = xEventGroupCreate();

  // initialize the queue
  dispatch_queue_init(queue, thread_priority);

  dispatch_printf("dispatch_queue_create: created queue=%u\n", (size_t)queue);

  return queue;
}

void dispatch_queue_init(dispatch_queue_t *ctx, size_t thread_priority) {
  dispatch_freertos_queue_t *queue = (dispatch_freertos_queue_t *)ctx;
  dispatch_assert(queue);

  dispatch_printf("dispatch_queue_init: queue=%u\n", (size_t)queue);

  // create workers
  for (int i = 0; i < queue->thread_count; i++) {
    queue->worker_data[i].parent = (size_t)queue;
    queue->worker_data[i].xQueue = queue->xQueue;
    queue->worker_data[i].xEventGroup = queue->xEventGroup;
    queue->worker_data[i].xReadyBit = 1 << i;
    // create task
    xTaskCreate(dispatch_queue_worker, "", queue->thread_stack_size,
                (void *)&queue->worker_data[i], thread_priority,
                &queue->threads[i]);
  }
  // set the ready bits
  queue->xReadyBits = 0xFFFFFFFF >> (32 - queue->thread_count);
  xEventGroupSetBits(queue->xEventGroup, queue->xReadyBits);
}

void dispatch_queue_task_add(dispatch_queue_t *ctx, dispatch_task_t *task) {
  dispatch_freertos_queue_t *queue = (dispatch_freertos_queue_t *)ctx;
  dispatch_assert(queue);
  dispatch_assert(task);

  dispatch_printf("dispatch_queue_add_task: queue=%u   task=%u\n",
                  (size_t)queue, (size_t)task);

  if (task->waitable) {
    task->private_data = dispatch_event_counter_create(1, NULL);
  }

  // send to queue
  xQueueSend(queue->xQueue, (void *)&task, portMAX_DELAY);
}

void dispatch_queue_group_add(dispatch_queue_t *ctx, dispatch_group_t *group) {
  dispatch_freertos_queue_t *queue = (dispatch_freertos_queue_t *)ctx;
  dispatch_assert(queue);
  dispatch_assert(group);

  dispatch_printf("dispatch_queue_group_add: queue=%u   group=%u\n",
                  (size_t)queue, (size_t)group);

  dispatch_event_counter_t *counter = NULL;

  if (group->waitable) {
    // create event counter
    counter = dispatch_event_counter_create(group->count, NULL);
  }

  // send to queue
  for (int i = 0; i < group->count; i++) {
    group->tasks[i]->private_data = counter;
    xQueueSend(queue->xQueue, (void *)&group->tasks[i], portMAX_DELAY);
  }
}

void dispatch_queue_task_wait(dispatch_queue_t *ctx, dispatch_task_t *task) {
  dispatch_assert(task);
  dispatch_assert(task->waitable);

  dispatch_printf("dispatch_queue_task_wait: queue=%u   task=%u\n", (size_t)ctx,
                  (size_t)task);

  if (task->waitable) {
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
    dispatch_event_counter_destroy(counter);
    for (int i = 0; i < group->count; i++) {
      dispatch_task_destroy(group->tasks[i]);
    }
  }
}

void dispatch_queue_wait(dispatch_queue_t *ctx) {
  dispatch_freertos_queue_t *queue = (dispatch_freertos_queue_t *)ctx;
  dispatch_assert(queue);

  dispatch_printf("dispatch_queue_wait: queue=%u\n", (size_t)queue);

  int waiting_count;

  // busywait for xQueue to empty
  for (;;) {
    waiting_count = uxQueueMessagesWaiting(queue->xQueue);
    if (waiting_count == 0) break;
  }
  // wait for all ready bts to be set
  xEventGroupWaitBits(queue->xEventGroup, queue->xReadyBits, pdFALSE, pdTRUE,
                      portMAX_DELAY);
}

void dispatch_queue_destroy(dispatch_queue_t *ctx) {
  dispatch_freertos_queue_t *queue = (dispatch_freertos_queue_t *)ctx;
  dispatch_assert(queue);

  dispatch_printf("dispatch_queue_destroy: queue=%u\n", (size_t)queue);

  // delete all threads
  for (int i = 0; i < queue->thread_count; i++) {
    vTaskDelete(queue->threads[i]);
  }

  // free memory
  vPortFree((void *)queue->worker_data);
  vPortFree((void *)queue->threads);
  vEventGroupDelete(queue->xEventGroup);
  vQueueDelete(queue->xQueue);
}
