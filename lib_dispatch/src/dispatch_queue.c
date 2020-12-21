// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "lib_dispatch/api/dispatch_queue.h"

#include "lib_dispatch/api/dispatch_group.h"
#include "lib_dispatch/api/dispatch_task.h"

void dispatch_queue_async_group(dispatch_queue_t *ctx,
                                dispatch_group_t *group) {
  group->queue = ctx;

  for (int i = 0; i < group->length; i++) {
    dispatch_task_t *task = &group->tasks[i];
    dispatch_queue_async_task(ctx, task);
  }
}

void dispatch_queue_for(dispatch_queue_t *ctx, int N, dispatch_task_t *task) {
  for (int i = 0; i < N; i++) {
    dispatch_queue_async_task(ctx, task);
  }
}
