# ---- Tests ----

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(test)
endif()

# ---- Linting (clang-format) ----

include(cmake/lint-targets.cmake)
