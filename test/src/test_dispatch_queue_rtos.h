// Copyright 2021 XMOS LIMITED. This Software is subject to the terms of the 
// XMOS Public License: Version 1
#ifndef TEST_DISPATCH_QUEUE_FREERTOS_H_
#define TEST_DISPATCH_QUEUE_FREERTOS_H_

#include "FreeRTOS.h"

#define QUEUE_LENGTH (10)
#define QUEUE_THREAD_COUNT (3)
#define QUEUE_THREAD_STACK_SIZE (1024)  // more than enough
#define QUEUE_THREAD_PRIORITY (configMAX_PRIORITIES - 1)

void look_busy(int milliseconds);

inline void look_busy(int milliseconds) {
  const TickType_t xDelay = milliseconds / portTICK_PERIOD_MS;
  vTaskDelay(xDelay);
}

#endif  // TEST_DISPATCH_QUEUE_FREERTOS_H_
