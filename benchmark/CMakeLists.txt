# ---- Benchmark ----

add_executable(queue_benchmark source/queue_benchmark.cpp)
target_link_libraries(queue_benchmark PRIVATE queue-exploration_lib)
target_compile_features(queue_benchmark PRIVATE cxx_std_23)

set_target_properties(queue_benchmark PROPERTIES
    CXX_CLANG_TIDY "clang-tidy"
)
