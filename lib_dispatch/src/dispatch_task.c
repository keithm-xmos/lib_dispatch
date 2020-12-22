// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "debug_print.h"
#include "lib_dispatch/api/dispatch.h"

void dispatch_task_init(dispatch_task_t *ctx, dispatch_function_t fn, void *arg,
                        const char *name) {
  assert(ctx);
  assert(fn);
#ifndef NDEBUG
  if (name)
    strncpy(ctx->name, name, 32);
  else
    strncpy(ctx->name, "null", 32);
#endif
  ctx->fn = fn;
  ctx->arg = arg;
  ctx->notify = NULL;
  ctx->queue = NULL;
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

  debug_printf("dispatch_task_wait:  name=%s\n", ctx->name);

  if (ctx->queue) {
    for (;;) {
      if (dispatch_queue_task_status(ctx->queue, ctx) == DISPATCH_QUEUE_DONE)
        break;
    }
  } else {
    dispatch_task_perform(ctx);
  }
}

bool dispatch_task_equal(dispatch_task_t *lhs, dispatch_task_t *rhs) {
  assert(lhs);
  assert(rhs);

  return ((lhs->fn == rhs->fn) && (lhs->arg == rhs->arg) &&
          (lhs->notify == rhs->notify) && (lhs->queue == rhs->queue) &&
          (strncmp(lhs->name, rhs->name, 32) == 0));
}
