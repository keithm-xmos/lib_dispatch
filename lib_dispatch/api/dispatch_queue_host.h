// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef LIB_DISPATCH_HOST_H_
#define LIB_DISPATCH_HOST_H_

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
  std::deque<dispatch_task_t> queue;
  std::condition_variable cv;
  bool quit;
};

#endif  // LIB_DISPATCH_HOST_H_