#include "lib.hpp"
#include <gtest/gtest.h>

TEST(LibraryTest, NameIsCorrect) {
  auto const lib = library{};
  EXPECT_EQ(lib.name, "queue-exploration");
}
