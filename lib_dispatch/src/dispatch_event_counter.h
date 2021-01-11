// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef DISPATCH_EVENT_COUNTER_H_
#define DISPATCH_EVENT_COUNTER_H_

#include <stddef.h>

#include "dispatch_config.h"

typedef struct dispatch_event_counter_struct dispatch_event_counter_t;

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

dispatch_event_counter_t *dispatch_event_counter_create(size_t count,
                                                        dispatch_lock_t lock);

void dispatch_event_counter_signal(dispatch_event_counter_t *counter);

void dispatch_event_counter_wait(dispatch_event_counter_t *counter);

void dispatch_event_counter_destroy(dispatch_event_counter_t *counter);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // DISPATCH_EVENT_COUNTER_H_