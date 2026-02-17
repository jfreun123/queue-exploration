# ---- Library ----

add_library(
    queue-exploration_lib OBJECT
    lib.cpp
)

target_include_directories(
    queue-exploration_lib
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(queue-exploration_lib PUBLIC cxx_std_23)

# ---- Executable ----

add_executable(queue-exploration_exe main.cpp)
add_executable(queue-exploration::exe ALIAS queue-exploration_exe)

set_property(TARGET queue-exploration_exe PROPERTY OUTPUT_NAME queue-exploration)

target_compile_features(queue-exploration_exe PRIVATE cxx_std_23)

target_link_libraries(queue-exploration_exe PRIVATE queue-exploration_lib)
