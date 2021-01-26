
if(NOT DEFINED LIB_DISPATCH_DIR)
  set(LIB_DISPATCH_DIR ${CMAKE_CURRENT_LIST_DIR})
endif()

set(LIB_DISPATCH_SOURCES
  "${LIB_DISPATCH_DIR}/lib_dispatch/src/dispatch_task.c"
  "${LIB_DISPATCH_DIR}/lib_dispatch/src/dispatch_group.c"
)

set(LIB_DISPATCH_HOST_SOURCES
  ${LIB_DISPATCH_SOURCES}
  "${LIB_DISPATCH_DIR}/lib_dispatch/src/dispatch_queue_host.cc"
)

set(LIB_DISPATCH_METAL_SOURCES
  ${LIB_DISPATCH_SOURCES}
  "${LIB_DISPATCH_DIR}/lib_dispatch/src/dispatch_queue_metal.c"
  "${LIB_DISPATCH_DIR}/lib_dispatch/src/spinlock.c"
  "${LIB_DISPATCH_DIR}/lib_dispatch/src/spinlock_asm.S"
  "${LIB_DISPATCH_DIR}/lib_dispatch/src/condition_variable_metal.c"
  "${LIB_DISPATCH_DIR}/lib_dispatch/src/event_counter_metal.c"
  "${LIB_DISPATCH_DIR}/lib_dispatch/src/queue_metal.c"
)

set(LIB_DISPATCH_FREERTOS_SOURCES
  ${LIB_DISPATCH_SOURCES}
  "${LIB_DISPATCH_DIR}/lib_dispatch/src/dispatch_queue_rtos.c"
  "${LIB_DISPATCH_DIR}/lib_dispatch/src/event_counter_rtos.c"
)

set(LIB_DISPATCH_INCLUDES
  "${LIB_DISPATCH_DIR}/lib_dispatch/api"
)