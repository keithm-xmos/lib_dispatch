// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "lib_dispatch/api/dispatch_queue_host.h"

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "lib_dispatch/api/dispatch_group.h"
#include "lib_dispatch/api/dispatch_queue.h"
#include "lib_dispatch/api/dispatch_task.h"

#define DISPATCH_TASK_NONE (NULL)

void dispatch_thread_handler(dispatch_host_queue_t *q,
                             dispatch_task_t **current_task) {
  std::unique_lock<std::mutex> lock(q->lock);

  do {
    // Wait until we have data or a quit signal
    q->cv.wait(lock, [q] { return (q->queue.size() || q->quit); });

    // after wait, we own the lock
    if (!q->quit && q->queue.size()) {
      dispatch_task_t *task = q->queue.front();

      // set the current task
      *current_task = task;

      q->queue.pop_front();

      // unlock now that we're done messing with the queue
      lock.unlock();

      dispatch_task_perform(task);

      // reset current task
      *current_task = DISPATCH_TASK_NONE;

      lock.lock();
    }
  } while (!q->quit);
}

dispatch_queue_t *dispatch_queue_create(size_t length, size_t thread_count,
                                        size_t stack_size, const char *name) {
  dispatch_host_queue_t *queue;

  std::printf("dispatch_queue_create: length=%d, thread_count=%d\n", length,
              thread_count);

  queue = new dispatch_host_queue_t;
  queue->threads.resize(thread_count);
  queue->thread_tasks.resize(thread_count);

  if (name)
    queue->name.assign(name);
  else
    queue->name.assign("null");

  // initialize the queue
  dispatch_queue_init(queue);

  std::printf("dispatch_queue_create: name=%s\n", queue->name.c_str());

  return queue;
}

void dispatch_queue_init(dispatch_queue_t *ctx) {
  assert(ctx);
  dispatch_host_queue_t *q = static_cast<dispatch_host_queue_t *>(ctx);

  std::printf("dispatch_queue_init: name=%s\n", q->name.c_str());

  q->quit = false;
  for (size_t i = 0; i < q->threads.size(); i++) {
    q->thread_tasks[i] = DISPATCH_TASK_NONE;
    q->threads[i] =
        std::thread(&dispatch_thread_handler, q, &q->thread_tasks[i]);
  }
}

void dispatch_queue_async_task(dispatch_queue_t *ctx, dispatch_task_t *task) {
  assert(ctx);
  assert(task);
  dispatch_host_queue_t *q = static_cast<dispatch_host_queue_t *>(ctx);

  std::printf("dispatch_queue_async_task: name=%s\n", q->name.c_str());

  // assign to this queue
  task->queue = static_cast<dispatch_queue_struct *>(ctx);

  std::unique_lock<std::mutex> lock(q->lock);
  q->queue.push_back(task);
  // manual unlocking is done before notifying, to avoid waking up
  // the waiting thread only to block again (see notify_one for details)
  lock.unlock();

  q->cv.notify_one();
}

void dispatch_queue_wait(dispatch_queue_t *ctx) {
  assert(ctx);
  dispatch_host_queue_t *q = static_cast<dispatch_host_queue_t *>(ctx);

  std::printf("dispatch_queue_wait: name=%s\n", q->name.c_str());

  int busy_count = 0;

  for (;;) {
    std::unique_lock<std::mutex> lock(q->lock);
    busy_count = q->queue.size();  // tasks on the queue are considered busy
    lock.unlock();
    for (int i = 0; i < q->threads.size(); i++) {
      if (q->thread_tasks[i] != DISPATCH_TASK_NONE) busy_count++;
    }
    if (busy_count == 0) return;
  }
}

dispatch_queue_status_t dispatch_queue_task_status(dispatch_queue_t *ctx,
                                                   dispatch_task_t *task) {
  assert(ctx);
  dispatch_host_queue_t *q = static_cast<dispatch_host_queue_t *>(ctx);

  bool waiting = false;
  std::unique_lock<std::mutex> lock(q->lock);
  for (int i = 0; i < q->queue.size(); i++) {
    dispatch_task_t *queued_task = q->queue.at(i);

    if (queued_task == task) {
      waiting = true;
      break;
    }
  }
  lock.unlock();

  if (waiting) return DISPATCH_QUEUE_WAITING;

  for (int i = 0; i < q->threads.size(); i++) {
    if (q->thread_tasks[i] == task) {
      return DISPATCH_QUEUE_EXECUTING;
    }
  }
  return DISPATCH_QUEUE_DONE;
}

dispatch_queue_status_t dispatch_queue_group_status(dispatch_queue_t *ctx,
                                                    dispatch_group_t *group) {
  assert(ctx);

  int busy_count = 0;
  dispatch_queue_status_t task_status;

  for (int i = 0; i < group->length; i++) {
    dispatch_task_t *task = &group->tasks[i];

    task_status = dispatch_queue_task_status(ctx, task);
    if (task_status != DISPATCH_QUEUE_DONE) busy_count++;
  }
  if (busy_count == 0) return DISPATCH_QUEUE_DONE;

  return DISPATCH_QUEUE_WAITING;
}

void dispatch_queue_destroy(dispatch_queue_t *ctx) {
  assert(ctx);
  dispatch_host_queue_t *q = static_cast<dispatch_host_queue_t *>(ctx);

  std::printf("dispatch_queue_destroy: name=%s\n", q->name.c_str());

  // signal to all thread workers that it is time to quit
  std::unique_lock<std::mutex> lock(q->lock);
  q->quit = true;
  lock.unlock();
  q->cv.notify_all();

  // Wait for threads to finish before we exit
  for (size_t i = 0; i < q->threads.size(); i++) {
    if (q->threads[i].joinable()) {
      // printf("Destructor: Joining thread %zu until completion\n", i);
      q->threads[i].join();
    }
  }

  // free memory
  delete q;
}
