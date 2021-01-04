// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include <string.h>

#include "lib_dispatch/api/dispatch.h"
#include "unity.h"
#include "unity_fixture.h"

#define TEST_STATIC_LENGTH (3)

typedef struct test_work_arg {
  int count;
} test_work_arg_t;

DISPATCH_TASK_FUNCTION
void do_dispatch_group_work(void *p) {
  test_work_arg_t *arg = (test_work_arg_t *)p;
  arg->count += 1;
}

DISPATCH_TASK_FUNCTION
void undo_dispatch_group_work(void *p) {
  test_work_arg_t *arg = (test_work_arg_t *)p;
  arg->count -= 1;
}

TEST_GROUP(dispatch_group);

TEST_SETUP(dispatch_group) {}

TEST_TEAR_DOWN(dispatch_group) {}

TEST(dispatch_group, test_static) {
  dispatch_group_t group_s;
  dispatch_task_t tasks[TEST_STATIC_LENGTH];

  test_work_arg_t arg;

  arg.count = 0;

  group_s.length = TEST_STATIC_LENGTH;
  group_s.tasks = &tasks[0];

  dispatch_group_t *group = &group_s;
  dispatch_group_init(group);

  for (int i = 0; i < TEST_STATIC_LENGTH; i++) {
    dispatch_group_add(group, do_dispatch_group_work, &arg);
  }
  dispatch_group_perform(group);

  TEST_ASSERT_EQUAL_INT(TEST_STATIC_LENGTH, arg.count);
}

TEST(dispatch_group, test_perform) {
  dispatch_group_t *group;
  test_work_arg_t arg;
  int length = 3;

  arg.count = 0;

  group = dispatch_group_create(length);
  for (int i = 0; i < length; i++) {
    dispatch_group_add(group, do_dispatch_group_work, &arg);
  }
  dispatch_group_perform(group);

  TEST_ASSERT_EQUAL_INT(length, arg.count);

  dispatch_group_destroy(group);
}

TEST_GROUP_RUNNER(dispatch_group) {
  RUN_TEST_CASE(dispatch_group, test_static);
  RUN_TEST_CASE(dispatch_group, test_perform);
}