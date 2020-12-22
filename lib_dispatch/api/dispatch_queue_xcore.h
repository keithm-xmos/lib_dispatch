// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef LIB_DISPATCH_XCORE_H_
#define LIB_DISPATCH_XCORE_H_

#include "xcore/channel.h"

typedef int dispatch_thread_task_t;

typedef struct dispatch_thread_data_struct dispatch_thread_data_t;
struct dispatch_thread_data_struct {
  volatile dispatch_thread_task_t *task;
  chanend_t cend;
};

typedef struct dispatch_xcore_struct dispatch_xcore_queue_t;
struct dispatch_xcore_struct {
  char name[32];  // to identify it when debugging
  int length;
  int thread_count;
  int thread_stack_size;
  char *thread_stack;
  dispatch_thread_task_t *thread_tasks;
  dispatch_thread_data_t *thread_data;
  chanend_t *thread_chanends;
};

#endif  // LIB_DISPATCH_XCORE_H_