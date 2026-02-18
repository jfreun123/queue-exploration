#pragma once

#include <stdexcept>

#define EXPECT(cond, msg)                                                      \
  if (!(cond)) [[unlikely]] {                                                  \
    [&]()                                                                      \
        __attribute__((noinline, cold)) { throw std::runtime_error(msg); }();  \
  }
