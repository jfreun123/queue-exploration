# queue-exploration

A minimal C++23 project with static analysis and formatting tools.

## Features

- **C++23** standard
- **Clang compiler** with strict warnings
- **Static Analysis**: clang-tidy, cppcheck
- **Code Formatting**: clang-format (LLVM style)
- **Testing**: CTest integration
- **Build System**: CMake with presets

## Building

### Prerequisites

- CMake 3.14+
- Clang++ (C++23 support)
- Ninja build system
- clang-tidy
- cppcheck
- clang-format

Install on Ubuntu/Debian:
```bash
sudo apt-get install cmake clang ninja-build clang-tidy cppcheck clang-format
```

### Build Commands

```bash
# Configure
cmake --preset=dev

# Build
cmake --build --preset=dev

# Test
ctest --preset=dev

# Format code
cmake --build build -t format-fix

# Check formatting
cmake --build build -t format-check
```

## Project Structure

```
queue-exploration/
├── source/           # Source code
│   ├── slow_queue.hpp
│   └── main.cpp
├── test/             # Tests
│   └── source/
│       └── slow_queue_test.cpp
├── cmake/            # CMake modules
│   ├── dev-mode.cmake
│   └── lint-targets.cmake
└── CMakeLists.txt    # Main build file
```

## License

[Choose a license](https://choosealicense.com/licenses/)
