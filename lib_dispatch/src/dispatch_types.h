// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef DISPATCH_TYPES_H_
#define DISPATCH_TYPES_H_

#include <stdbool.h>
#include <stddef.h>

#include "dispatch_task.h"

struct dispatch_task_struct {
  dispatch_function_t function;  // the function to perform
  void *argument;                // argument to pass to the function
  bool waitable;                 // task can be waited on
  void *private;                 // private data used by queue implementations
};

struct dispatch_group_struct {
  size_t length;            // maximum number of tasks in the group
  size_t count;             // number of tasks added to the group
  bool waitable;            // task can be waited on
  dispatch_task_t **tasks;  // array of task pointers
};

#endif  // DISPATCH_TYPES_H_