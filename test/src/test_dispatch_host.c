// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "lib_dispatch/api/dispatch.h"
#include "unity.h"
#include "unity_fixture.h"

TEST_GROUP(dispatch_host);

TEST_SETUP(dispatch_host) {}

TEST_TEAR_DOWN(dispatch_host) {}

TEST(dispatch_host, test_create) { TEST_ASSERT_NULL(NULL); }

TEST_GROUP_RUNNER(dispatch_host) { RUN_TEST_CASE(dispatch_host, test_create); }