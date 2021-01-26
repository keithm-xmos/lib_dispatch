// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include <stdio.h>

#if FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#define THREAD_PRIORITY (configMAX_PRIORITIES - 1)
#else
#define THREAD_PRIORITY (0)
#endif

#include "dispatch.h"

DISPATCH_TASK_FUNCTION
void do_work(void* arg) { printf("Hello World from task %d\n", *(int*)arg); }

static void hello_world(void* unused) {
  dispatch_queue_t* queue;

  // create the dispatch queue
  queue = dispatch_queue_create(5, 5, 1024, THREAD_PRIORITY);

  // add functions
  const int task_data[5] = {0, 1, 2, 3, 4};
  for (int i = 0; i < 5; i++) {
    dispatch_queue_function_add(queue, do_work, (void*)&task_data[i], false);
  }

  // wait for all functions to finish executing
  dispatch_queue_wait(queue);

  // delete the dispatch queue
  dispatch_queue_delete(queue);
}

int main(int argc, const char* argv[]) {
#if FREERTOS
  xTaskCreate(hello_world, "hello_world", 1024 * 16, NULL,
              configMAX_PRIORITIES - 1, NULL);
  vTaskStartScheduler();
#endif
  // for FreeRTOS build we never reach here because vTaskStartScheduler never
  // returns
  hello_world(NULL);
  return 0;
}