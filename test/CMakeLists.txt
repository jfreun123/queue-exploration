# ---- Tests ----

add_executable(queue-exploration_test source/queue-exploration_test.cpp)
target_link_libraries(queue-exploration_test PRIVATE queue-exploration_lib)
target_compile_features(queue-exploration_test PRIVATE cxx_std_23)

add_test(NAME queue-exploration_test COMMAND queue-exploration_test)
