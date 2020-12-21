// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "unity.h"
#include "unity_fixture.h"

static void RunTests(void) {
  RUN_TEST_GROUP(dispatch_task);
  RUN_TEST_GROUP(dispatch_group);
#if defined(XCORE)
  RUN_TEST_GROUP(dispatch_xcore);
#elif defined(HOST)
  RUN_TEST_GROUP(dispatch_host);
#elif defined(FREERTOS)
  RUN_TEST_GROUP(dispatch_freertos);
#endif
}

int main(int argc, const char* argv[]) {
  UnityGetCommandLineOptions(argc, argv);
  UnityBegin(argv[0]);
  RunTests();
  UnityEnd();

  return (int)Unity.TestFailures;
}