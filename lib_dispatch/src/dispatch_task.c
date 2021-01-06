// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "dispatch_task.h"

#include "dispatch_config.h"
#include "dispatch_types.h"

dispatch_task_t *dispatch_task_create(dispatch_function_t function,
                                      void *argument, bool waitable) {
  dispatch_assert(function);

  dispatch_task_t *task;
  task = dispatch_malloc(sizeof(dispatch_task_t));

  dispatch_task_init(task, function, argument, waitable);

  dispatch_printf("dispatch_task_create:  task=%u\n", (long)task);

  return task;
}

void dispatch_task_init(dispatch_task_t *task, dispatch_function_t function,
                        void *argument, bool waitable) {
  dispatch_assert(task);
  dispatch_assert(function);

  task->function = function;
  task->argument = argument;
  task->waitable = waitable;
  task->private = NULL;
}

void dispatch_task_perform(dispatch_task_t *task) {
  dispatch_assert(task);

  dispatch_printf("dispatch_task_perform:  task=%u\n", (long)task);

  // call function in current thread
  task->function(task->argument);
}

void dispatch_task_destroy(dispatch_task_t *task) {
  dispatch_assert(task);

  dispatch_printf("dispatch_task_destroy:  task=%u\n", (long)task);

  dispatch_free(task);
}
