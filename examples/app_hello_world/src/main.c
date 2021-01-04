// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include <stdio.h>

#if FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#endif

#include "lib_dispatch/api/dispatch.h"

#define NUM_FUNCTIONS (3)

typedef struct worker_arg {
  int index;
} worker_arg_t;

DISPATCH_TASK_FUNCTION
void do_work(void* p) {
  worker_arg_t* arg = (worker_arg_t*)p;
  printf("Hello World from thread %d\n", arg->index);
}

static void hello_world(void* unused) {
  dispatch_queue_t* queue;
  worker_arg_t args[NUM_FUNCTIONS];
  int queue_length = NUM_FUNCTIONS;
  int queue_thread_count = 2;

  // create the dispatch queue
  queue = dispatch_queue_create(queue_length, queue_thread_count, 1024,
                                "hello_world");

  // add NUM_FUNCTIONS functions
  for (int i = 0; i < NUM_FUNCTIONS; i++) {
    args[i].index = i;
    dispatch_queue_function_add(queue, do_work, &args[i]);
  }

  // wait for all functions to finish executing
  dispatch_queue_wait(queue);

  // destroy the dispatch queue
  dispatch_queue_destroy(queue);
}

int main(int argc, const char* argv[]) {
#if FREERTOS
  xTaskCreate(hello_world, "hello_world", 1024 * 16, NULL, configMAX_PRIORITIES,
              NULL);
  vTaskStartScheduler();
#endif
  // for FreeRTOS build we never reach here because vTaskStartScheduler never
  // returns
  hello_world(NULL);
  return 0;
}