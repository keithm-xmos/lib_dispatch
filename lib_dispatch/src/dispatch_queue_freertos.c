// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "lib_dispatch/api/dispatch_queue_freertos.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "debug_print.h"
#include "lib_dispatch/api/dispatch_group.h"
#include "lib_dispatch/api/dispatch_queue.h"
#include "lib_dispatch/api/dispatch_task.h"

void dispatch_thread_handler(void *param) {
  dispatch_thread_data_t *thread_data = (dispatch_thread_data_t *)param;
  dispatch_task_t task;
  QueueHandle_t xQueue = thread_data->xQueue;
  volatile size_t *task_id = thread_data->task_id;

  debug_printf("dispatch_thread_handler started\n");

  for (;;) {
    if (xQueueReceive(xQueue, &task, portMAX_DELAY)) {
      // set task id
      *task_id = task.id;
      // run task
      dispatch_task_perform(&task);
      // clear task id
      *task_id = DISPATCH_TASK_NONE;
    }
  }
}

dispatch_queue_t *dispatch_queue_create(size_t length, size_t thread_count,
                                        size_t stack_size, const char *name) {
  dispatch_freertos_queue_t *queue;

  debug_printf("dispatch_queue_create: length=%d, thread_count=%d\n", length,
               thread_count);

  queue = (dispatch_freertos_queue_t *)pvPortMalloc(
      sizeof(dispatch_freertos_queue_t));

  queue->length = length;
  queue->thread_count = thread_count;
  queue->thread_stack_size = stack_size;
#if !NDEBUG
  if (name)
    strncpy(queue->name, name, 32);
  else
    strncpy(queue->name, "null", 32);
#endif

  // allocate FreeRTOS queue
  queue->xQueue = xQueueCreate(length, sizeof(dispatch_task_t));

  // allocate threads
  queue->threads = pvPortMalloc(sizeof(TaskHandle_t) * thread_count);

  // allocate thread task ids
  queue->thread_task_ids = pvPortMalloc(sizeof(size_t) * thread_count);

  // allocate thread data
  queue->thread_data =
      pvPortMalloc(sizeof(dispatch_thread_data_t) * thread_count);

  // initialize the queue
  dispatch_queue_init(queue);

  debug_printf("dispatch_queue_create: name=%s\n", queue->name);

  return queue;
}

void dispatch_queue_init(dispatch_queue_t *ctx) {
  assert(ctx);
  dispatch_freertos_queue_t *queue = (dispatch_freertos_queue_t *)ctx;

  debug_printf("dispatch_queue_init: name=%s\n", queue->name);

  // create workers
  for (int i = 0; i < queue->thread_count; i++) {
    queue->thread_task_ids[i] = DISPATCH_TASK_NONE;
    queue->thread_data[i].task_id = &queue->thread_task_ids[i];
    queue->thread_data[i].xQueue = queue->xQueue;
    // create task
    xTaskCreate(dispatch_thread_handler, "", queue->thread_stack_size,
                (void *)&queue->thread_data[i], configMAX_PRIORITIES,
                &queue->threads[i]);
  }
}

void dispatch_queue_async_task(dispatch_queue_t *ctx, dispatch_task_t *task) {
  assert(ctx);
  assert(task);
  dispatch_freertos_queue_t *queue = (dispatch_freertos_queue_t *)ctx;

  debug_printf("dispatch_queue_async_task: name=%s\n", queue->name);

  // assign to this queue
  task->queue = ctx;
  // send to queue
  xQueueSend(queue->xQueue, task, portMAX_DELAY);
}

void dispatch_queue_wait(dispatch_queue_t *ctx) {
  assert(ctx);
  dispatch_freertos_queue_t *queue = (dispatch_freertos_queue_t *)ctx;

  debug_printf("dispatch_queue_wait: name=%s\n", queue->name);

  int busy_count = 0;

  for (;;) {
    busy_count = uxQueueMessagesWaiting(
        queue->xQueue);  // tasks on the queue are considered busy
    for (int i = 0; i < queue->thread_count; i++) {
      if (queue->thread_task_ids[i] != DISPATCH_TASK_NONE) busy_count++;
    }
    // debug_printf("   dispatch_queue_wait: busy_count=%d\n", busy_count);
    if (busy_count == 0) return;
  }
}

void dispatch_queue_task_wait(dispatch_queue_t *ctx, int task_id) {
  assert(ctx);
  assert(task_id > DISPATCH_TASK_NONE);

  dispatch_freertos_queue_t *queue = (dispatch_freertos_queue_t *)ctx;

  bool done_waiting = true;

  for (;;) {
    done_waiting = true;
    for (int i = 0; i < uxQueueMessagesWaiting(queue->xQueue); i++) {
      dispatch_task_t queued_task;
      if (xQueuePeek(queue->xQueue, &queued_task, 10)) {
        if (queued_task.id == task_id) {
          done_waiting = false;
          break;
        }
      }
    }
    for (int i = 0; i < queue->thread_count; i++) {
      if (queue->thread_task_ids[i] == task_id) {
        done_waiting = false;
        break;
      }
    }
    if (done_waiting) break;
  }
}

void dispatch_queue_destroy(dispatch_queue_t *ctx) {
  assert(ctx);
  dispatch_freertos_queue_t *queue = (dispatch_freertos_queue_t *)ctx;

  assert(queue);
  assert(queue->thread_task_ids);
  assert(queue->thread_data);
  assert(queue->threads);

  debug_printf("dispatch_queue_destroy: name=%s\n", queue->name);

  // delete all threads
  for (int i = 0; i < queue->thread_count; i++) {
    vTaskDelete(queue->threads[i]);
  }

  // free memory
  vPortFree((void *)queue->thread_data);
  vPortFree((void *)queue->thread_task_ids);
  vPortFree((void *)queue->threads);
}
