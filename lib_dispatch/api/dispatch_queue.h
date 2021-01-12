// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef DISPATCH_QUEUE_H_
#define DISPATCH_QUEUE_H_

#include <stddef.h>

#include "dispatch_group.h"
#include "dispatch_task.h"

typedef void dispatch_queue_t;

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** Create a new dispatch queue
 *
 * \param length             Maximum number of tasks in the queue
 * \param thread_count       Number of thread workers
 * \param thread_stack_size  Size (in words) of the stack for each thread
 * worker
 * \param thread_priority    Priority for each thread worker.
 *
 * @return                   New dispatch queue object
 */
dispatch_queue_t* dispatch_queue_create(size_t length, size_t thread_count,
                                        size_t thread_stack_size,
                                        size_t thread_priority);

/** Initialize a new dispatch queue
 *
 * \param ctx              Dispatch queue object
 * \param thread_priority  Priority for each thread worker.
 */
void dispatch_queue_init(dispatch_queue_t* ctx, size_t thread_priority);

/** Free memory allocated by dispatch_queue_create
 *
 * \param ctx  Dispatch queue object
 */
void dispatch_queue_destroy(dispatch_queue_t* ctx);

/** Add a task to the dispatch queue.  If the dispatch queue is full,
 * this function will block in the callers thread until it can be added to the
 * queue.
 *
 * \param ctx   Dispatch queue object
 * \param task  Task object
 *
 */
void dispatch_queue_task_add(dispatch_queue_t* ctx, dispatch_task_t* task);

/** Add a group to the dispatch queue.  If the dispatch queue is full,
 * this function will block in the callers thread until all the tasks in the
 * group can be added to the queue.
 *
 * \param ctx    Dispatch queue object
 * \param group  Group object
 *
 */
void dispatch_queue_group_add(dispatch_queue_t* ctx, dispatch_group_t* group);

/** Creates a task and adds it to the the queue.  If the dispatch queue is full,
 * this function will block in the callers thread until it can be added to the
 * queue.
 *
 * \param queue     Queue object
 * \param function  Function to perform, signature must be void my_fun(void
 * *arg)
 * \param argument  Function argument
 * \param waitable  The created task is waitable if TRUE, otherwise the
 * task can not be waited on
 *
 * \return          Task object
 */
static inline dispatch_task_t* dispatch_queue_function_add(
    dispatch_queue_t* ctx, dispatch_function_t function, void* argument,
    bool waitable) {
  dispatch_task_t* task;

  task = dispatch_task_create(function, argument, waitable);
  dispatch_queue_task_add(ctx, task);

  return task;
}

/** Wait synchronously in the caller's thread for the task to finish executing
 *
 * \param ctx   Dispatch queue object
 * \param task  Task object, must be waitable
 */
void dispatch_queue_task_wait(dispatch_queue_t* ctx, dispatch_task_t* task);

/** Wait synchronously in the caller's thread for the group to finish executing
 *
 * \param ctx    Dispatch queue object
 * \param group  Group object, must be waitable
 */
void dispatch_queue_group_wait(dispatch_queue_t* ctx, dispatch_group_t* group);

/** Wait synchronously in the caller's thread for all tasks to finish
 * executing
 *
 * \param ctx  Dispatch queue object
 */
void dispatch_queue_wait(dispatch_queue_t* ctx);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // DISPATCH_QUEUE_H_