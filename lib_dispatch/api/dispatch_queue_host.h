// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef DISPATCH_QUEUE_HOST_H_
#define DISPATCH_QUEUE_HOST_H_

#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "lib_dispatch/api/dispatch_task.h"

typedef struct dispatch_host_struct dispatch_host_queue_t;
struct dispatch_host_struct {
  std::string name;  // to identify it when debugging
  std::mutex lock;
  std::vector<std::thread> threads;
  std::vector<int> thread_task_ids;
  std::deque<dispatch_task_t> deque;
  std::condition_variable cv;
  size_t next_id;
  bool quit;
};

#endif  // DISPATCH_QUEUE_HOST_H_