// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef LIB_DISPATCH_TASK_H_
#define LIB_DISPATCH_TASK_H_

typedef enum {
  DISPATCH_TASK_WAITING,
  DISPATCH_TASK_EXECUTING,
  DISPATCH_TASK_DONE,
} dispatch_task_status_t;

typedef void (*dispatch_function_t)(void *);

struct dispatch_queue_t;

typedef struct dispatch_task {
  char name[32];                   // to identify it when debugging
  dispatch_function_t fn;          // the function to perform
  void *arg;                       // argument to pass to the function
  struct dispatch_task *notify;    // the task to notify
  struct dispatch_queue_t *queue;  // parent queue
} dispatch_task_t;

// Create a new task with function pointer, void pointer arument, and name (for
// debugging)
void dispatch_task_init(dispatch_task_t *ctx, dispatch_function_t fn, void *arg,
                        char *name);

// Schedules the execution of the notify task after the completion of the
// current task
void dispatch_task_notify(dispatch_task_t *ctx, dispatch_task_t *notify_task);

// Run the task in the caller's thread
void dispatch_task_perform(dispatch_task_t *ctx);

// Causes the caller to wait synchronously until the task finishes executing
void dispatch_task_wait(dispatch_task_t *ctx);

#endif  // LIB_DISPATCH_TASK_H_