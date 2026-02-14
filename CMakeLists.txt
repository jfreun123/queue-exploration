cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    queue-exploration
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    queue-exploration_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    queue-exploration_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(queue-exploration_lib PUBLIC cxx_std_20)

# ---- Declare executable ----

add_executable(queue-exploration_exe source/main.cpp)
add_executable(queue-exploration::exe ALIAS queue-exploration_exe)

set_property(TARGET queue-exploration_exe PROPERTY OUTPUT_NAME queue-exploration)

target_compile_features(queue-exploration_exe PRIVATE cxx_std_20)

target_link_libraries(queue-exploration_exe PRIVATE queue-exploration_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT queue-exploration_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of queue-exploration"
  )
endif()

include(cmake/dev-mode.cmake)
