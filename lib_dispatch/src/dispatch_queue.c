// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "dispatch_queue.h"

#include "dispatch_config.h"
#include "dispatch_group.h"
#include "dispatch_task.h"
#include "dispatch_types.h"

dispatch_task_t *dispatch_queue_function_add(dispatch_group_t *group,
                                             dispatch_function_t function,
                                             void *argument, bool waitable) {
  dispatch_task_t *task;

  task = dispatch_task_create(function, argument, waitable);
  dispatch_group_task_add(group, task);

  return task;
}

void dispatch_queue_group_add(dispatch_queue_t *ctx, dispatch_group_t *group) {
  for (int i = 0; i < group->count; i++) {
    dispatch_queue_task_add(ctx, group->tasks[i]);
  }
}

void dispatch_queue_group_wait(dispatch_queue_t *ctx, dispatch_group_t *group) {
  dispatch_assert(group->waitable);

  if (group->waitable) {
    for (int i = 0; i < group->count; i++) {
      dispatch_queue_task_wait(ctx, group->tasks[i]);
    }
  }
}