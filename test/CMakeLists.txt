# ---- Tests ----

add_executable(queue-exploration_test source/queue-exploration_test.cpp)
target_link_libraries(queue-exploration_test PRIVATE queue-exploration_lib GTest::gtest_main)
target_compile_features(queue-exploration_test PRIVATE cxx_std_23)

# Static analysis (only for our test code, not gtest)
set_target_properties(queue-exploration_test PROPERTIES
    CXX_CLANG_TIDY "clang-tidy"
    CXX_CPPCHECK "cppcheck;--inline-suppr"
)

include(GoogleTest)
gtest_discover_tests(queue-exploration_test)
