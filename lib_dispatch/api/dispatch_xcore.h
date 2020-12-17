// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef LIB_DISPATCH_XCORE_H_
#define LIB_DISPATCH_XCORE_H_

#include "lib_dispatch/api/dispatch.h"
#include "xcore/channel.h"

typedef enum {
  DISPATCH_WORKER_READY,
  DISPATCH_WORKER_BUSY
} dispatch_worker_flag_t;

typedef struct dispatch_xcore {
  int length;
  int thread_count;
  int stack_words;
  char *stack;
  volatile dispatch_worker_flag_t
      *flags;  // must be volatile as it is shared by >1 threads
  channel_t *channels;
  char *name;  // to identify it when debugging
} dispatch_xcore_t;

#endif  // LIB_DISPATCH_XCORE_H_