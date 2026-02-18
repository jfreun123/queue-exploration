# queue-exploration

Exploring thread-safe queue implementations in C++23. Currently includes a
mutex-based queue (`SlowQueueTS`) with unit tests and a throughput benchmark.

## Queue Implementations

- **SlowQueueTS** — a simple thread-safe queue backed by `std::queue` and a
  single `std::mutex`. Supports `push`, `pop` (returns `std::optional`),
  `empty`, and `size`.

## Prerequisites

- CMake 3.14+
- Clang++ (C++23 support)
- Ninja build system
- clang-tidy
- cppcheck
- clang-format
- Python 3 with `matplotlib` and `numpy` (for plotting benchmark results)

Install on Ubuntu/Debian:
```bash
sudo apt-get install cmake clang ninja-build clang-tidy cppcheck clang-format
pip install matplotlib numpy
```

## Building and Testing

```bash
# Configure (Debug build with static analysis)
cmake --preset=dev

# Build
cmake --build --preset=dev

# Run tests
ctest --preset=dev
```

## Running the Benchmark

The benchmark measures queue throughput (messages/second) with 1 producer and
1–8 consumers over a 2-second window for each configuration.

```bash
# Configure with RelWithDebInfo optimizations
cmake --preset=bench

# Build
cmake --build --preset=bench

# Run the benchmark (outputs CSV to stdout)
./build-bench/benchmark/queue_benchmark

# Save results and plot
./build-bench/benchmark/queue_benchmark > results.csv
python3 benchmark/plot_results.py results.csv results.png
```

## Code Quality

```bash
# Format code
cmake --build build -t format-fix

# Check formatting
cmake --build build -t format-check
```

## Project Structure

```
queue-exploration/
├── source/                        # Library source
│   └── slow_queue.hpp             # SlowQueueTS implementation
├── test/                          # Unit tests (Google Test)
│   └── source/
│       └── slow_queue_test.cpp
├── benchmark/                     # Throughput benchmarks
│   ├── source/
│   │   └── queue_benchmark.cpp
│   └── plot_results.py            # Plot CSV results to PNG
├── cmake/                         # CMake modules
│   ├── dev-mode.cmake
│   ├── gtest.cmake
│   └── lint-targets.cmake
├── CMakeLists.txt                 # Root build file
└── CMakePresets.json              # dev & bench presets
```
