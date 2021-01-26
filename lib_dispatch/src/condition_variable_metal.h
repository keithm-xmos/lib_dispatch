// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef DISPATCH_CONDITION_VARIABLE_H_
#define DISPATCH_CONDITION_VARIABLE_H_

#include <stdbool.h>
#include <xcore/channel.h>
#include <xcore/lock.h>

#include "dispatch_config.h"
#include "spinlock_metal.h"

typedef struct condition_node_struct condition_node_t;
struct condition_node_struct {
  chanend_t cend;
  condition_node_t* next;
};

typedef struct condition_variable_struct condition_variable_t;
struct condition_variable_struct {
  condition_node_t* waiters;  // linked list
  swlock_t swlock;            // spinlock
};

condition_variable_t* condition_variable_create();
bool condition_variable_wait(condition_variable_t* cv, dispatch_lock_t lock,
                             chanend_t dest);
void condition_variable_signal(condition_variable_t* cv, chanend_t source);
void condition_variable_broadcast(condition_variable_t* cv, chanend_t source);
void condition_variable_terminate(condition_variable_t* cv, chanend_t source);
void condition_variable_destroy(condition_variable_t* cv);

#endif  // DISPATCH_CONDITION_VARIABLE_H_