// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "lib_dispatch/api/dispatch.h"
#include "unity.h"
#include "unity_fixture.h"

typedef struct test_work_params {
  int zero;
  int one;
} test_work_params_t;

DISPATCH_TASK_FUNCTION
void do_dispatch_task_work(void *p) {
  test_work_params_t *params = (test_work_params_t *)p;
  params->zero = 0;
  params->one = 1;
}

DISPATCH_TASK_FUNCTION
void undo_dispatch_task_work(void *p) {
  test_work_params_t *params = (test_work_params_t *)p;
  params->zero = 1;
  params->one = 0;
}

TEST_GROUP(dispatch_task);

TEST_SETUP(dispatch_task) {}

TEST_TEAR_DOWN(dispatch_task) {}

TEST(dispatch_task, test_create) {
  dispatch_task_t task;
  void *params = NULL;
  char *name = "test_create_task";

  dispatch_task_create(&task, do_dispatch_task_work, params, name);

  TEST_ASSERT_NULL(task.notify);
  TEST_ASSERT_EQUAL(do_dispatch_task_work, task.work);
  TEST_ASSERT_EQUAL(params, task.params);
  // TEST_ASSERT_EQUAL_STRING(name, task.name);
}

TEST(dispatch_task, test_wait) {
  dispatch_task_t task;
  test_work_params_t params;

  dispatch_task_create(&task, do_dispatch_task_work, &params, "test_wait_task");
  dispatch_task_wait(&task);

  TEST_ASSERT_EQUAL_INT(0, params.zero);
  TEST_ASSERT_EQUAL_INT(1, params.one);
}

TEST(dispatch_task, test_notify) {
  dispatch_task_t do_task;
  dispatch_task_t undo_task;
  test_work_params_t params;

  // create two tasks
  dispatch_task_create(&do_task, do_dispatch_task_work, &params, NULL);
  dispatch_task_create(&undo_task, undo_dispatch_task_work, &params, NULL);

  // setup task to notify when first task is complete
  dispatch_task_notify(&do_task, &undo_task);

  // wait for first task to complete
  dispatch_task_wait(&do_task);

  // assert the notified task ran
  TEST_ASSERT_EQUAL_INT(1, params.zero);
  TEST_ASSERT_EQUAL_INT(0, params.one);
}

TEST_GROUP_RUNNER(dispatch_task) {
  RUN_TEST_CASE(dispatch_task, test_create);
  RUN_TEST_CASE(dispatch_task, test_wait);
  RUN_TEST_CASE(dispatch_task, test_notify);
}