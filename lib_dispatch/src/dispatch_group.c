// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "lib_dispatch/api/dispatch_group.h"

#include <stdlib.h>
#include <string.h>
#include <xassert.h>

#include "debug_print.h"
#include "lib_dispatch/api/dispatch_queue.h"

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
  assert(ctx);

  ctx->count = 0;
  ctx->notify_task = NULL;
  ctx->notify_group = NULL;
  ctx->queue = NULL;
}

void dispatch_group_add(dispatch_group_t *ctx, dispatch_task_t *task) {
  assert(ctx);
  assert(ctx->count < ctx->length);

  memcpy(&ctx->tasks[ctx->count], task, sizeof(dispatch_task_t));
  ctx->count++;
}

void dispatch_group_notify_task(dispatch_group_t *ctx, dispatch_task_t *task) {
  assert(ctx);
  assert(task);

  ctx->notify_task = task;
}

void dispatch_group_notify_group(dispatch_group_t *ctx,
                                 dispatch_group_t *group) {
  assert(ctx);
  assert(group);

  ctx->notify_group = group;
}

void dispatch_group_perform(dispatch_group_t *ctx) {
  assert(ctx);

  debug_printf("dispatch_group_perform: %u\n", (long)ctx);

  // call group in current thread
  for (int i = 0; i < ctx->count; i++) {
    dispatch_task_t *task = &ctx->tasks[i];
    dispatch_task_perform(task);
  }

  if (ctx->notify_task) {
    dispatch_task_perform(ctx->notify_task);
  }

  if (ctx->notify_group) {
    dispatch_group_perform(ctx->notify_group);
  }
}

void dispatch_group_wait(dispatch_group_t *ctx) {
  assert(ctx);

  debug_printf("dispatch_group_wait: %u\n", (long)ctx);
  if (ctx->queue) {
    for (int i = 0; i < ctx->count; i++) {
      dispatch_task_t *task = &ctx->tasks[i];
      dispatch_queue_task_wait(ctx->queue, task->id);
    }
  }
}

void dispatch_group_destroy(dispatch_group_t *ctx) {
  assert(ctx);
  assert(ctx->tasks);

  free(ctx->tasks);
  free(ctx);
}
