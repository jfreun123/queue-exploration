# ---- Library ----

add_library(queue-exploration_lib INTERFACE)

target_include_directories(
    queue-exploration_lib
    INTERFACE
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(queue-exploration_lib INTERFACE cxx_std_23)