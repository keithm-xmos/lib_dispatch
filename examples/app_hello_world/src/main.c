// Copyright (c) 2020, XMOS Ltd, All rights reserved
#if FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#endif

#include "debug_print.h"
#include "lib_dispatch/api/dispatch.h"

typedef struct worker_arg {
  int index;
} worker_arg_t;

DISPATCH_TASK_FUNCTION
void do_work(void* p) {
  worker_arg_t* arg = (worker_arg_t*)p;
  debug_printf("Hello World from worker %d\n", arg->index);
}

static void hello_world(void* unused) {
  dispatch_queue_t* queue;
  dispatch_task_t tasks[3];
  worker_arg_t args[3];
  int queue_length = 3;
  int queue_thread_count = 3;

  // create the dispatch queue
  queue = dispatch_queue_create(queue_length, queue_thread_count, 1024,
                                "hello_world");

  // initialize 3 tasks
  for (int i = 0; i < 3; i++) {
    args[i].index = i;
    dispatch_task_init(&tasks[i], do_work, &args[i]);
  }

  // add 3 tasks to the dispatch queue
  for (int i = 0; i < 3; i++) {
    dispatch_queue_add_task(queue, &tasks[i]);
  }

  // wait for all tasks to finish executing
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