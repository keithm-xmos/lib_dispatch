// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include <stdio.h>
#include <xcore/hwtimer.h>

#if FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#else
#include "debug_print.h"
#endif

#include "lib_dispatch/api/dispatch.h"

#define NUM_THREADS 4
#define ROWS 100  // must be a multiple of NUM_THREADS
#define COLUMNS 100

typedef struct worker_arg {
  int start_row;
  int end_row;
} worker_arg_t;

static int input_mat1[ROWS][COLUMNS];
static int input_mat2[ROWS][COLUMNS];
static int output_mat[ROWS][COLUMNS];

void reset_matrices() {
  for (int i = 0; i < ROWS; i++)
    for (int j = 0; j < COLUMNS; j++) {
      input_mat1[i][j] = 1;
      input_mat2[i][j] = 1;
      output_mat[i][j] = 0;
    }
}

void verify_output() {
  for (int i = 0; i < ROWS; i++)
    for (int j = 0; j < COLUMNS; j++) {
      if (output_mat[i][j] != ROWS) {
        debug_printf("Whoops! output_mat[%d][%d] equals %d, expected %d\n", i,
                     j, output_mat[i][j], ROWS);
      }
    }
}

DISPATCH_TASK_FUNCTION
void do_matrix_multiply(void* p) {
  worker_arg_t* arg = (worker_arg_t*)p;

  for (int i = arg->start_row; i < arg->end_row; i++)
    for (int j = 0; j < COLUMNS; j++)
      for (int k = 0; k < ROWS; k++)
        output_mat[i][j] += (input_mat1[i][k] * input_mat2[k][j]);
  return;
}

static int single_thread_mat_mul() {
  reset_matrices();

  worker_arg_t arg;
  hwtimer_t hwtimer;
  int ticks;

  arg.start_row = 0;
  arg.end_row = ROWS;

  hwtimer = hwtimer_alloc();
  ticks = hwtimer_get_time(hwtimer);

  do_matrix_multiply(&arg);

  ticks = hwtimer_get_time(hwtimer) - ticks;
  hwtimer_free(hwtimer);

  verify_output();

  return ticks;
}

static int multi_thread_mat_mul() {
  dispatch_queue_t* queue;
  dispatch_group_t* group;
  dispatch_task_t tasks[NUM_THREADS];
  worker_arg_t args[NUM_THREADS];
  int queue_length = NUM_THREADS;
  int queue_thread_count = NUM_THREADS;
  hwtimer_t hwtimer;
  int ticks;

  reset_matrices();

  // create the dispatch queue
  queue =
      dispatch_queue_create(queue_length, queue_thread_count, 1024, "mat_mul");

  // create the dispatch group
  group = dispatch_group_create(queue_thread_count);

  // initialize NUM_THREADS tasks, add them to the group
  int num_rows = ROWS / NUM_THREADS;
  for (int i = 0; i < NUM_THREADS; i++) {
    args[i].start_row = i * num_rows;
    args[i].end_row = args[i].start_row + num_rows;

    dispatch_task_init(&tasks[i], do_matrix_multiply, &args[i]);
    dispatch_group_add(group, &tasks[i]);
  }

  hwtimer = hwtimer_alloc();
  ticks = hwtimer_get_time(hwtimer);

  // add group to dispatch queue
  dispatch_queue_add_group(queue, group);
  // wait for all tasks in the group to finish executing
  dispatch_group_wait(group);

  ticks = hwtimer_get_time(hwtimer) - ticks;
  hwtimer_free(hwtimer);

  verify_output();

  // destroy the dispatch group and queue
  dispatch_group_destroy(group);
  dispatch_queue_destroy(queue);

  return ticks;
}

static void mat_mul(void* unused) {
  int single_thread_duration;
  int multi_thread_duration;

  single_thread_duration = single_thread_mat_mul();
  multi_thread_duration = multi_thread_mat_mul();

  printf("single thread duration = %d ticks\n", single_thread_duration);
  printf("multiply thread duration = %d ticks\n", multi_thread_duration);
  printf("speedup = %0.1fx\n",
         (float)single_thread_duration / (float)multi_thread_duration);
}

int main(int argc, const char* argv[]) {
#if FREERTOS
  xTaskCreate(mat_mul, "mat_mul", 16 * 1024, NULL, configMAX_PRIORITIES, NULL);
  vTaskStartScheduler();
#endif
  // for FreeRTOS build we never reach here because vTaskStartScheduler never
  // returns
  mat_mul(NULL);
  return 0;
}