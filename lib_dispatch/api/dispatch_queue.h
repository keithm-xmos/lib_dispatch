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
 * @param[in] length        Maximum number of tasks in the queue
 * @param[in] thread_count  Number of thread workers
 * @param[in] stack_size    Size (in words) of the stack for each thread worker
 * @param[in] name          Queue name used for debugging
 *
 * @return                  New dispatch queue object
 */
dispatch_queue_t* dispatch_queue_create(size_t length, size_t thread_count,
                                        size_t stack_size, const char* name);

/** Initialize a new dispatch queue
 *
 * @param[in,out] ctx  Dispatch queue object
 */
void dispatch_queue_init(dispatch_queue_t* ctx);

/** Free memory allocated by dispatch_queue_create
 *
 * @param[in] ctx  Dispatch queue object
 */
void dispatch_queue_destroy(dispatch_queue_t* ctx);

/** Add a task to the dispatch queue
 *
 * Note: The XCORE bare-metal implementation is currently limited and does not
 * store queued tasks in a container.  Instead, this function blocks the callers
 * thread until a worker thread is available.  This limitation may be eliminated
 * in a future release.
 *
 * @param[in] ctx   Dispatch queue object
 * @param[in] task  Task object
 *
 * @return          Task ID that can be used in a call to
 * dispatch_queue_task_wait
 */
size_t dispatch_queue_add_task(dispatch_queue_t* ctx, dispatch_task_t* task);

/** Add a group to the dispatch queue
 *
 * @param[in] ctx    Dispatch queue object
 * @param[in] group  Group object
 *
 */
void dispatch_queue_add_group(dispatch_queue_t* ctx, dispatch_group_t* group);

/** Create a task and add to the dispatch queue
 *
 * @param[in] ctx Dispatch queue object
 * @param[in] fn  Function to perform, signature must be void my_fun(void
 * *arg)
 * @param[in] arg  Function argument
 *
 * @return         Task ID that can be used in a call to
 * dispatch_queue_task_wait
 */
size_t dispatch_queue_add_function(dispatch_queue_t* ctx,
                                   dispatch_function_t fn, void* arg);

/** Run a task asyncronously N times
 *
 * @param[in] ctx   Dispatch queue object
 * @param[in] N     Number of iterations to run the task
 * @param[in] task  Task object
 *
 * @return          Task ID that can be used in a call to
 * dispatch_queue_task_wait
 */
size_t dispatch_queue_for_task(dispatch_queue_t* ctx, int N,
                               dispatch_task_t* task);

/** Create and run a task asyncronously N times
 *
 * @param[in] ctx   Dispatch queue object
 * @param[in] N     Number of iterations to run the task
 * @param[in] fn  Function to perform, signature must be void my_fun(void
 * *arg)
 * @param[in] arg  Function argument
 *
 * @return          Task ID that can be used in a call to
 * dispatch_queue_task_wait
 */
size_t dispatch_queue_for_function(dispatch_queue_t* ctx, int N,
                                   dispatch_function_t fn, void* arg);

/** Wait synchronously in the caller's thread for the task with the given ID to
 * finish executing
 *
 * @param[in] ctx      Dispatch queue object
 * @param[in] task_id  Task ID
 */
void dispatch_queue_task_wait(dispatch_queue_t* ctx, int task_id);

/** Wait synchronously in the caller's thread for all tasks to finish executing
 *
 * @param[in] ctx  Dispatch queue object
 */
void dispatch_queue_wait(dispatch_queue_t* ctx);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // DISPATCH_QUEUE_H_