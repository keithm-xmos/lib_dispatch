// Copyright (c) 2020, XMOS Ltd, All rights reserved

#include "test_dispatch_queue.h"

#include "lib_dispatch/api/dispatch.h"
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

TEST(dispatch_queue, test_wait_task) {
  dispatch_queue_t *queue;
  dispatch_task_t task;
  test_work_arg_t arg;
  int queue_length = QUEUE_LENGTH;
  int queue_thread_count = QUEUE_THREAD_COUNT;

  queue = dispatch_queue_create(queue_length, queue_thread_count,
                                QUEUE_STACK_SIZE, "test_wait_task");
  dispatch_task_init(&task, do_standard_work, &arg);

  arg.count = 0;

  dispatch_queue_add_task(queue, &task);
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
                                QUEUE_STACK_SIZE, "test_add_group");
  group = dispatch_group_create(group_length);
  dispatch_task_init(&task, do_standard_work, &arg);

  arg.count = 0;

  // add tasks to group
  for (int i = 0; i < group_length; i++) {
    dispatch_group_add(group, &task);
  }

  dispatch_queue_add_group(queue, group);
  dispatch_group_wait(group);

  TEST_ASSERT_EQUAL_INT(group_length, arg.count);

  dispatch_group_destroy(group);
  dispatch_queue_destroy(queue);
}

TEST(dispatch_queue, test_add_task) {
  dispatch_queue_t *queue;
  dispatch_task_t task;
  test_work_arg_t arg;
  int queue_length = QUEUE_LENGTH;
  int queue_thread_count = QUEUE_THREAD_COUNT;
  int task_count = 8;

  queue = dispatch_queue_create(queue_length, queue_thread_count,
                                QUEUE_STACK_SIZE, "test_add_task");
  dispatch_task_init(&task, do_standard_work, &arg);

  arg.count = 0;
  for (int i = 0; i < task_count; i++) {
    dispatch_queue_add_task(queue, &task);
  }
  dispatch_queue_wait(queue);

  TEST_ASSERT_EQUAL_INT(task_count, arg.count);

  dispatch_queue_destroy(queue);
}

TEST(dispatch_queue, test_add_function) {
  dispatch_queue_t *queue;
  test_work_arg_t arg;
  int queue_length = QUEUE_LENGTH;
  int queue_thread_count = QUEUE_THREAD_COUNT;
  int function_count = 3;

  queue = dispatch_queue_create(queue_length, queue_thread_count,
                                QUEUE_STACK_SIZE, "test_add_task");

  arg.count = 0;
  for (int i = 0; i < function_count; i++) {
    dispatch_queue_add_function(queue, do_standard_work, &arg);
  }
  dispatch_queue_wait(queue);

  TEST_ASSERT_EQUAL_INT(function_count, arg.count);

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
  dispatch_task_init(&task, do_standard_work, &arg);

  arg.count = 0;
  dispatch_queue_for(queue, for_count, &task);
  dispatch_queue_wait(queue);

  TEST_ASSERT_EQUAL_INT(for_count, arg.count);

  dispatch_queue_destroy(queue);
}

TEST(dispatch_queue, test_add_group) {
  dispatch_queue_t *queue;
  dispatch_group_t *group;
  dispatch_task_t task;
  test_work_arg_t arg;
  int queue_length = QUEUE_LENGTH;
  int queue_thread_count = QUEUE_THREAD_COUNT;
  int group_length = 3;
  int group_count = 2;

  queue = dispatch_queue_create(queue_length, queue_thread_count,
                                QUEUE_STACK_SIZE, "test_add_group");
  group = dispatch_group_create(group_length);
  dispatch_task_init(&task, do_standard_work, &arg);

  arg.count = 0;

  // add tasks to group
  for (int i = 0; i < group_length; i++) {
    dispatch_group_add(group, &task);
  }

  for (int i = 0; i < group_count; i++) {
    dispatch_queue_add_group(queue, group);
    dispatch_group_wait(group);
  }

  TEST_ASSERT_EQUAL_INT((group_length * group_count), arg.count);

  dispatch_group_destroy(group);
  dispatch_queue_destroy(queue);
}

TEST(dispatch_queue, test_mixed_durations1) {
  dispatch_queue_t *queue;
  dispatch_task_t limited_task;
  dispatch_task_t *extended_tasks;
  test_work_arg_t arg;
  int queue_length = QUEUE_LENGTH;
  int queue_thread_count = QUEUE_THREAD_COUNT;
  int max_extended_task_count = QUEUE_THREAD_COUNT + 1;
  int extended_task_count;

  for (int i = 0; i < max_extended_task_count; i++) {
    queue = dispatch_queue_create(queue_length, queue_thread_count,
                                  QUEUE_STACK_SIZE, "test_mixed_durations1");

    extended_task_count = i + 1;

    extended_tasks = malloc(extended_task_count * sizeof(dispatch_task_t));

    for (int j = 0; j < extended_task_count; j++) {
      dispatch_task_init(&extended_tasks[j], do_extended_work, &arg);
    }

    dispatch_task_init(&limited_task, do_standard_work, &arg);

    arg.count = 0;

    // add extended tasks
    for (int j = 0; j < extended_task_count; j++) {
      dispatch_queue_add_task(queue, &extended_tasks[j]);
    }

    // add limited task
    dispatch_queue_add_task(queue, &limited_task);

    // now wait for the limited task
    dispatch_queue_task_wait(queue, limited_task.id);
    if (extended_task_count < QUEUE_THREAD_COUNT)
      TEST_ASSERT_EQUAL_INT(1, arg.count);
    else
      TEST_ASSERT_EQUAL_INT(QUEUE_THREAD_COUNT + 1, arg.count);

    // now wait for the extended task
    dispatch_queue_task_wait(queue, extended_tasks[i].id);
    TEST_ASSERT_EQUAL_INT(extended_task_count + 1, arg.count);

    free(extended_tasks);
    dispatch_queue_destroy(queue);
  }
}

TEST(dispatch_queue, test_mixed_durations2) {
  dispatch_queue_t *queue;
  dispatch_task_t extended_task1;
  dispatch_task_t extended_task2;
  dispatch_task_t limited_task1;
  dispatch_task_t limited_task2;
  dispatch_task_t limited_task3;
  dispatch_group_t *limited_group;
  test_work_arg_t extended_arg;
  test_work_arg_t limited_arg;
  int queue_length = QUEUE_LENGTH;
  int queue_thread_count = QUEUE_THREAD_COUNT;

  queue = dispatch_queue_create(queue_length, queue_thread_count,
                                QUEUE_STACK_SIZE, "test_mixed_durations2");
  limited_group = dispatch_group_create(3);

  dispatch_task_init(&extended_task1, do_extended_work, &extended_arg);
  dispatch_task_init(&extended_task2, do_extended_work, &extended_arg);
  dispatch_task_init(&limited_task1, do_limited_work, &limited_arg);
  dispatch_task_init(&limited_task2, do_limited_work, &limited_arg);
  dispatch_task_init(&limited_task3, do_limited_work, &limited_arg);
  dispatch_group_add(limited_group, &limited_task1);
  dispatch_group_add(limited_group, &limited_task2);
  dispatch_group_add(limited_group, &limited_task3);

  extended_arg.count = 0;
  limited_arg.count = 0;

  // add first extended task
  dispatch_queue_add_task(queue, &extended_task1);

  // now wait for the first extended task
  dispatch_task_wait(&extended_task1);
  TEST_ASSERT_EQUAL_INT(1, extended_arg.count);

  // add limited task group, and second extended task
  dispatch_queue_add_group(queue, limited_group);
  dispatch_queue_add_task(queue, &extended_task2);

  // now wait for the limited tasks
  dispatch_group_wait(limited_group);
  TEST_ASSERT_EQUAL_INT(3, limited_arg.count);

  // now wait for the second extended task
  dispatch_task_wait(&extended_task2);
  TEST_ASSERT_EQUAL_INT(2, extended_arg.count);

  dispatch_group_destroy(limited_group);
  dispatch_queue_destroy(queue);
}

TEST_GROUP_RUNNER(dispatch_queue) {
  RUN_TEST_CASE(dispatch_queue, test_wait_task);
  RUN_TEST_CASE(dispatch_queue, test_wait_group);
  RUN_TEST_CASE(dispatch_queue, test_add_task);
  RUN_TEST_CASE(dispatch_queue, test_add_function);
  RUN_TEST_CASE(dispatch_queue, test_mixed_durations1);
  RUN_TEST_CASE(dispatch_queue, test_mixed_durations2);
  RUN_TEST_CASE(dispatch_queue, test_add_group);
  RUN_TEST_CASE(dispatch_queue, test_for);
}