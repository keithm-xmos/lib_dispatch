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

  queue->next_id = DISPATCH_TASK_NONE + 1;

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

size_t dispatch_queue_add_task(dispatch_queue_t *ctx, dispatch_task_t *task) {
  assert(ctx);
  assert(task);
  dispatch_freertos_queue_t *queue = (dispatch_freertos_queue_t *)ctx;

  // assign to this queue
  task->queue = ctx;
  task->id = queue->next_id++;

  debug_printf("dispatch_queue_add_task: name=%s   task=%d\n", queue->name,
               task->id);

  // send to queue
  xQueueSend(queue->xQueue, task, portMAX_DELAY);

  return task->id;
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
      if (VALID_TASK_ID(queue->thread_task_ids[i])) busy_count++;
    }
    if (busy_count == 0) return;
  }
}

void dispatch_queue_task_wait(dispatch_queue_t *ctx, int task_id) {
  assert(ctx);
  assert(VALID_TASK_ID(task_id));

  dispatch_freertos_queue_t *queue = (dispatch_freertos_queue_t *)ctx;
  assert(task_id <= queue->next_id);

  debug_printf("dispatch_queue_task_wait: queue=%s   task=%d\n", queue->name,
               task_id);

  bool task_is_running;
  bool tasks_are_waiting;
  size_t num_running_tasks;
  size_t min_running_task_id;
  size_t front_task_id;
  dispatch_task_t front_task;

  for (;;) {
    num_running_tasks = 0;
    task_is_running = false;

    // peek at the task at the front of the queue
    if (xQueuePeek(queue->xQueue, &front_task, 0)) {
      tasks_are_waiting = true;
      front_task_id = front_task.id;
      min_running_task_id = front_task_id;  // it must be lower
    } else {
      tasks_are_waiting = false;
      front_task_id = DISPATCH_TASK_NONE;
      min_running_task_id = queue->next_id;  // it must be lower, worst case
    }

    // check currently running tasks
    for (int i = 0; i < queue->thread_count; i++) {
      if (VALID_TASK_ID(queue->thread_task_ids[i])) {
        num_running_tasks++;
        min_running_task_id =
            MIN_TASK_ID(queue->thread_task_ids[i], min_running_task_id);
      }

      if (queue->thread_task_ids[i] == task_id) {
        // this task is still running
        task_is_running = true;
      }
    }

    if (task_is_running == true)
      continue;
    else {
      // task_id is NOT running so...

      // if task_id is less than the id of the task at the front of the queue,
      // then task_id must be finished (or it would be running)
      if (task_id < front_task_id) {
        return;
      }

      // if no tasks are running and tasks are waiting in the queue,
      // the worker threads need a moment so back off a bit and continue loop.
      if ((num_running_tasks == 0) && (tasks_are_waiting)) {
        vTaskDelay(10);
        continue;
      }

      // when the lowest running task id equals the queue's next_id,
      // it means the queue is empty so the task_id must be finished (or
      // something would be running)
      if (min_running_task_id == queue->next_id) {
        return;
      }

      // if task_id is greater than the lowest running task id,
      //  but is not running and nothing is waiting, it must have finished
      //  already
      if ((task_id > min_running_task_id) && (tasks_are_waiting == false)) {
        return;
      }
    }
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
