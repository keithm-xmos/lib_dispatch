// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef DISPATCH_EVENT_COUNTER_H_
#define DISPATCH_EVENT_COUNTER_H_

#include <stddef.h>

#include "dispatch_config.h"

typedef struct event_counter_struct event_counter_t;

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

event_counter_t *event_counter_create(size_t count);
void event_counter_signal(event_counter_t *counter);
void event_counter_wait(event_counter_t *counter);
void event_counter_delete(event_counter_t *counter);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // DISPATCH_EVENT_COUNTER_H_