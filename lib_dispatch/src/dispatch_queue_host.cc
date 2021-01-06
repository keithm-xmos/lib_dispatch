// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#include "dispatch_config.h"
#include "dispatch_group.h"
#include "dispatch_queue.h"
#include "dispatch_task.h"
#include "dispatch_types.h"

class BinarySemaphore {
 public:
  BinarySemaphore() : flag(false) {}

  void Take() const {
    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, [&]() -> bool { return flag; });
  }

  void Clear() {
    mutex.lock();
    flag = false;
    mutex.unlock();
  }

  void Give() {
    mutex.lock();
    flag = true;
    mutex.unlock();
    condition.notify_all();
  }

 private:
  bool flag;
  mutable std::mutex mutex;
  mutable std::condition_variable condition;
};

typedef struct dispatch_host_struct dispatch_host_queue_t;
struct dispatch_host_struct {
  std::mutex lock;
  std::condition_variable cv;
  std::vector<std::thread> threads;
  std::deque<dispatch_task_t *> deque;
  std::vector<BinarySemaphore *> thread_ready_semaphores;
  bool quit;
};

void dispatch_thread_handler(dispatch_host_queue_t *queue,
                             BinarySemaphore *ready_semaphore) {
  std::unique_lock<std::mutex> lock(queue->lock);

  dispatch_printf("dispatch_thread_handler started: queue=%u\n", (size_t)queue);
  // ready_semaphore->Give();

  do {
    // give the ready signal
    ready_semaphore->Give();

    // wait until we have data or a quit signal
    queue->cv.wait(lock,
                   [queue] { return (queue->deque.size() || queue->quit); });

    // after wait, we own the lock
    if (!queue->quit && queue->deque.size()) {
      // signal that not ready
      ready_semaphore->Clear();

      dispatch_task_t *task = queue->deque.front();

      queue->deque.pop_front();

      // unlock now that we're done messing with the queue
      lock.unlock();

      dispatch_task_perform(task);

      if (task->waitable) {
        // clear semaphore
        BinarySemaphore *task_semaphore =
            static_cast<BinarySemaphore *>(task->private_data);
        task_semaphore->Give();
      } else {
        // the contract is that the worker must destroy non-waitable tasks
        dispatch_task_destroy(task);
      }

      lock.lock();
    }
  } while (!queue->quit);
}

dispatch_queue_t *dispatch_queue_create(size_t length, size_t thread_count,
                                        size_t thread_stack_size,
                                        size_t thread_priority) {
  dispatch_host_queue_t *queue;

  dispatch_printf("dispatch_queue_create: length=%d, thread_count=%d\n", length,
                  thread_count);

  queue = new dispatch_host_queue_t;
  queue->threads.resize(thread_count);
  queue->thread_ready_semaphores.resize(thread_count);

  // initialize the queue
  dispatch_queue_init(queue, thread_priority);

  dispatch_printf("dispatch_queue_create: created queue=%u\n", (size_t)queue);

  return queue;
}

void dispatch_queue_init(dispatch_queue_t *ctx, size_t thread_priority) {
  dispatch_host_queue_t *queue = static_cast<dispatch_host_queue_t *>(ctx);
  dispatch_assert(queue);

  dispatch_printf("dispatch_queue_init: queue=%u\n", (size_t)queue);

  queue->quit = false;
  for (size_t i = 0; i < queue->threads.size(); i++) {
    queue->thread_ready_semaphores[i] = new BinarySemaphore();
    queue->threads[i] = std::thread(&dispatch_thread_handler, queue,
                                    queue->thread_ready_semaphores[i]);
  }
}

void dispatch_queue_task_add(dispatch_queue_t *ctx, dispatch_task_t *task) {
  dispatch_host_queue_t *queue = static_cast<dispatch_host_queue_t *>(ctx);
  dispatch_assert(queue);
  dispatch_assert(task);

  dispatch_printf("dispatch_queue_add_task: queue=%u\n", (size_t)queue);

  if (task->waitable) {
    task->private_data = new BinarySemaphore;
  }

  std::unique_lock<std::mutex> lock(queue->lock);
  queue->deque.push_back(task);
  // manual unlocking is done before notifying, to avoid waking up
  // the waiting thread only to block again (see notify_one for details)
  lock.unlock();

  queue->cv.notify_one();
}

void dispatch_queue_wait(dispatch_queue_t *ctx) {
  dispatch_host_queue_t *queue = static_cast<dispatch_host_queue_t *>(ctx);
  dispatch_assert(queue);

  dispatch_printf("dispatch_queue_wait: queue=%u\n", (size_t)queue);

  int waiting_count;

  // wait for deque to empty
  for (;;) {
    std::unique_lock<std::mutex> lock(queue->lock);
    waiting_count = queue->deque.size();
    lock.unlock();
    if (waiting_count == 0) break;
  }
  // wait for all workers to be ready
  for (size_t i = 0; i < queue->thread_ready_semaphores.size(); i++) {
    queue->thread_ready_semaphores[i]->Take();
  }
}

void dispatch_queue_task_wait(dispatch_queue_t *ctx, dispatch_task_t *task) {
  dispatch_host_queue_t *queue = (dispatch_host_queue_t *)ctx;
  dispatch_assert(queue);
  dispatch_assert(task);

  if (task->waitable) {
    BinarySemaphore *semaphore =
        static_cast<BinarySemaphore *>(task->private_data);
    // wait on the task's semaphore which signals that it is complete
    semaphore->Take();
    // the contract is that the dispatch queue must destroy waitable tasks
    delete semaphore;
    dispatch_task_destroy(task);
  }
}

void dispatch_queue_destroy(dispatch_queue_t *ctx) {
  dispatch_assert(ctx);
  dispatch_host_queue_t *queue = static_cast<dispatch_host_queue_t *>(ctx);

  dispatch_printf("dispatch_queue_destroy: queue=%u\n", (size_t)queue);

  // signal to all thread workers that it is time to quit
  std::unique_lock<std::mutex> lock(queue->lock);
  queue->quit = true;
  lock.unlock();
  queue->cv.notify_all();

  // Wait for threads to finish before we exit
  for (size_t i = 0; i < queue->threads.size(); i++) {
    if (queue->threads[i].joinable()) {
      // printf("Destructor: Joining thread %zu until completion\n", i);
      queue->threads[i].join();
    }
  }

  for (size_t i = 0; i < queue->thread_ready_semaphores.size(); i++) {
    delete queue->thread_ready_semaphores[i];
  }

  // free memory
  delete queue;
}
