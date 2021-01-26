cmake_minimum_required(VERSION 3.14)

#**********************
# Options
#**********************
OPTION (BARE_METAL "If enabled, will build the bare-metal version." OFF)
OPTION (HOST "If enabled, will build the x86 version." OFF)
OPTION (FREERTOS "If enabled, will build the FreeRTOS version." OFF)

#**********************
# Setup XMOS toolchain
#**********************
if(BARE_METAL OR FREERTOS)
  set(CMAKE_C_COMPILER "xcc")
  set(CMAKE_AR "xmosar")
  set(CMAKE_C_COMPILER_AR "xmosar")

  set(CMAKE_RANLIB "")
  set(CMAKE_C_COMPILER_FORCED TRUE)
  set(CMAKE_ASM_COMPILER_FORCED TRUE)

  set(CMAKE_C_FLAGS "" CACHE STRING "C Compiler Base Flags" FORCE)
  set(CMAKE_EXE_LINKER_FLAGS "" CACHE INTERNAL "" FORCE)

  enable_language(C ASM)
else()
  enable_language(C CXX)
endif()

#**********************
# Setup project
#**********************
project(lib_dispatch_tests VERSION 1.0.0)


#**********************
# Disable in-source build.
#**********************
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source build is not allowed! Please specify a build folder.\n\tex:cmake -B build")
endif()

#**********************
# install
#**********************
set(INSTALL_DIR "${PROJECT_SOURCE_DIR}/bin")

#**********************
# Build flags
#**********************
if(BARE_METAL OR FREERTOS)
  set(BUILD_FLAGS
    "-target=XCORE-AI-EXPLORER"
    "-fcmdline-buffer-bytes=1024"
    "-mcmodel=large"
    "-fxscope"
    "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope"
    "-Wno-xcore-fptrgroup"
    "-Wno-unknown-pragmas"
    "-report"
    "-DDEBUG_PRINT_ENABLE=1"
    "-march=xs3a"
    "-Os"
  )
endif()

if(BARE_METAL)
  set(BUILD_FLAGS
    ${BUILD_FLAGS} 
    "-DBARE_METAL"
  )
elseif(FREERTOS)
  set(BUILD_FLAGS
    ${BUILD_FLAGS} 
    "-DFREERTOS"
  )
elseif(HOST)
  set(BUILD_FLAGS
    "-DHOST"
  )
endif()

add_executable(lib_dispatch_tests)

target_compile_options(lib_dispatch_tests PRIVATE ${BUILD_FLAGS})
target_link_options(lib_dispatch_tests PRIVATE ${BUILD_FLAGS})

if(BARE_METAL OR FREERTOS)
  set_target_properties(lib_dispatch_tests PROPERTIES OUTPUT_NAME lib_dispatch_tests.xe)
else()
  target_link_libraries(lib_dispatch_tests stdc++ m pthread)
endif()

#**********************
# Unity
#**********************

set(UNITY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Unity")
set(LIB_LOGGING_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib_logging")
set(FREERTOS_DIR $ENV{XMOS_AIOT_SDK_PATH}/modules/rtos/FreeRTOS/FreeRTOS-Plus/FreeRTOS-SMP/FreeRTOS)
set(RTOS_SUPPORT_DIR $ENV{XMOS_AIOT_SDK_PATH}/modules/lib_rtos_support)

#NOTE This can go away when lib_xassert is no longer part of SDK
set(XASSERT_DIR $ENV{XMOS_AIOT_SDK_PATH}/modules/lib_xassert/lib_xassert/lib_xassert)

set(UNITY_SOURCES
  PRIVATE "${UNITY_DIR}/src/unity.c"
  PRIVATE "${UNITY_DIR}/extras/memory/src/unity_memory.c"
  PRIVATE "${UNITY_DIR}/extras/fixture/src/unity_fixture.c"
)

set(UNITY_INCLUDES
  PRIVATE "${UNITY_DIR}/src"
  PRIVATE "${UNITY_DIR}/extras/memory/src"
  PRIVATE "${UNITY_DIR}/extras/fixture/src"
)

set(LIB_LOGGING_SOURCES
  PRIVATE "${LIB_LOGGING_DIR}/lib_logging/src/debug_printf.c"
)

set(LIB_LOGGING_INCLUDES
  PRIVATE "${LIB_LOGGING_DIR}/lib_logging/api"
)

#**********************
# targets
#**********************
include("${CMAKE_CURRENT_SOURCE_DIR}/../lib_dispatch.cmake")

if(HOST)
  set(LIB_DISPATCH_SOURCES ${LIB_DISPATCH_HOST_SOURCES})
elseif(FREERTOS)
  set(LIB_DISPATCH_SOURCES ${LIB_DISPATCH_FREERTOS_SOURCES})
  set(TEST_DISPATCH_SOURCES
    ${LIB_LOGGING_SOURCES}
    "${FREERTOS_DIR}/Source/croutine.c"
    "${FREERTOS_DIR}/Source/event_groups.c"
    "${FREERTOS_DIR}/Source/list.c"
    "${FREERTOS_DIR}/Source/queue.c"
    "${FREERTOS_DIR}/Source/stream_buffer.c"
    "${FREERTOS_DIR}/Source/tasks.c"
    "${FREERTOS_DIR}/Source/timers.c"
    "${FREERTOS_DIR}/Source/portable/XCC/XCOREAI/port.c"
    "${FREERTOS_DIR}/Source/portable/XCC/XCOREAI/port.xc"
    "${FREERTOS_DIR}/Source/portable/MemMang/heap_4.c"
    "${FREERTOS_DIR}/Source/portable/XCC/XCOREAI/portasm.S"
    "${RTOS_SUPPORT_DIR}/src/rtos_cores.c"
    "${RTOS_SUPPORT_DIR}/src/rtos_irq.c"
    "${RTOS_SUPPORT_DIR}/src/rtos_locks.c"
    "${RTOS_SUPPORT_DIR}/src/rtos_printf.c"
    "${RTOS_SUPPORT_DIR}/src/rtos_time.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/test_dispatch_rtos.c"
  )

  set_source_files_properties("${FREERTOS_DIR}/Source/portable/XCC/XCOREAI/port.xc" PROPERTIES LANGUAGE C)

  set(TEST_DISPATCH_INCLUDES 
    #NOTE This can go away when lib_xassert is no longer part of SDK
    "${XASSERT_DIR}/api"
    ${LIB_LOGGING_INCLUDES}
    "${FREERTOS_DIR}/Source/include"
    "${FREERTOS_DIR}/Source/portable/XCC/XCOREAI"
    "${RTOS_SUPPORT_DIR}/api"
    "${RTOS_SUPPORT_DIR}/src"
  )
else()
  set(LIB_DISPATCH_SOURCES ${LIB_DISPATCH_METAL_SOURCES})
  set(TEST_DISPATCH_SOURCES 
    ${LIB_LOGGING_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/test_queue_metal.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/test_dispatch_metal.c"
  )
  set(TEST_DISPATCH_INCLUDES 
    ${LIB_LOGGING_INCLUDES}
  )
endif()

target_sources(lib_dispatch_tests
  PRIVATE ${UNITY_SOURCES}
  PRIVATE ${LIB_DISPATCH_SOURCES}
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/test_dispatch_task.c"
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/test_dispatch_group.c"
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/test_dispatch_queue.c"
  PRIVATE ${TEST_DISPATCH_SOURCES}
)

target_include_directories(lib_dispatch_tests
  PRIVATE ${UNITY_INCLUDES}
  PRIVATE ${LIB_DISPATCH_INCLUDES}
  PRIVATE "${LIB_DISPATCH_DIR}/lib_dispatch/src"
  PRIVATE ${TEST_DISPATCH_INCLUDES}
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

install(TARGETS lib_dispatch_tests DESTINATION ${INSTALL_DIR})
