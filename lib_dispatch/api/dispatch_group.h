// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef DISPATCH_GROUP_H_
#define DISPATCH_GROUP_H_

#include <stddef.h>

#include "lib_dispatch/api/dispatch_task.h"

struct dispatch_queue_struct;

typedef struct dispatch_group_struct dispatch_group_t;
struct dispatch_group_struct {
  size_t length;
  size_t count;
  dispatch_task_t *tasks;
  struct dispatch_queue_struct *queue;  // parent queue
};

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** Create a new task group
 *
 * @param[in] length  Maximum number of tasks in the group
 *
 * @return             Group ID
 */
dispatch_group_t *dispatch_group_create(size_t length);

/** Initialize a new task group
 *
 * @param[in,out] ctx  Group object
 */
void dispatch_group_init(dispatch_group_t *ctx);

/** Free memory allocated by dispatch_group_create
 *
 * @param[in] ctx  Group object
 */
void dispatch_group_destroy(dispatch_group_t *ctx);

/** Add a task to the group
 *
 * @param[in] ctx  Group object
 * @param[in] task Task to add
 */
void dispatch_group_task_add(dispatch_group_t *ctx, dispatch_task_t *task);

/** Create a task and add it to the group
 *
 * @param[in] ctx  Group object
 * @param[in] fn   Function to perform, signature must be void my_fun(void
 * *arg)
 * @param[in] arg  Function argument
 */
void dispatch_group_function_add(dispatch_group_t *ctx, dispatch_function_t fn,
                                 void *arg);

/** Run the group's tasks in the caller's thread
 *
 * @param[in] ctx  Group object
 */
void dispatch_group_perform(dispatch_group_t *ctx);

/** Wait synchronously in the caller's thread for the group's tasks to finish
 * executing
 *
 * @param[in] ctx  Group object
 */
void dispatch_group_wait(dispatch_group_t *ctx);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // DISPATCH_GROUP_H_