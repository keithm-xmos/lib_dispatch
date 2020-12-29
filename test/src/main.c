// Copyright (c) 2020, XMOS Ltd, All rights reserved
#if FREERTOS
#include "FreeRTOS.h"
#include "task.h"
//#define UNITY_MALLOC(size) pvPortMalloc(size)
//#define UNITY_FREE(ptr) vPortFree(ptr)
void vApplicationMallocFailedHook(void) { debug_printf("Malloc failed!\n"); }
#endif

#include "unity.h"
#include "unity_fixture.h"

#if FREERTOS
static void RunTests(void* unused) {
  RUN_TEST_GROUP(dispatch_task);
  RUN_TEST_GROUP(dispatch_group);
  RUN_TEST_GROUP(dispatch_queue);
  RUN_TEST_GROUP(dispatch_queue_freertos);
  UnityEnd();
  exit(Unity.TestFailures);
}
#endif

#if defined(XCORE)
static void RunTests(void* unused) {
  RUN_TEST_GROUP(dispatch_task);
  RUN_TEST_GROUP(dispatch_group);
  RUN_TEST_GROUP(dispatch_queue);
  RUN_TEST_GROUP(dispatch_queue_xcore);
  UnityEnd();
}
#endif

#if defined(HOST)
static void RunTests(void* unused) {
  RUN_TEST_GROUP(dispatch_task);
  RUN_TEST_GROUP(dispatch_group);
  RUN_TEST_GROUP(dispatch_queue);
  UnityEnd();
}
#endif

int main(int argc, const char* argv[]) {
  UnityGetCommandLineOptions(argc, argv);
  UnityBegin(argv[0]);

#if FREERTOS
  xTaskCreate(RunTests, "RunTests", 1024 * 16, NULL, configMAX_PRIORITIES,
              NULL);
  vTaskStartScheduler();
#endif
  // for FreeRTOS build we never reach here because vTaskStartScheduler never
  // returns
  RunTests(NULL);
  return (int)Unity.TestFailures;
}