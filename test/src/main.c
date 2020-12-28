// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "unity.h"
#include "unity_fixture.h"

#if FREERTOS
#include "FreeRTOS.h"
#endif

static void RunTests(void) {
  RUN_TEST_GROUP(dispatch_task);
  RUN_TEST_GROUP(dispatch_group);
  RUN_TEST_GROUP(dispatch_queue);
#if defined(XCORE)
  RUN_TEST_GROUP(dispatch_queue_xcore);
#endif
#if defined(FREERTOS)
  RUN_TEST_GROUP(dispatch_queue_freertos);
#endif
}

int main(int argc, const char* argv[]) {
#if FREERTOS
  vTaskStartScheduler();
#endif

  UnityGetCommandLineOptions(argc, argv);
  UnityBegin(argv[0]);
  RunTests();
  UnityEnd();

  return (int)Unity.TestFailures;
}