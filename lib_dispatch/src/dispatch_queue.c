// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "dispatch.h"

size_t dispatch_queue_function_add(dispatch_queue_t *ctx,
                                   dispatch_function_t fn, void *arg) {
  dispatch_task_t task;

  dispatch_task_init(&task, fn, arg);

  return dispatch_queue_task_add(ctx, &task);
}

void dispatch_queue_group_add(dispatch_queue_t *ctx, dispatch_group_t *group) {
  group->queue = ctx;
  size_t task_id;

  for (int i = 0; i < group->length; i++) {
    dispatch_task_t *task = &group->tasks[i];
    task_id = dispatch_queue_task_add(ctx, task);
  }
}
