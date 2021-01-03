// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "lib_dispatch/api/dispatch_queue.h"

#include "lib_dispatch/api/dispatch_group.h"
#include "lib_dispatch/api/dispatch_task.h"

size_t dispatch_queue_add_function(dispatch_queue_t *ctx,
                                   dispatch_function_t fn, void *arg) {
  dispatch_task_t task;

  dispatch_task_init(&task, fn, arg);

  return dispatch_queue_add_task(ctx, &task);
}

void dispatch_queue_add_group(dispatch_queue_t *ctx, dispatch_group_t *group) {
  group->queue = ctx;
  size_t task_id;

  for (int i = 0; i < group->length; i++) {
    dispatch_task_t *task = &group->tasks[i];
    task_id = dispatch_queue_add_task(ctx, task);
  }
}

size_t dispatch_queue_for_task(dispatch_queue_t *ctx, int N,
                               dispatch_task_t *task) {
  size_t task_id;

  for (int i = 0; i < N; i++) {
    task_id = dispatch_queue_add_task(ctx, task);
  }

  return task_id;
}

size_t dispatch_queue_for_function(dispatch_queue_t *ctx, int N,
                                   dispatch_function_t fn, void *arg) {
  dispatch_task_t task;

  dispatch_task_init(&task, fn, arg);

  return dispatch_queue_for_task(ctx, N, &task);
}
