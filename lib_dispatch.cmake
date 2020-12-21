
if(NOT DEFINED LIB_DISPATCH_DIR)
  set(LIB_DISPATCH_DIR ${CMAKE_CURRENT_LIST_DIR})
endif()

set(LIB_DISPATCH_SOURCES
  "${LIB_DISPATCH_DIR}/lib_dispatch/src/dispatch_task.c"
  "${LIB_DISPATCH_DIR}/lib_dispatch/src/dispatch_group.c"
  "${LIB_DISPATCH_DIR}/lib_dispatch/src/dispatch_queue.c"
)

set(LIB_DISPATCH_HOST_SOURCES
  ${LIB_DISPATCH_SOURCES}
  "${LIB_DISPATCH_DIR}/lib_dispatch/src/dispatch_host.cc"
)

set(LIB_DISPATCH_XCORE_SOURCES
  ${LIB_DISPATCH_SOURCES}
  "${LIB_DISPATCH_DIR}/lib_dispatch/src/dispatch_xcore.c"
  "${LIB_DISPATCH_DIR}/lib_logging/lib_logging/src/debug_printf.c"
)

set(LIB_DISPATCH_FREERTOS_SOURCES
  ${LIB_DISPATCH_SOURCES}
  "${LIB_DISPATCH_DIR}/lib_dispatch/src/dispatch_freertos.c"
  "${LIB_DISPATCH_DIR}/lib_logging/lib_logging/src/debug_printf.c"
)

set(LIB_DISPATCH_INCLUDES
  "${LIB_DISPATCH_DIR}"
  "${LIB_DISPATCH_DIR}/lib_logging/lib_logging/api"
)