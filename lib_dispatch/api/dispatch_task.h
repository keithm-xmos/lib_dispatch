// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef LIB_DISPATCH_TASK_H_
#define LIB_DISPATCH_TASK_H_

typedef struct dispatch_task {
  void (*work)(void *);          // the function to perform
  void *params;                  // parameters to pass to the function
  struct dispatch_task *notify;  // the task to notify
  char *name;                    // to identify it when debugging
} dispatch_task_t;

// Create a new task with function pointer, void pointer arument, and name (for
// debugging)
void dispatch_task_create(dispatch_task_t *task, void (*work)(void *),
                          void *params, char *name);

// Schedules the execution of the notify task after the completion of the
// current task
void dispatch_task_notify(dispatch_task_t *current_task,
                          dispatch_task_t *notify_task);

// Causes the caller to wait synchronously until the task finishes executing
void dispatch_task_wait(dispatch_task_t *task);

#endif  // LIB_DISPATCH_TASK_H_