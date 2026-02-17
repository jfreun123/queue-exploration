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

# Static analysis (only for our code, not dependencies)
set_target_properties(queue-exploration_lib PROPERTIES
    CXX_CLANG_TIDY "clang-tidy"
    CXX_CPPCHECK "cppcheck;--inline-suppr"
)

# ---- Executable ----

add_executable(queue-exploration_exe main.cpp)
add_executable(queue-exploration::exe ALIAS queue-exploration_exe)

set_property(TARGET queue-exploration_exe PROPERTY OUTPUT_NAME queue-exploration)

target_compile_features(queue-exploration_exe PRIVATE cxx_std_23)

# Static analysis (only for our code, not dependencies)
set_target_properties(queue-exploration_exe PROPERTIES
    CXX_CLANG_TIDY "clang-tidy"
    CXX_CPPCHECK "cppcheck;--inline-suppr"
)

target_link_libraries(queue-exploration_exe PRIVATE queue-exploration_lib)
