// Copyright 2021 XMOS LIMITED. This Software is subject to the terms of the 
// XMOS Public License: Version 1
#include <stdio.h>

#include "dispatch.h"

DISPATCH_TASK_FUNCTION
void do_work(void* arg) { printf("Hello World from task %d\n", *(int*)arg); }

static void hello_world(void* unused) {
  dispatch_queue_t* queue;

  // create the dispatch queue
  queue = dispatch_queue_create(5, 5, 1024, 0);

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
  hello_world(NULL);
  return 0;
}