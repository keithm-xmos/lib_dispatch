// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef LIB_DISPATCH_H_
#define LIB_DISPATCH_H_

#include <stddef.h>

#define DISPATCH_TASK_FUNCTION __attribute__((fptrgroup("dispatch_function")))

#include "dispatch_task.h"

typedef void dispatch_queue_t;

// **********
// group API
// **********
// typedef struct dispatch_group {
//   char name[64];  // to identify it when debugging
//   // void (*notify)(void *);  // the function to notify
//   dispatch_task_t *notify_task;    // the task to notify
//   dispatch_group_t *notify_group;  // the group to notify
// } dispatch_group_t;

// void dispatch_group_create(dispatch_group_t *group, char *name);

// void dispatch_group_task_notify(dispatch_group_t *current_group,
//                                 dispatch_task_t *notify_task);

// void dispatch_group_group_notify(dispatch_group_t *current_group,
//                                  dispatch_group_t *notify_group);

// void dispatch_group_wait(dispatch_group_t *group);

dispatch_queue_t* dispatch_queue_create(size_t length, size_t thread_count,
                                        size_t stack_size, char* name);

void dispatch_queue_init(dispatch_queue_t* ctx);

void dispatch_queue_destroy(dispatch_queue_t* ctx);

// Run task asyncronously
void dispatch_queue_async(dispatch_queue_t* ctx, dispatch_task_t* task);

// Run task syncronously
void dispatch_queue_sync(dispatch_queue_t* ctx, dispatch_task_t* task);

// Run task N times
void dispatch_queue_for(dispatch_queue_t* ctx, int N, dispatch_task_t* task);

// Wait for all tasks to be finish
void dispatch_queue_wait(dispatch_queue_t* ctx);

#endif  // LIB_DISPATCH_H_