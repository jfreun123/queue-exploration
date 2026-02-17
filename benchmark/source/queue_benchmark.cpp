#include "slow_queue.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

namespace {

constexpr int kMaxReaders = 8;
constexpr int kDurationMs = 2000;
constexpr std::uint64_t kMicrosecondsPerSecond = 1'000'000ULL;
constexpr const char *kQueueName = "SlowQueueTS";

std::uint64_t run(int num_readers) {
  CustomQueues::SlowQueueTS<int> queue;
  std::atomic<bool> running{true};
  std::vector<std::uint64_t> counts(static_cast<size_t>(num_readers), 0U);

  std::thread producer([&queue, &running]() {
    while (running.load(std::memory_order_relaxed)) {
      queue.push(0);
    }
  });

  std::vector<std::thread> consumers;
  consumers.reserve(static_cast<size_t>(num_readers));
  for (int reader_idx = 0; reader_idx < num_readers; ++reader_idx) {
    consumers.emplace_back([&queue, &running, &counts, reader_idx]() {
      std::uint64_t local_count = 0;
      while (running.load(std::memory_order_relaxed)) {
        if (queue.pop().has_value()) {
          ++local_count;
        } else {
          std::this_thread::yield();
        }
      }
      while (queue.pop().has_value()) {
        ++local_count;
      }
      counts[static_cast<size_t>(reader_idx)] = local_count;
    });
  }

  auto start = std::chrono::steady_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(kDurationMs));
  running.store(false, std::memory_order_relaxed);

  producer.join();
  for (auto &consumer : consumers) {
    consumer.join();
  }

  auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::steady_clock::now() - start)
                        .count();

  std::uint64_t total = 0;
  for (auto count : counts) {
    total += count;
  }
  return total * kMicrosecondsPerSecond /
         static_cast<std::uint64_t>(elapsed_us);
}

} // namespace

int main() {
  std::cout << "queue_name,num_readers,messages_per_second\n";
  for (int num_readers = 1; num_readers <= kMaxReaders; ++num_readers) {
    std::uint64_t msgs_per_sec = run(num_readers);
    std::cout << kQueueName << "," << num_readers << "," << msgs_per_sec
              << "\n";
  }
  return 0;
}
