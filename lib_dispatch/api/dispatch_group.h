// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef LIB_DISPATCH_GROUP_H_
#define LIB_DISPATCH_GROUP_H_

#include <stddef.h>

#include "lib_dispatch/api/dispatch_task.h"

struct dispatch_queue_struct;

typedef struct dispatch_group_struct dispatch_group_t;
struct dispatch_group_struct {
  size_t id;
  size_t max_length;
  size_t length;
  dispatch_task_t *tasks;
  dispatch_task_t *notify_task;                // the task to notify
  struct dispatch_group_struct *notify_group;  // the group to notify
  struct dispatch_queue_struct *queue;         // parent queue
};

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// TODO: document me!
dispatch_group_t *dispatch_group_create(size_t length);

// TODO: document me!
size_t dispatch_group_init(dispatch_group_t *ctx);

// TODO: document me!
void dispatch_group_add(dispatch_group_t *ctx, dispatch_task_t *task);

// TODO: document me!
void dispatch_group_notify_group(dispatch_group_t *ctx,
                                 dispatch_group_t *group);

// TODO: document me!
void dispatch_group_notify_task(dispatch_group_t *ctx, dispatch_task_t *task);

// TODO: document me!
void dispatch_group_perform(dispatch_group_t *ctx);

// TODO: document me!
void dispatch_group_wait(dispatch_group_t *ctx);

// TODO: document me!
void dispatch_group_destroy(dispatch_group_t *ctx);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // LIB_DISPATCH_GROUP_H_