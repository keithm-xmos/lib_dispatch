// Copyright (c) 2020, XMOS Ltd, All rights reserved

#include "test_dispatch_queue.h"

#include "dispatch.h"
#include "unity.h"
#include "unity_fixture.h"

#if BARE_METAL
#include "test_dispatch_queue_metal.h"
#elif FREERTOS
#include "test_dispatch_queue_freertos.h"
#elif HOST
#include "test_dispatch_queue_host.h"
#endif

static thread_mutex_t lock;

DISPATCH_TASK_FUNCTION
void do_limited_work(void *p) {
  test_work_arg_t *arg = (test_work_arg_t *)p;

  look_busy(100);

  mutex_lock(&lock);
  arg->count++;
  mutex_unlock(&lock);
}

DISPATCH_TASK_FUNCTION
void do_standard_work(void *p) {
  test_work_arg_t *arg = (test_work_arg_t *)p;

  look_busy(500);

  mutex_lock(&lock);
  arg->count++;
  mutex_unlock(&lock);
}

DISPATCH_TASK_FUNCTION
void do_extended_work(void *p) {
  test_work_arg_t *arg = (test_work_arg_t *)p;

  look_busy(1000);

  mutex_lock(&lock);
  arg->count++;
  mutex_unlock(&lock);
}

DISPATCH_TASK_FUNCTION
void do_parallel_work(void *p) {
  // NOTE: the "volatile" is needed here or the compiler may optimize this away
  test_parallel_work_arg volatile *arg = (test_parallel_work_arg volatile *)p;

  for (int i = arg->begin; i < arg->end; i++) arg->count++;
}

TEST_GROUP(dispatch_queue);

TEST_SETUP(dispatch_queue) { mutex_init(&lock); }

TEST_TEAR_DOWN(dispatch_queue) { mutex_destroy(&lock); }

TEST(dispatch_queue, test_wait_queue) {
  dispatch_queue_t *queue;
  dispatch_task_t *task1;
  dispatch_task_t *task2;
  test_work_arg_t arg;
  int queue_length = QUEUE_LENGTH;
  int queue_thread_count = QUEUE_THREAD_COUNT;

  queue = dispatch_queue_create(queue_length, queue_thread_count,
                                QUEUE_THREAD_STACK_SIZE, QUEUE_THREAD_PRIORITY);

  arg.count = 0;

  task1 = dispatch_task_create(do_standard_work, &arg, false);
  task2 = dispatch_task_create(do_standard_work, &arg, false);
  dispatch_queue_task_add(queue, task1);
  dispatch_queue_task_add(queue, task2);
  dispatch_queue_wait(queue);

  TEST_ASSERT_EQUAL_INT(2, arg.count);

  dispatch_queue_destroy(queue);
}

TEST(dispatch_queue, test_wait_task) {
  dispatch_queue_t *queue;
  dispatch_task_t *task;
  test_work_arg_t arg;
  int queue_length = QUEUE_LENGTH;
  int queue_thread_count = QUEUE_THREAD_COUNT;

  queue = dispatch_queue_create(queue_length, queue_thread_count,
                                QUEUE_THREAD_STACK_SIZE, QUEUE_THREAD_PRIORITY);

  arg.count = 0;

  task = dispatch_task_create(do_standard_work, &arg, true);
  dispatch_queue_task_add(queue, task);
  dispatch_queue_task_wait(queue, task);

  TEST_ASSERT_EQUAL_INT(1, arg.count);

  dispatch_queue_destroy(queue);
}

TEST(dispatch_queue, test_wait_group) {
  dispatch_queue_t *queue;
  dispatch_group_t *group;
  test_work_arg_t arg;
  int queue_length = QUEUE_LENGTH;
  int queue_thread_count = QUEUE_THREAD_COUNT;
  int group_length = 3;

  queue = dispatch_queue_create(queue_length, queue_thread_count,
                                QUEUE_THREAD_STACK_SIZE, QUEUE_THREAD_PRIORITY);
  group = dispatch_group_create(group_length, true);

  arg.count = 0;

  // add tasks to group
  for (int i = 0; i < group_length; i++) {
    dispatch_group_function_add(group, do_standard_work, &arg);
  }

  dispatch_queue_group_add(queue, group);
  dispatch_queue_group_wait(queue, group);

  TEST_ASSERT_EQUAL_INT(group_length, arg.count);

  dispatch_group_destroy(group);
  dispatch_queue_destroy(queue);
}

TEST(dispatch_queue, test_mixed_durations1) {
  dispatch_queue_t *queue;
  dispatch_task_t *standard_task;
  dispatch_task_t **extended_tasks;
  test_work_arg_t arg;
  int queue_length = QUEUE_LENGTH;
  int queue_thread_count = QUEUE_THREAD_COUNT;
  int max_extended_task_count = QUEUE_THREAD_COUNT + 1;
  int extended_task_count;

  for (int i = 0; i < max_extended_task_count; i++) {
    queue =
        dispatch_queue_create(queue_length, queue_thread_count,
                              QUEUE_THREAD_STACK_SIZE, QUEUE_THREAD_PRIORITY);

    extended_task_count = i + 1;

    extended_tasks = malloc(extended_task_count * sizeof(dispatch_task_t *));

    // create all the tasks
    for (int j = 0; j < extended_task_count; j++) {
      extended_tasks[j] = dispatch_task_create(do_extended_work, &arg, true);
    }
    standard_task = dispatch_task_create(do_standard_work, &arg, true);

    arg.count = 0;

    // add extended tasks
    for (int j = 0; j < extended_task_count; j++) {
      dispatch_queue_task_add(queue, extended_tasks[j]);
    }

    // add limited task
    dispatch_queue_task_add(queue, standard_task);

    // now wait for the standard task
    dispatch_queue_task_wait(queue, standard_task);
    if (extended_task_count < QUEUE_THREAD_COUNT)
      TEST_ASSERT_EQUAL_INT(1, arg.count);
    else
      TEST_ASSERT_EQUAL_INT(QUEUE_THREAD_COUNT + 1, arg.count);

    // now wait for the last added extended task
    dispatch_queue_task_wait(queue, extended_tasks[extended_task_count - 1]);
    TEST_ASSERT_EQUAL_INT(extended_task_count + 1, arg.count);

    // cleanup
    for (int j = 0; j < extended_task_count - 1; j++) {
      dispatch_task_destroy(extended_tasks[j]);
    }
    dispatch_queue_destroy(queue);
    free(extended_tasks);
  }
}

TEST(dispatch_queue, test_mixed_durations2) {
  dispatch_queue_t *queue;
  dispatch_task_t *extended_task1;
  dispatch_task_t *extended_task2;
  dispatch_group_t *limited_group;
  test_work_arg_t extended_arg;
  test_work_arg_t limited_arg;
  int queue_length = QUEUE_LENGTH;
  int queue_thread_count = QUEUE_THREAD_COUNT;

  queue = dispatch_queue_create(queue_length, queue_thread_count,
                                QUEUE_THREAD_STACK_SIZE, QUEUE_THREAD_PRIORITY);
  limited_group = dispatch_group_create(3, true);

  extended_task1 = dispatch_task_create(do_extended_work, &extended_arg, true);
  extended_task2 = dispatch_task_create(do_extended_work, &extended_arg, true);
  dispatch_group_function_add(limited_group, do_limited_work, &limited_arg);
  dispatch_group_function_add(limited_group, do_limited_work, &limited_arg);
  dispatch_group_function_add(limited_group, do_limited_work, &limited_arg);

  extended_arg.count = 0;
  limited_arg.count = 0;

  // add first extended task
  dispatch_queue_task_add(queue, extended_task1);

  // now wait for the first extended task
  dispatch_queue_task_wait(queue, extended_task1);
  TEST_ASSERT_EQUAL_INT(1, extended_arg.count);

  // add limited task group, and second extended task
  dispatch_queue_group_add(queue, limited_group);
  dispatch_queue_task_add(queue, extended_task2);

  // now wait for the limited tasks
  dispatch_queue_group_wait(queue, limited_group);
  TEST_ASSERT_EQUAL_INT(3, limited_arg.count);

  // now wait for the second extended task
  dispatch_queue_task_wait(queue, extended_task2);
  TEST_ASSERT_EQUAL_INT(2, extended_arg.count);

  dispatch_group_destroy(limited_group);
  dispatch_queue_destroy(queue);
}

TEST_GROUP_RUNNER(dispatch_queue) {
  RUN_TEST_CASE(dispatch_queue, test_wait_queue);
  RUN_TEST_CASE(dispatch_queue, test_wait_task);
  RUN_TEST_CASE(dispatch_queue, test_wait_group);
  RUN_TEST_CASE(dispatch_queue, test_mixed_durations1);
  RUN_TEST_CASE(dispatch_queue, test_mixed_durations2);
}