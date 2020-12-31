// Copyright (c) 2020, XMOS Ltd, All rights reserved

#include "test_dispatch_queue.h"

#include "lib_dispatch/api/dispatch.h"
#include "unity.h"
#include "unity_fixture.h"

#if XCORE
#include "test_dispatch_queue_xcore.h"
#elif FREERTOS
#include "test_dispatch_queue_freertos.h"
#elif HOST
#include "test_dispatch_queue_host.h"
#endif

static thread_mutex_t lock;

DISPATCH_TASK_FUNCTION
void do_dispatch_queue_work(void *p) {
  test_work_arg_t *arg = (test_work_arg_t *)p;

  look_busy(500);

  mutex_lock(&lock);
  arg->count++;
  mutex_unlock(&lock);
}

TEST_GROUP(dispatch_queue);

TEST_SETUP(dispatch_queue) { mutex_init(&lock); }

TEST_TEAR_DOWN(dispatch_queue) { mutex_destroy(&lock); }

TEST(dispatch_queue, test_wait_task) {
  dispatch_queue_t *queue;
  dispatch_task_t task;
  test_work_arg_t arg;
  int queue_length = QUEUE_LENGTH;
  int queue_thread_count = QUEUE_THREAD_COUNT;

  queue = dispatch_queue_create(queue_length, queue_thread_count,
                                QUEUE_STACK_SIZE, "test_wait_task");
  dispatch_task_init(&task, do_dispatch_queue_work, &arg);

  arg.count = 0;

  dispatch_queue_async_task(queue, &task);
  dispatch_task_wait(&task);

  TEST_ASSERT_EQUAL_INT(1, arg.count);

  dispatch_queue_destroy(queue);
}

TEST(dispatch_queue, test_wait_group) {
  dispatch_queue_t *queue;
  dispatch_group_t *group;
  dispatch_task_t task;
  test_work_arg_t arg;
  int queue_length = QUEUE_LENGTH;
  int queue_thread_count = QUEUE_THREAD_COUNT;
  int group_length = 3;

  queue = dispatch_queue_create(queue_length, queue_thread_count,
                                QUEUE_STACK_SIZE, "test_async_group");
  group = dispatch_group_create(group_length);
  dispatch_task_init(&task, do_dispatch_queue_work, &arg);

  arg.count = 0;

  // add tasks to group
  for (int i = 0; i < group_length; i++) {
    dispatch_group_add(group, &task);
  }

  dispatch_queue_async_group(queue, group);
  dispatch_group_wait(group);

  TEST_ASSERT_EQUAL_INT(group_length, arg.count);

  dispatch_group_destroy(group);
  dispatch_queue_destroy(queue);
}

TEST(dispatch_queue, test_async_task) {
  dispatch_queue_t *queue;
  dispatch_task_t task;
  test_work_arg_t arg;
  int queue_length = QUEUE_LENGTH;
  int queue_thread_count = QUEUE_THREAD_COUNT;
  int task_count = 8;

  queue = dispatch_queue_create(queue_length, queue_thread_count,
                                QUEUE_STACK_SIZE, "test_async_task");
  dispatch_task_init(&task, do_dispatch_queue_work, &arg);

  arg.count = 0;
  for (int i = 0; i < task_count; i++) {
    dispatch_queue_async_task(queue, &task);
  }
  dispatch_queue_wait(queue);

  TEST_ASSERT_EQUAL_INT(task_count, arg.count);

  dispatch_queue_destroy(queue);
}

TEST(dispatch_queue, test_for) {
  dispatch_queue_t *queue;
  dispatch_task_t task;
  test_work_arg_t arg;
  int queue_length = QUEUE_LENGTH;
  int queue_thread_count = QUEUE_THREAD_COUNT;
  int for_count = 5;

  queue = dispatch_queue_create(queue_length, queue_thread_count,
                                QUEUE_STACK_SIZE, "test_for");
  dispatch_task_init(&task, do_dispatch_queue_work, &arg);

  arg.count = 0;
  dispatch_queue_for(queue, for_count, &task);
  dispatch_queue_wait(queue);

  TEST_ASSERT_EQUAL_INT(for_count, arg.count);

  dispatch_queue_destroy(queue);
}

TEST(dispatch_queue, test_async_group) {
  dispatch_queue_t *queue;
  dispatch_group_t *group;
  dispatch_task_t task;
  test_work_arg_t arg;
  int queue_length = QUEUE_LENGTH;
  int queue_thread_count = QUEUE_THREAD_COUNT;
  int group_length = 3;
  int group_count = 2;

  queue = dispatch_queue_create(queue_length, queue_thread_count,
                                QUEUE_STACK_SIZE, "test_async_group");
  group = dispatch_group_create(group_length);
  dispatch_task_init(&task, do_dispatch_queue_work, &arg);

  arg.count = 0;

  // add tasks to group
  for (int i = 0; i < group_length; i++) {
    dispatch_group_add(group, &task);
  }

  for (int i = 0; i < group_count; i++) {
    dispatch_queue_async_group(queue, group);
    dispatch_group_wait(group);
  }

  TEST_ASSERT_EQUAL_INT((group_length * group_count), arg.count);

  dispatch_group_destroy(group);
  dispatch_queue_destroy(queue);
}

TEST_GROUP_RUNNER(dispatch_queue) {
  RUN_TEST_CASE(dispatch_queue, test_wait_task);
  RUN_TEST_CASE(dispatch_queue, test_wait_group);
  RUN_TEST_CASE(dispatch_queue, test_async_task);
  RUN_TEST_CASE(dispatch_queue, test_async_group);
  RUN_TEST_CASE(dispatch_queue, test_for);
}