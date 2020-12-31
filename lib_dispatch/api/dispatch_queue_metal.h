// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef DISPATCH_QUEUE_XCORE_H_
#define DISPATCH_QUEUE_XCORE_H_

#include "xcore/channel.h"

typedef struct dispatch_thread_data_struct dispatch_thread_data_t;
struct dispatch_thread_data_struct {
  volatile size_t *task_id;
  chanend_t cend;
};

typedef struct dispatch_xcore_struct dispatch_xcore_queue_t;
struct dispatch_xcore_struct {
  char name[32];  // to identify it when debugging
  size_t length;
  size_t thread_count;
  size_t thread_stack_size;
  size_t next_id;
  char *thread_stack;
  size_t *thread_task_ids;
  dispatch_thread_data_t *thread_data;
  chanend_t *thread_chanends;
};

#endif  // DISPATCH_QUEUE_XCORE_H_