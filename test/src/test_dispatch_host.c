// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include <pthread.h>
#include <time.h>

#include "lib_dispatch/api/dispatch.h"
#include "unity.h"
#include "unity_fixture.h"

pthread_mutex_t lock;

typedef struct test_work_arg {
  int count;
} test_work_arg_t;

DISPATCH_TASK_FUNCTION
void do_dispatch_host_work(void *p) {
  test_work_arg_t *arg = (test_work_arg_t *)p;

  nanosleep((const struct timespec[]){{0, 1000 * 1000000000L}}, NULL);

  pthread_mutex_lock(&lock);
  arg->count++;
  pthread_mutex_unlock(&lock);
}

TEST_GROUP(dispatch_queue_host);

TEST_SETUP(dispatch_queue_host) { pthread_mutex_init(&lock, NULL); }

TEST_TEAR_DOWN(dispatch_queue_host) { pthread_mutex_destroy(&lock); }

TEST(dispatch_queue_host, test_async_task) {
  dispatch_queue_t *queue;
  dispatch_task_t task;
  test_work_arg_t arg;
  int length = 3;
  int thread_count = 3;
  int task_count = 8;

  queue = dispatch_queue_create(length, thread_count, 0, "test_async_task");
  dispatch_task_init(&task, do_dispatch_host_work, &arg, "test_async_task");

  arg.count = 0;
  for (int i = 0; i < task_count; i++) {
    dispatch_queue_async_task(queue, &task);
  }
  dispatch_queue_wait(queue);

  TEST_ASSERT_EQUAL_INT(task_count, arg.count);

  dispatch_queue_destroy(queue);
}

TEST(dispatch_queue_host, test_for) {
  dispatch_queue_t *queue;
  dispatch_task_t task;
  test_work_arg_t arg;
  int length = 3;
  int thread_count = 3;
  int for_count = 5;

  queue = dispatch_queue_create(length, thread_count, 0, "test_for");
  dispatch_task_init(&task, do_dispatch_host_work, &arg, "test_for");

  arg.count = 0;
  dispatch_queue_for(queue, for_count, &task);
  dispatch_queue_wait(queue);

  TEST_ASSERT_EQUAL_INT(for_count, arg.count);

  dispatch_queue_destroy(queue);
}

TEST(dispatch_queue_host, test_async_group) {
  dispatch_queue_t *queue;
  dispatch_group_t *group;
  dispatch_task_t task;
  test_work_arg_t arg;
  int group_length = 3;
  int thread_count = 4;
  int group_count = 2;

  queue =
      dispatch_queue_create(thread_count, thread_count, 0, "test_async_group");
  group = dispatch_group_create(group_length, "test_async_group");
  dispatch_task_init(&task, do_dispatch_host_work, &arg, "test_async_group");

  arg.count = 0;

  // add tasks to group
  for (int i = 0; i < group_length; i++) {
    dispatch_group_add(group, &task);
  }

  for (int i = 0; i < group_count; i++) {
    dispatch_queue_async_group(queue, group);
    dispatch_group_wait(group);
    // nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);
  }

  TEST_ASSERT_EQUAL_INT((group_length * group_count), arg.count);

  dispatch_group_destroy(group);
  dispatch_queue_destroy(queue);
}

TEST(dispatch_queue_host, test_wait_task) {
  dispatch_queue_t *queue;
  dispatch_task_t task;
  test_work_arg_t arg;
  int thread_count = 4;

  queue =
      dispatch_queue_create(thread_count, thread_count, 0, "test_wait_task");
  dispatch_task_init(&task, do_dispatch_host_work, &arg, "test_wait_task");

  arg.count = 0;

  dispatch_queue_async_task(queue, &task);
  dispatch_task_wait(&task);
  // nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);

  TEST_ASSERT_EQUAL_INT(1, arg.count);

  dispatch_queue_destroy(queue);
}

TEST_GROUP_RUNNER(dispatch_queue_host) {
  RUN_TEST_CASE(dispatch_queue_host, test_wait_task);
  RUN_TEST_CASE(dispatch_queue_host, test_async_task);
  RUN_TEST_CASE(dispatch_queue_host, test_async_group);
  RUN_TEST_CASE(dispatch_queue_host, test_for);
}