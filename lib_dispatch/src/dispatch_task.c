// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include <stdlib.h>
#include <string.h>
#include <xassert.h>

#include "debug_print.h"
#include "lib_dispatch/api/dispatch.h"

size_t dispatch_task_init(dispatch_task_t *ctx, dispatch_function_t fn,
                          void *arg) {
  assert(ctx);
  assert(fn);

  static int next_id = DISPATCH_TASK_NONE + 1;

  ctx->id = next_id++;
  ctx->fn = fn;
  ctx->arg = arg;
  ctx->notify = NULL;
  ctx->queue = NULL;

  return ctx->id;
}

void dispatch_task_notify(dispatch_task_t *ctx, dispatch_task_t *task) {
  assert(ctx);
  assert(task);
  ctx->notify = task;
}

void dispatch_task_perform(dispatch_task_t *ctx) {
  assert(ctx);

  // call function in current thread
  ctx->fn(ctx->arg);

  if (ctx->notify) {
    // call notify function in current thread
    ctx->notify->fn(ctx->notify->arg);
  }
}

void dispatch_task_wait(dispatch_task_t *ctx) {
  assert(ctx);

  debug_printf("dispatch_task_wait:  id=%d\n", ctx->id);
  if (ctx->queue) {
    dispatch_queue_task_wait(ctx->queue, ctx->id);
  }
}
