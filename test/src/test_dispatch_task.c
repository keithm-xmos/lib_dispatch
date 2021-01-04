// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "dispatch.h"
#include "unity.h"
#include "unity_fixture.h"

typedef struct test_work_arg {
  int zero;
  int one;
} test_work_arg_t;

DISPATCH_TASK_FUNCTION
void do_dispatch_task_work(void *p) {
  test_work_arg_t *arg = (test_work_arg_t *)p;
  arg->zero = 0;
  arg->one = 1;
}

DISPATCH_TASK_FUNCTION
void undo_dispatch_task_work(void *p) {
  test_work_arg_t *arg = (test_work_arg_t *)p;
  arg->zero = 1;
  arg->one = 0;
}

TEST_GROUP(dispatch_task);

TEST_SETUP(dispatch_task) {}

TEST_TEAR_DOWN(dispatch_task) {}

TEST(dispatch_task, test_create) {
  dispatch_task_t task;
  void *arg = NULL;

  dispatch_task_init(&task, do_dispatch_task_work, arg);

  TEST_ASSERT_EQUAL(do_dispatch_task_work, task.fn);
  TEST_ASSERT_EQUAL(arg, task.arg);
  TEST_ASSERT_EQUAL_INT(DISPATCH_TASK_NONE, task.id);
  TEST_ASSERT_NULL(task.queue);
}

TEST(dispatch_task, test_perform) {
  dispatch_task_t task;
  test_work_arg_t arg;

  dispatch_task_init(&task, do_dispatch_task_work, &arg);
  dispatch_task_perform(&task);

  TEST_ASSERT_EQUAL_INT(0, arg.zero);
  TEST_ASSERT_EQUAL_INT(1, arg.one);
}

TEST_GROUP_RUNNER(dispatch_task) {
  RUN_TEST_CASE(dispatch_task, test_create);
  RUN_TEST_CASE(dispatch_task, test_perform);
}