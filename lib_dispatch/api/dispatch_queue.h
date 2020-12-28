// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef LIB_DISPATCH_QUEUE_H_
#define LIB_DISPATCH_QUEUE_H_

#include <stddef.h>

#include "dispatch_group.h"
#include "dispatch_task.h"

typedef enum {
  DISPATCH_QUEUE_WAITING,
  DISPATCH_QUEUE_EXECUTING,
  DISPATCH_QUEUE_DONE,
} dispatch_queue_status_t;

typedef void dispatch_queue_t;

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// TODO: document me!
dispatch_queue_t* dispatch_queue_create(size_t length, size_t thread_count,
                                        size_t stack_size, const char* name);
// TODO: document me!
void dispatch_queue_init(dispatch_queue_t* ctx);

// TODO: document me!
void dispatch_queue_destroy(dispatch_queue_t* ctx);

// Run task asyncronously
void dispatch_queue_async_task(dispatch_queue_t* ctx, dispatch_task_t* task);

// Run task asyncronously
void dispatch_queue_async_group(dispatch_queue_t* ctx, dispatch_group_t* group);

// Run task N times
void dispatch_queue_for(dispatch_queue_t* ctx, int N, dispatch_task_t* task);

// Wait for tasks to be finish
void dispatch_queue_task_wait(dispatch_queue_t* ctx, int task_id);

// Wait for all tasks to be finish
void dispatch_queue_wait(dispatch_queue_t* ctx);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // LIB_DISPATCH_QUEUE_H_