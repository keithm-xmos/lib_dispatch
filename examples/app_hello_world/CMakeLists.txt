cmake_minimum_required(VERSION 3.14)

#**********************
# Options
#**********************
OPTION (BARE_METAL "If enabled, will build the bare-metal version." OFF)
OPTION (FREERTOS "If enabled, will build the FreeRTOS version." OFF)

#**********************
# Setup XMOS toolchain
#**********************
set(CMAKE_C_COMPILER "xcc")
set(CMAKE_AR "xmosar")
set(CMAKE_C_COMPILER_AR "xmosar")

set(CMAKE_RANLIB "")
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_ASM_COMPILER_FORCED TRUE)

set(CMAKE_C_FLAGS "" CACHE STRING "C Compiler Base Flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "" CACHE INTERNAL "" FORCE)

enable_language(C ASM)

#**********************
# Setup project
#**********************
project(hello_world VERSION 1.0.0)

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
set(BUILD_FLAGS
  "-target=XCORE-AI-EXPLORER"
  "-mcmodel=large"
  "-fxscope"
  "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope"
  "-Wno-xcore-fptrgroup"
  "-Wno-unknown-pragmas"
  "-report"
  "-DDEBUG_PRINT_ENABLE=0"
  "-march=xs3a"
  "-Os"
)

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
endif()

add_executable(hello_world)

target_compile_options(hello_world PRIVATE ${BUILD_FLAGS})
target_link_options(hello_world PRIVATE ${BUILD_FLAGS})
set_target_properties(hello_world PROPERTIES OUTPUT_NAME hello_world.xe)

#**********************
# External paths
#**********************

set(FREERTOS_DIR $ENV{XMOS_AIOT_SDK_PATH}/modules/rtos/FreeRTOS/FreeRTOS-Plus/FreeRTOS-SMP/FreeRTOS)
set(RTOS_SUPPORT_DIR $ENV{XMOS_AIOT_SDK_PATH}/modules/lib_rtos_support)
set(XASSERT_DIR $ENV{XMOS_AIOT_SDK_PATH}/modules/lib_xassert/lib_xassert/lib_xassert)

#**********************
# targets
#**********************
include("${CMAKE_CURRENT_SOURCE_DIR}/../../lib_dispatch.cmake")

if(FREERTOS)
  set(LIB_DISPATCH_SOURCES ${LIB_DISPATCH_FREERTOS_SOURCES})
  set(HELLO_WORLD_SOURCES 
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
  )

  set_source_files_properties("${FREERTOS_DIR}/Source/portable/XCC/XCOREAI/port.xc" PROPERTIES LANGUAGE C)

  set(HELLO_WORLD_INCLUDES 
    "${XASSERT_DIR}/api"
    "${FREERTOS_DIR}/Source/include"
    "${FREERTOS_DIR}/Source/portable/XCC/XCOREAI"
    "${RTOS_SUPPORT_DIR}/api"
    "${RTOS_SUPPORT_DIR}/src"
  )
else()
  set(LIB_DISPATCH_SOURCES ${LIB_DISPATCH_METAL_SOURCES})
endif()

target_sources(hello_world
  PRIVATE ${LIB_DISPATCH_SOURCES}
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
  PRIVATE ${HELLO_WORLD_SOURCES}
)

target_include_directories(hello_world
  PRIVATE ${LIB_DISPATCH_INCLUDES}
  PRIVATE ${HELLO_WORLD_INCLUDES}
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

install(TARGETS hello_world DESTINATION ${INSTALL_DIR})
