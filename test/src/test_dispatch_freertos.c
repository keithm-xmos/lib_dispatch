// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "lib_dispatch/api/dispatch.h"
#include "unity.h"
#include "unity_fixture.h"

TEST_GROUP(dispatch_freertos);

TEST_SETUP(dispatch_freertos) {}

TEST_TEAR_DOWN(dispatch_freertos) {}

TEST(dispatch_freertos, test_create) { TEST_ASSERT_NULL(NULL); }

TEST_GROUP_RUNNER(dispatch_freertos) {
  RUN_TEST_CASE(dispatch_freertos, test_create);
}
}