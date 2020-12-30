// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef DISPATCH_GROUP_H_
#define DISPATCH_GROUP_H_

#include <stddef.h>

#include "lib_dispatch/api/dispatch_task.h"

#define DISPATCH_GROUP_NONE (0)

struct dispatch_queue_struct;

typedef struct dispatch_group_struct dispatch_group_t;
struct dispatch_group_struct {
  size_t id;
  size_t length;
  size_t count;
  dispatch_task_t *tasks;
  dispatch_task_t *notify_task;                // the task to notify
  struct dispatch_group_struct *notify_group;  // the group to notify
  struct dispatch_queue_struct *queue;         // parent queue
};

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** Create a new task group
 *
 * @param[in] length       Maximum number of tasks in the group
 *
 * @return    New group object
 */
dispatch_group_t *dispatch_group_create(size_t length);

/** Initialize a new task group
 *
 * @param[in] ctx       Group object
 */
size_t dispatch_group_init(dispatch_group_t *ctx);

/** Free memory allocated by dispatch_group_create
 *
 * @param[in] ctx       Group object
 */
void dispatch_group_destroy(dispatch_group_t *ctx);

/** Add a task to the group
 *
 * @param[in] ctx       Group object
 * @param[in] task      Task object
 */
void dispatch_group_add(dispatch_group_t *ctx, dispatch_task_t *task);

/** Schedules the execution of the notify group after the completion of the
 * current group
 *
 * @param[in] ctx       Group object
 * @param[in] group     Group to notify
 */
void dispatch_group_notify_group(dispatch_group_t *ctx,
                                 dispatch_group_t *group);

/** Schedules the execution of the notify task after the completion of the
 * current group
 *
 * @param[in] ctx       Group object
 * @param[in] task      Task to notify
 */
void dispatch_group_notify_task(dispatch_group_t *ctx, dispatch_task_t *task);

/** Run the group's tasks in the caller's thread
 *
 * @param[in] ctx       Group object
 */
void dispatch_group_perform(dispatch_group_t *ctx);

/** Wait synchronously in the caller's thread for the group's tasks to finish
 * executing
 *
 * @param[in] ctx       Group object
 */
void dispatch_group_wait(dispatch_group_t *ctx);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // DISPATCH_GROUP_H_