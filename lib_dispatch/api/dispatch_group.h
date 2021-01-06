// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef DISPATCH_GROUP_H_
#define DISPATCH_GROUP_H_

#include <stdbool.h>
#include <stddef.h>

#include "dispatch_task.h"

typedef struct dispatch_group_struct dispatch_group_t;

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** Create a new task group
 *
 * @param[in] length    Maximum number of tasks in the group
 * @param[in] waitable  The task is waitable if TRUE, otherwise the task can not
 *
 * @return            Group object
 */
dispatch_group_t *dispatch_group_create(size_t length, bool waitable);

/** Initialize a new task group
 *
 * @param[in,out] group  Group object
 * @param[in] waitable   The group is waitable if TRUE, otherwise the group can
 * not be waited on
 */
void dispatch_group_init(dispatch_group_t *group, bool waitable);

/** Free memory allocated by dispatch_group_create
 *
 * @param[in] group  Group object
 */
void dispatch_group_destroy(dispatch_group_t *group);

/** Creates a task and adds it to the the group
 *
 * @param[in] group     Group object
 * @param[in] function  Function to perform, signature must be void my_fun(void
 * *arg)
 * @param[in] argument  Function argument
 *
 * @return              Task object
 */
dispatch_task_t *dispatch_group_function_add(dispatch_group_t *group,
                                             dispatch_function_t function,
                                             void *argument);

/** Add a task to the group
 *
 * @param[in] group  Group object
 * @param[in] task   Task to add
 */
void dispatch_group_task_add(dispatch_group_t *group, dispatch_task_t *task);

/** Run the group's tasks in the caller's thread
 *
 * @param[in] group  Group object
 */
void dispatch_group_perform(dispatch_group_t *group);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // DISPATCH_GROUP_H_