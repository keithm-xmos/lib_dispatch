// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef LIB_DISPATCH_TASK_H_
#define LIB_DISPATCH_TASK_H_

#define DISPATCH_TASK_FUNCTION __attribute__((fptrgroup("dispatch_function")))

typedef void (*dispatch_function_t)(void *);

struct dispatch_queue_t;

typedef struct dispatch_task {
  char name[32];                   // to identify it when debugging
  dispatch_function_t fn;          // the function to perform
  void *arg;                       // argument to pass to the function
  struct dispatch_task *notify;    // the task to notify
  struct dispatch_queue_t *queue;  // parent queue
} dispatch_task_t;

// TODO: document me!
// Create a new task with function pointer, void pointer arument, and name (for
// debugging)
void dispatch_task_init(dispatch_task_t *ctx, dispatch_function_t fn, void *arg,
                        char *name);

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

#endif  // LIB_DISPATCH_TASK_H_