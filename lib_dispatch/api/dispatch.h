// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef LIB_DISPATCH_H_
#define LIB_DISPATCH_H_

#if BARE_METAL
#include <xcore/assert.h>

#include "debug_print.h"
#elif FREERTOS
#include <xcore/assert.h>

#include "rtos_printf.h"
#elif HOST
#include <assert.h>
#include <stdio.h>
#define xassert(A) assert(A)
#define debug_printf(...) printf(__VA_ARGS__)
#endif

#include "dispatch_group.h"
#include "dispatch_queue.h"
#include "dispatch_task.h"

#endif  // LIB_DISPATCH_H_