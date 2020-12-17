cmake_minimum_required(VERSION 3.14)

#**********************
# Options
#**********************
OPTION (XCORE "If enabled, will build the XCORE version." OFF)
OPTION (HOST "If enabled, will build the x86 version." OFF)
OPTION (FREERTOS "If enabled, will build the FreeRTOS version." OFF)

#**********************
# Setup XMOS toolchain
#**********************
if(XCORE OR FREERTOS)
  set(CMAKE_C_COMPILER "xcc")
  set(CMAKE_AR "xmosar")
  set(CMAKE_C_COMPILER_AR "xmosar")

  set(CMAKE_RANLIB "")
  set(CMAKE_C_COMPILER_FORCED TRUE)
  set(CMAKE_ASM_COMPILER_FORCED TRUE)

  set(CMAKE_C_FLAGS "" CACHE STRING "C Compiler Base Flags" FORCE)
  set(CMAKE_EXE_LINKER_FLAGS "" CACHE INTERNAL "" FORCE)

  enable_language(C)
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
if(XCORE OR FREERTOS)
  set(BUILD_FLAGS
    "-target=XCORE-AI-EXPLORER"
    "-fcmdline-buffer-bytes=1024"
    "-mcmodel=large"
    "-Wno-xcore-fptrgroup"
    "-Wno-unknown-pragmas"
    "-report"
    "-DXCORE"
    "-DDEBUG_PRINT_ENABLE=1"
    "-Os"
  )
endif()

add_executable(lib_dispatch_tests)

target_compile_options(lib_dispatch_tests PRIVATE ${BUILD_FLAGS})
target_link_options(lib_dispatch_tests PRIVATE ${BUILD_FLAGS})

if(XCORE OR FREERTOS)
  set_target_properties(lib_dispatch_tests PROPERTIES OUTPUT_NAME lib_dispatch_tests.xe)
endif()

#**********************
# Unity
#**********************

set(UNITY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Unity")

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

#**********************
# targets
#**********************
include("${CMAKE_CURRENT_SOURCE_DIR}/../lib_dispatch.cmake")

if(HOST)
  set(LIB_DISPATCH_SOURCES ${LIB_DISPATCH_HOST_SOURCES})
elseif(FREERTOS)
  set(LIB_DISPATCH_SOURCES ${LIB_DISPATCH_FREERTOS_SOURCES})
else()
  set(LIB_DISPATCH_SOURCES ${LIB_DISPATCH_XCORE_SOURCES})
endif()

target_sources(lib_dispatch_tests
  PRIVATE ${UNITY_SOURCES}
  PRIVATE ${LIB_DISPATCH_SOURCES}
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/test_dispatch_task.c"
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/test_dispatch_xcore.c"
)

target_include_directories(lib_dispatch_tests
  PRIVATE ${UNITY_INCLUDES}
  PRIVATE ${LIB_DISPATCH_INCLUDES}
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

install(TARGETS lib_dispatch_tests DESTINATION ${INSTALL_DIR})
