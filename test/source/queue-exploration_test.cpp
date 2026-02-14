#include "lib.hpp"

auto main() -> int
{
  auto const lib = library {};

  return lib.name == "queue-exploration" ? 0 : 1;
}
