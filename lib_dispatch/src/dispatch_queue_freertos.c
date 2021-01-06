// Copyright (c) 2020, XMOS Ltd, All rights reserved
//#include "dispatch_queue_freertos.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "dispatch_config.h"
#include "dispatch_group.h"
#include "dispatch_queue.h"
#include "dispatch_task.h"
#include "dispatch_types.h"
#include "event_groups.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

typedef struct dispatch_thread_data_struct dispatch_thread_data_t;
struct dispatch_thread_data_struct {
  size_t parent;
  QueueHandle_t xQueue;
  EventGroupHandle_t xEventGroup;
  EventBits_t xReadyBit;
};

typedef struct dispatch_freertos_struct dispatch_freertos_queue_t;
struct dispatch_freertos_struct {
  size_t length;
  size_t thread_count;
  size_t thread_stack_size;
  QueueHandle_t xQueue;
  EventGroupHandle_t xEventGroup;
  EventBits_t xReadyBits;
  dispatch_thread_data_t *thread_data;
  TaskHandle_t *threads;
};

void dispatch_thread_handler(void *param) {
  dispatch_thread_data_t *thread_data = (dispatch_thread_data_t *)param;

  QueueHandle_t xQueue = thread_data->xQueue;
  EventGroupHandle_t xEventGroup = thread_data->xEventGroup;
  EventBits_t xReadyBit = thread_data->xReadyBit;

  dispatch_task_t *task = NULL;

  dispatch_printf("dispatch_thread_handler started: queue=%u\n",
                  thread_data->parent);

  for (;;) {
    if (xQueueReceive(xQueue, &task, portMAX_DELAY)) {
      // unset ready bit
      xEventGroupClearBits(xEventGroup, xReadyBit);
      // run task
      dispatch_task_perform(task);
      if (task->waitable) {
        // clear semaphore
        xSemaphoreGive((SemaphoreHandle_t)task->private);
      } else {
        // the contract is that the worker must destroy non-waitable tasks
        dispatch_task_destroy(task);
      }
      // set ready bit
      xEventGroupSetBits(xEventGroup, xReadyBit);
    }
  }
}

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
  queue->thread_data =
      pvPortMalloc(sizeof(dispatch_thread_data_t) * thread_count);

  queue->xEventGroup = xEventGroupCreate();

  // initialize the queue
  dispatch_queue_init(queue, thread_priority);

  dispatch_printf("dispatch_queue_create: created queue=%u\n", (size_t)queue);

  return queue;
}

void dispatch_queue_init(dispatch_queue_t *ctx, size_t thread_priority) {
  dispatch_assert(ctx);
  dispatch_freertos_queue_t *queue = (dispatch_freertos_queue_t *)ctx;

  dispatch_printf("dispatch_queue_init: queue=%u\n", (size_t)queue);

  // create workers
  EventBits_t xReadyBit;
  for (int i = 0; i < queue->thread_count; i++) {
    xReadyBit = 1 << i;
    queue->thread_data[i].parent = (size_t)queue;
    queue->thread_data[i].xQueue = queue->xQueue;
    queue->thread_data[i].xEventGroup = queue->xEventGroup;
    queue->thread_data[i].xReadyBit = xReadyBit;
    queue->xReadyBits |= xReadyBit;
    // create task
    xTaskCreate(dispatch_thread_handler, "", queue->thread_stack_size,
                (void *)&queue->thread_data[i], thread_priority,
                &queue->threads[i]);
  }
  // set the ready bits
  xEventGroupSetBits(queue->xEventGroup, queue->xReadyBits);
}

void dispatch_queue_task_add(dispatch_queue_t *ctx, dispatch_task_t *task) {
  dispatch_assert(ctx);
  dispatch_assert(task);
  dispatch_freertos_queue_t *queue = (dispatch_freertos_queue_t *)ctx;

  if (task->waitable) {
    task->private = xSemaphoreCreateBinary();
  }

  dispatch_printf("dispatch_queue_add_task: queue=%u   task=%u\n",
                  (size_t)queue, (size_t)task);

  // send to queue
  xQueueSend(queue->xQueue, (void *)&task, portMAX_DELAY);
}

void dispatch_queue_wait(dispatch_queue_t *ctx) {
  dispatch_assert(ctx);
  dispatch_freertos_queue_t *queue = (dispatch_freertos_queue_t *)ctx;

  dispatch_printf("dispatch_queue_wait: queue=%u\n", (size_t)queue);

  int waiting_count;

  for (;;) {
    waiting_count = uxQueueMessagesWaiting(queue->xQueue);
    if (waiting_count == 0) break;
  }
  // now wait for all ready bts to be set
  xEventGroupWaitBits(queue->xEventGroup, queue->xReadyBits, pdFALSE, pdTRUE,
                      portMAX_DELAY);
}

void dispatch_queue_task_wait(dispatch_queue_t *ctx, dispatch_task_t *task) {
  dispatch_assert(task);
  dispatch_assert(task->waitable);

  if (task->waitable) {
    SemaphoreHandle_t semaphore = (SemaphoreHandle_t)task->private;
    // wait on the task's semaphore which signals that it is complete
    xSemaphoreTake(semaphore, portMAX_DELAY);
    // the contract is that the dispatch queue must destroy waitable tasks
    vSemaphoreDelete(semaphore);
    dispatch_task_destroy(task);
  }
}

void dispatch_queue_destroy(dispatch_queue_t *ctx) {
  dispatch_freertos_queue_t *queue = (dispatch_freertos_queue_t *)ctx;

  dispatch_assert(queue);

  dispatch_printf("dispatch_queue_destroy: queue=%u\n", (size_t)queue);

  // free memory

  // delete all threads
  for (int i = 0; i < queue->thread_count; i++) {
    vTaskDelete(queue->threads[i]);
  }

  vPortFree((void *)queue->thread_data);
  vPortFree((void *)queue->threads);
  vEventGroupDelete(queue->xEventGroup);
  vQueueDelete(queue->xQueue);
}
