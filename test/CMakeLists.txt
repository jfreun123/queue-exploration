# ---- Tests ----

add_executable(slow_queue_test source/slow_queue_test.cpp)
target_link_libraries(slow_queue_test PRIVATE queue-exploration_lib GTest::gtest_main)
target_compile_features(slow_queue_test PRIVATE cxx_std_23)

# Static analysis (only for our test code, not gtest)
set_target_properties(slow_queue_test PROPERTIES
    CXX_CLANG_TIDY "clang-tidy"
    CXX_CPPCHECK "cppcheck;--inline-suppr"
)

include(GoogleTest)
gtest_discover_tests(slow_queue_test)
