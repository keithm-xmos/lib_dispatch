// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef DISPATCH_TASK_H_
#define DISPATCH_TASK_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef XCORE
#define DISPATCH_TASK_FUNCTION __attribute__((fptrgroup("dispatch_function")))
#else
#define DISPATCH_TASK_FUNCTION
#endif

typedef void (*dispatch_function_t)(void *);

typedef struct dispatch_task_struct dispatch_task_t;

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** Create a new task, non-waitable task
 *
 * @param[in] function  Function to perform, signature must be void my_fun(void
 * *arg)
 * @param[in] argument  Function argument
 * @param[in] waitable  The task is waitable if TRUE, otherwise the task can not
 * be waited on
 *
 * @return              Task object
 */
dispatch_task_t *dispatch_task_create(dispatch_function_t function,
                                      void *aargumentrg, bool waitable);

/** Initialize a task
 *
 * @param[in,out] task  Task object
 * @param[in] function  Function to perform, signature must be void my_fun(void
 * *arg)
 * @param[in] argument  Function argument
 * @param[in] waitable  The task is waitable if TRUE, otherwise the task can not
 * be waited on
 */
void dispatch_task_init(dispatch_task_t *task, dispatch_function_t function,
                        void *argument, bool waitable);

/** Run the task in the caller's thread
 *
 * @param[in] task  Task object
 */
void dispatch_task_perform(dispatch_task_t *task);

/** Destroy the task
 *
 * @param[in] task  Task object
 */
void dispatch_task_destroy(dispatch_task_t *task);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // DISPATCH_TASK_H_