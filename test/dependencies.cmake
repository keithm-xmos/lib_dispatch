include(FetchContent)

FetchContent_Declare(
  lib_logging
  GIT_REPOSITORY https://github.com/xmos/lib_logging.git
  GIT_TAG        09c77b06920afc1d3ddc70f377fe92a0bec46e0f
  GIT_SHALLOW    TRUE
  SOURCE_DIR     lib_logging
)

FetchContent_GetProperties(lib_logging)
if (NOT lib_logging_POPULATED)
  FetchContent_Populate(lib_logging)
  set(LIB_LOGGING_SOURCES
    PRIVATE "${lib_logging_SOURCE_DIR}/lib_logging/src/debug_printf.c"
  )
  set(LIB_LOGGING_INCLUDES
    PRIVATE "${lib_logging_SOURCE_DIR}/lib_logging/api"
  )
endif ()

FetchContent_Declare(
  unity
  GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
  GIT_TAG        cf949f45ca6d172a177b00da21310607b97bc7a7
  GIT_SHALLOW    TRUE
  SOURCE_DIR     unity
)

FetchContent_GetProperties(unity)
if (NOT unity_POPULATED)
  FetchContent_Populate(unity)
  # Unity has CMake support but not for xcore, so we create manually create variables
  set(UNITY_SOURCES
    PRIVATE "${unity_SOURCE_DIR}/src/unity.c"
    PRIVATE "${unity_SOURCE_DIR}/extras/memory/src/unity_memory.c"
    PRIVATE "${unity_SOURCE_DIR}/extras/fixture/src/unity_fixture.c"
  )
  set(UNITY_INCLUDES
    PRIVATE "${unity_SOURCE_DIR}/src"
    PRIVATE "${unity_SOURCE_DIR}/extras/memory/src"
    PRIVATE "${unity_SOURCE_DIR}/extras/fixture/src"
  )
endif ()
