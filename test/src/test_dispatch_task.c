// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "lib_dispatch/api/dispatch.h"
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
  char *name = "test_create_task";

  dispatch_task_init(&task, do_dispatch_task_work, arg, name);

  TEST_ASSERT_NULL(task.notify);
  TEST_ASSERT_EQUAL(do_dispatch_task_work, task.fn);
  TEST_ASSERT_EQUAL(arg, task.arg);
}

TEST(dispatch_task, test_wait) {
  dispatch_task_t task;
  test_work_arg_t arg;

  dispatch_task_init(&task, do_dispatch_task_work, &arg, "test_wait_task");
  dispatch_task_wait(&task);

  TEST_ASSERT_EQUAL_INT(0, arg.zero);
  TEST_ASSERT_EQUAL_INT(1, arg.one);
}

TEST(dispatch_task, test_perform) {
  dispatch_task_t task;
  test_work_arg_t arg;

  dispatch_task_init(&task, do_dispatch_task_work, &arg, "test_perform_task");
  dispatch_task_perform(&task);

  TEST_ASSERT_EQUAL_INT(0, arg.zero);
  TEST_ASSERT_EQUAL_INT(1, arg.one);
}

TEST(dispatch_task, test_notify) {
  dispatch_task_t do_task;
  dispatch_task_t undo_task;
  test_work_arg_t arg;

  // create two tasks
  dispatch_task_init(&do_task, do_dispatch_task_work, &arg, NULL);
  dispatch_task_init(&undo_task, undo_dispatch_task_work, &arg, NULL);

  // setup task to notify when first task is complete
  dispatch_task_notify(&do_task, &undo_task);

  // wait for first task to complete
  dispatch_task_wait(&do_task);

  // assert the notified task ran
  TEST_ASSERT_EQUAL_INT(1, arg.zero);
  TEST_ASSERT_EQUAL_INT(0, arg.one);
}

TEST_GROUP_RUNNER(dispatch_task) {
  RUN_TEST_CASE(dispatch_task, test_create);
  RUN_TEST_CASE(dispatch_task, test_perform);
  RUN_TEST_CASE(dispatch_task, test_wait);
  RUN_TEST_CASE(dispatch_task, test_notify);
}