cmake_minimum_required(VERSION 3.14)

# ---- In-source guard ----

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed. Please create a build directory.")
endif()

project(
    queue-exploration
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

# ---- Developer mode ----

option(queue-exploration_DEVELOPER_MODE "Enable developer mode" ON)

# ---- Declare library ----

add_library(
    queue-exploration_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    queue-exploration_lib
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

# ---- Developer mode targets ----

if(queue-exploration_DEVELOPER_MODE)
  include(cmake/dev-mode.cmake)
endif()
