// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef LIB_DISPATCH_TASK_H_
#define LIB_DISPATCH_TASK_H_

#include <stddef.h>

#ifdef XCORE
#define DISPATCH_TASK_FUNCTION __attribute__((fptrgroup("dispatch_function")))
#else
#define DISPATCH_TASK_FUNCTION
#endif

typedef void (*dispatch_function_t)(void *);

struct dispatch_queue_struct;

typedef struct dispatch_task_struct dispatch_task_t;
struct dispatch_task_struct {
  size_t id;                            // unique identifier
  dispatch_function_t fn;               // the function to perform
  void *arg;                            // argument to pass to the function
  struct dispatch_task_struct *notify;  // the task to notify
  struct dispatch_queue_struct *queue;  // parent queue
};

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// TODO: document me!
// Create a new task with function pointer, void pointer arument, and name (for
// debugging)
size_t dispatch_task_init(dispatch_task_t *ctx, dispatch_function_t fn,
                          void *arg);

// TODO: document me!
// Schedules the execution of the notify task after the completion of the
// current task
void dispatch_task_notify(dispatch_task_t *ctx, dispatch_task_t *task);

// TODO: document me!
// Run the task in the caller's thread
void dispatch_task_perform(dispatch_task_t *ctx);

// TODO: document me!
// Causes the caller to wait synchronously until the task finishes executing
void dispatch_task_wait(dispatch_task_t *ctx);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // LIB_DISPATCH_TASK_H_