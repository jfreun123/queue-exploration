# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(queue-explorationTests LANGUAGES CXX)

# ---- Tests ----

add_executable(queue-exploration_test source/queue-exploration_test.cpp)
target_link_libraries(queue-exploration_test PRIVATE queue-exploration_lib)
target_compile_features(queue-exploration_test PRIVATE cxx_std_20)

add_test(NAME queue-exploration_test COMMAND queue-exploration_test)

# ---- End-of-file commands ----

add_folders(Test)
