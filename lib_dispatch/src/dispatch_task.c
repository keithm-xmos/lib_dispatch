// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include <stdlib.h>
#include <string.h>
#include <xassert.h>

#include "debug_print.h"
#include "lib_dispatch/api/dispatch.h"

void dispatch_task_init(dispatch_task_t *ctx, dispatch_function_t fn,
                        void *arg) {
  assert(ctx);
  assert(fn);

  ctx->fn = fn;
  ctx->arg = arg;
  ctx->id = DISPATCH_TASK_NONE;
  ctx->queue = NULL;
}

void dispatch_task_perform(dispatch_task_t *ctx) {
  assert(ctx);

  debug_printf("dispatch_task_perform:  id=%d\n", ctx->id);

  // call function in current thread
  ctx->fn(ctx->arg);
}
