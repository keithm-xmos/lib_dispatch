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
  "-DBARE_METAL"
  "-DNDEBUG"
  "-march=xs3a"
  "-Os"
)

add_executable(hello_world)

target_compile_options(hello_world PRIVATE ${BUILD_FLAGS})
target_link_options(hello_world PRIVATE ${BUILD_FLAGS})
set_target_properties(hello_world PROPERTIES OUTPUT_NAME hello_world.xe)

#**********************
# targets
#**********************
include("${CMAKE_CURRENT_SOURCE_DIR}/../../lib_dispatch.cmake")

set(LIB_DISPATCH_SOURCES ${LIB_DISPATCH_METAL_SOURCES})

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
