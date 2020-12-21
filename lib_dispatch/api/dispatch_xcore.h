// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef LIB_DISPATCH_XCORE_H_
#define LIB_DISPATCH_XCORE_H_

#include "lib_dispatch/api/dispatch.h"
#include "xcore/channel.h"

typedef enum {
  DISPATCH_THREAD_READY,
  DISPATCH_THREAD_BUSY
} dispatch_thread_status_t;

typedef struct dispatch_thread_data {
  volatile dispatch_thread_status_t *status;
  chanend_t cend;
} dispatch_thread_data_t;

typedef struct dispatch_xcore {
  char name[32];  // to identify it when debugging
  int length;
  int thread_count;
  int thread_stack_size;
  char *thread_stack;
  dispatch_thread_status_t *thread_status;
  dispatch_thread_data_t *thread_data;
  chanend_t *thread_chanends;
} dispatch_xcore_t;

#endif  // LIB_DISPATCH_XCORE_H_