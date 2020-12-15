// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "unity.h"
#include "unity_fixture.h"

static void RunTests(void) { RUN_TEST_GROUP(task); }

int main(int argc, const char* argv[]) {
  UnityGetCommandLineOptions(argc, argv);
  UnityBegin(argv[0]);
  RunTests();
  UnityEnd();

  return (int)Unity.TestFailures;
}