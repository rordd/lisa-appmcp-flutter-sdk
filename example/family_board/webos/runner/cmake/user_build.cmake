cmake_minimum_required(VERSION 3.10)

# source files for user apps.
set(USER_APP_SRCS
  runner/flutter_application_description.cc
  runner/flutter_embedder_loader.cc
  runner/flutter_launch_params.cc
  runner/flutter_window.cc
  runner/logger.cc
  runner/main.cc
  runner/settings.cc
)

# header files for user apps.
set(USER_APP_INCLUDE_DIRS
  ## Public APIs for developers (Don't edit!).
  flutter/ephemeral/include
  ## header file include path for user apps.
  ${CMAKE_CURRENT_BINARY_DIR}/runner
)

set(THIRD_PARTY_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/runner/third_party")
set(RAPIDJSON_INCLUDE_DIRS "${THIRD_PARTY_DIRS}/rapidjson/include/")

# link libraries for user apps.
set(USER_APP_LIBRARIES)

IF(NOT DEFINED SERVICE_NAME)
    set(SERVICE_NAME com.webos.flutter.embedder)
endif(NOT DEFINED SERVICE_NAME)
add_definitions(-DWEBOS_SERVICE_NAME="${SERVICE_NAME}")


