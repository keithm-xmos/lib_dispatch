cmake_minimum_required(VERSION 3.14)

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

#**********************
# Setup project
#**********************
project(lib_dispatch_tests VERSION 1.0.0)

enable_language(C)

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
  "-fcmdline-buffer-bytes=1024"
  "-mcmodel=large"
  "-Wno-xcore-fptrgroup"
  "-Wno-unknown-pragmas"
  "-report"
  "-DXCORE"
  "-Os"
)

add_executable(lib_dispatch_tests)

target_compile_options(lib_dispatch_tests PRIVATE ${BUILD_FLAGS})
target_link_options(lib_dispatch_tests PRIVATE ${BUILD_FLAGS})

set_target_properties(lib_dispatch_tests PROPERTIES OUTPUT_NAME lib_dispatch_tests.xe)

#**********************
# Unity
#**********************

set(UNITY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/Unity/")

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

target_sources(lib_dispatch_tests
  PRIVATE ${UNITY_SOURCES}
  PRIVATE ${LIB_DISPATCH_SOURCES}
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/test_task.c"
)

target_include_directories(lib_dispatch_tests
  PRIVATE ${UNITY_INCLUDES}
  PRIVATE ${LIB_DISPATCH_INCLUDES}
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

install(TARGETS lib_dispatch_tests DESTINATION ${INSTALL_DIR})
