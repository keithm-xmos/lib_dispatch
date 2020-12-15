// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef LIB_DISPATCH_H_
#define LIB_DISPATCH_H_

// **********
// task API
// **********

typedef struct dispatch_task {
  char *name;                    // to identify it when debugging
  void (*work)(void *);          // the function to perform
  void *params;                  // parameters to pass to the function
  struct dispatch_task *notify;  // the task to notify
} dispatch_task_t;

// Create a new task with function pointer, void pointer arument, and name (for
// debugging)
void dispatch_task_create(dispatch_task_t *task, void (*work)(void *),
                          void *params, char *name);

// Schedules the execution of the notify task after the completion of the
// current task
void dispatch_task_notify(dispatch_task_t *current_task,
                          dispatch_task_t *notify_task);

// Causes the caller to wait synchronously until the task finishes executing
void dispatch_task_wait(dispatch_task_t *task);

// **********
// group API
// **********
// typedef struct dispatch_group {
//   char name[64];  // to identify it when debugging
//   // void (*notify)(void *);  // the function to notify
//   dispatch_task_t *notify_task;    // the task to notify
//   dispatch_group_t *notify_group;  // the group to notify
// } dispatch_group_t;

// void dispatch_group_create(dispatch_group_t *group, char *name);

// void dispatch_group_task_notify(dispatch_group_t *current_group,
//                                 dispatch_task_t *notify_task);

// void dispatch_group_group_notify(dispatch_group_t *current_group,
//                                  dispatch_group_t *notify_group);

// void dispatch_group_wait(dispatch_group_t *group);

// **********
// queue API
// **********
// typedef struct dispatch_queue_item {
//   dispatch_task_t *task;        // the task associated with this item
//   dispatch_queue_item_t *next;  // pointer to the next item in the queue
// } dispatch_queue_item_t;

// typedef struct dispatch_queue {
//   thread_t *thread_pool;       // the thread pool associated with the queue
//   int pool_size;               // the size of the thread pool
//   thread_mutex_t queue_mutex;  // mutex used to synchronise access to the
//   queue thread_cond_t queue_cond;    // condition variable used to signal
//   events to
//                                // the threads in the thread pool
//   dispatch_queue_item_t *front;  // pointer to the first item in the queue
//   dispatch_queue_item_t *back;   // pointer to the last item in the queue
//   volatile bool shutdown;  // flag to indicate if the queue has been asked to
//                            // shutdown (destroy)
//   volatile bool waiting;   // flag to indicate if the queue is waiting for
//   all
//                            // currently queued tasks to finish
// } dispatch_queue_t;

// void dispatch_queue_create(dispatch_queue_t *queue);

// void dispatch_queue_destroy(dispatch_queue_t *queue);

// // Run task asyncronously
// int dispatch_queue_async(dispatch_queue_t *queue, dispatch_task_t *task);

// // Run task syncronously
// int dispatch_queue_sync(dispatch_queue_t *queue, dispatch_task_t *task);

// // Run task N times
// // void dispatch_queue_for(dispatch_queue_t *queue, int N, dispatch_task_t
// // *task);

// // Wait for all tasks to be finish
// int dispatch_queue_wait(dispatch_queue_t *queue);

#endif  // LIB_DISPATCH_H_