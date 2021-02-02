// Copyright 2021 XMOS LIMITED. This Software is subject to the terms of the 
// XMOS Public License: Version 1
#ifndef TEST_DISPATCH_QUEUE_H_
#define TEST_DISPATCH_QUEUE_H_

#include "dispatch.h"

typedef struct test_work_arg {
  int count;
} test_work_arg_t;

typedef struct test_parallel_work_arg {
  int begin;
  int end;
  int count;
} test_parallel_work_arg;

DISPATCH_TASK_FUNCTION
void do_limited_work(void *p);

DISPATCH_TASK_FUNCTION
void do_standard_work(void *p);

DISPATCH_TASK_FUNCTION
void do_extended_work(void *p);

DISPATCH_TASK_FUNCTION
void do_parallel_work(void *p);

#endif  // TEST_DISPATCH_QUEUE_H_
