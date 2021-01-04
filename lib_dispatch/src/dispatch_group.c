// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "dispatch_group.h"

#include <stdlib.h>
#include <string.h>

#include "dispatch.h"

dispatch_group_t *dispatch_group_create(size_t length) {
  dispatch_group_t *group;

  debug_printf("dispatch_group_create: length=%d\n", length);

  group = (dispatch_group_t *)malloc(sizeof(dispatch_group_t));

  group->length = length;
  group->tasks = (dispatch_task_t *)malloc(sizeof(dispatch_task_t) * length);

  // initialize the queue
  dispatch_group_init(group);

  return group;
}

void dispatch_group_init(dispatch_group_t *ctx) {
  xassert(ctx);

  ctx->count = 0;
  ctx->queue = NULL;
}

void dispatch_group_task_add(dispatch_group_t *ctx, dispatch_task_t *task) {
  xassert(ctx);
  xassert(ctx->count < ctx->length);

  memcpy(&ctx->tasks[ctx->count], task, sizeof(dispatch_task_t));
  ctx->count++;
}

void dispatch_group_function_add(dispatch_group_t *ctx, dispatch_function_t fn,
                                 void *arg) {
  xassert(ctx);
  xassert(ctx->count < ctx->length);

  dispatch_task_t task;
  dispatch_task_init(&task, fn, arg);
  dispatch_group_task_add(ctx, &task);
}

void dispatch_group_perform(dispatch_group_t *ctx) {
  xassert(ctx);

  debug_printf("dispatch_group_perform: %u\n", (long)ctx);

  // call group in current thread
  for (int i = 0; i < ctx->count; i++) {
    dispatch_task_t *task = &ctx->tasks[i];
    dispatch_task_perform(task);
  }
}

void dispatch_group_wait(dispatch_group_t *ctx) {
  xassert(ctx);

  debug_printf("dispatch_group_wait: %u\n", (long)ctx);
  if (ctx->queue) {
    for (int i = 0; i < ctx->count; i++) {
      dispatch_task_t *task = &ctx->tasks[i];
      dispatch_queue_task_wait(ctx->queue, task->id);
    }
  }
}

void dispatch_group_destroy(dispatch_group_t *ctx) {
  xassert(ctx);
  xassert(ctx->tasks);

  free(ctx->tasks);
  free(ctx);
}
