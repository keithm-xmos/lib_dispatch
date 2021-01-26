include(CPM.cmake)

CPMAddPackage(
  NAME lib_logging
  GIT_REPOSITORY git@github.com:xmos/lib_logging.git
  GIT_TAG 09c77b06920afc1d3ddc70f377fe92a0bec46e0f
  VERSION v3.0.2
  DOWNLOAD_ONLY YES
  SOURCE_DIR lib_logging
)

CPMAddPackage(
  NAME Unity
  GIT_REPOSITORY git@github.com:ThrowTheSwitch/Unity.git
  GIT_TAG cf949f45ca6d172a177b00da21310607b97bc7a7
  VERSION v2.5.1
  DOWNLOAD_ONLY YES
  SOURCE_DIR Unity
)

if (lib_logging_ADDED)
  # lib_logging has no CMake support, so we create manually create variables

  set(LIB_LOGGING_SOURCES
    PRIVATE "${lib_logging_SOURCE_DIR}/lib_logging/src/debug_printf.c"
  )

  set(LIB_LOGGING_INCLUDES
    PRIVATE "${lib_logging_SOURCE_DIR}/lib_logging/api"
  )
endif()

if (Unity_ADDED)
  # Unity has CMake support but not for xcore, so we create manually create variables
  set(UNITY_SOURCES
    PRIVATE "${Unity_SOURCE_DIR}/src/unity.c"
    PRIVATE "${Unity_SOURCE_DIR}/extras/memory/src/unity_memory.c"
    PRIVATE "${Unity_SOURCE_DIR}/extras/fixture/src/unity_fixture.c"
  )

  set(UNITY_INCLUDES
    PRIVATE "${Unity_SOURCE_DIR}/src"
    PRIVATE "${Unity_SOURCE_DIR}/extras/memory/src"
    PRIVATE "${Unity_SOURCE_DIR}/extras/fixture/src"
  )
endif()
