// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef DISPATCH_QUEUE_FREERTOS_H_
#define DISPATCH_QUEUE_FREERTOS_H_
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

typedef struct dispatch_thread_data_struct dispatch_thread_data_t;
struct dispatch_thread_data_struct {
  volatile size_t *task_id;
  QueueHandle_t xQueue;
};

typedef struct dispatch_freertos_struct dispatch_freertos_queue_t;
struct dispatch_freertos_struct {
  char name[32];  // to identify it when debugging
  int length;
  int thread_count;
  int thread_stack_size;
  QueueHandle_t xQueue;
  size_t *thread_task_ids;
  dispatch_thread_data_t *thread_data;
  TaskHandle_t *threads;
};

#endif  // DISPATCH_QUEUE_FREERTOS_H_