// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef DISPATCH_TASK_H_
#define DISPATCH_TASK_H_

#include <stddef.h>

#ifdef XCORE
#define DISPATCH_TASK_FUNCTION __attribute__((fptrgroup("dispatch_function")))
#else
#define DISPATCH_TASK_FUNCTION
#endif

#define DISPATCH_TASK_NONE (0)
#define VALID_TASK_ID(X) (X > DISPATCH_TASK_NONE)
#define INVALID_TASK_ID(X) (X <= DISPATCH_TASK_NONE)
#define MIN_TASK_ID(X, Y) (((X) < (Y)) ? (X) : (Y))

typedef void (*dispatch_function_t)(void *);

struct dispatch_queue_struct;

typedef struct dispatch_task_struct dispatch_task_t;
struct dispatch_task_struct {
  dispatch_function_t fn;               // the function to perform
  void *arg;                            // argument to pass to the function
  struct dispatch_queue_struct *queue;  // parent queue
  size_t id;                            // unique identifier
};

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** Initialize a new task
 *
 * @param[in,out] ctx  Task object
 * @param[in] fn       Function to perform, signature must be void my_fun(void
 * *arg)
 * @param[in] arg      Function argument
 */
void dispatch_task_init(dispatch_task_t *ctx, dispatch_function_t fn,
                        void *arg);

/** Run the task in the caller's thread
 *
 * @param[in] ctx  Task object
 */
void dispatch_task_perform(dispatch_task_t *ctx);

/** Wait synchronously in the caller's thread for the task to finish executing
 *
 * @param[in] ctx  Task object
 */
void dispatch_task_wait(dispatch_task_t *ctx);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // DISPATCH_TASK_H_