// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include <assert.h>
#include <stdlib.h>

#include "lib_dispatch/api/dispatch.h"

void dispatch_task_create(dispatch_task_t *task, void (*work)(void *),
                          void *params, char *name) {
  assert(task);
  task->name = name;
  task->work = work;
  task->params = params;
  task->notify = NULL;
}

void dispatch_task_notify(dispatch_task_t *current_task,
                          dispatch_task_t *notify_task) {
  assert(current_task);
  assert(notify_task);
  current_task->notify = notify_task;
}

void dispatch_task_wait(dispatch_task_t *task) {
  assert(task);

  // call work function in current thread
  task->work(task->params);

  if (task->notify) {
    // call notify function in current thread
    task->notify->work(task->notify->params);
  }
}
