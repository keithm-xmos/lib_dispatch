// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef DISPATCH_CONFIG_H_
#define DISPATCH_CONFIG_H_

#if BARE_METAL

#include <xcore/assert.h>

#include "debug_print.h"

#define dispatch_assert(A) xassert(A)

#define dispatch_malloc(A) malloc(A)
#define dispatch_free(A) free(A)

#define dispatch_printf(...) debug_printf(__VA_ARGS__)

#elif FREERTOS

#include <xcore/assert.h>

#include "FreeRTOS.h"
#include "rtos_printf.h"

#define dispatch_assert(A) xassert(A)

#define dispatch_malloc(A) pvPortMalloc(A)
#define dispatch_free(A) vPortFree(A)

#define dispatch_printf(...) debug_printf(__VA_ARGS__)

#elif HOST

#include <assert.h>
#include <stdio.h>

#define dispatch_assert(A) assert(A)

#define dispatch_malloc(A) malloc(A)
#define dispatch_free(A) free(A)

#define dispatch_printf(...) printf(__VA_ARGS__)

#endif

#endif  // DISPATCH_CONFIG_H_