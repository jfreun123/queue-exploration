# ---- Tests ----

include(CTest)
if(BUILD_TESTING)
  include(cmake/gtest.cmake)
  add_subdirectory(test)
endif()

# ---- Linting (clang-format) ----

include(cmake/lint-targets.cmake)
