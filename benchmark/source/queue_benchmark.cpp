// queues
#include "expect.hpp"
#include "fast_queue.hpp"
#include "slow_queue.hpp"

// std
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <span>
#include <string_view>
#include <thread>
#include <vector>

namespace {

constexpr int kMaxReaders = 8;
constexpr int kDurationMs = 2000;
constexpr std::uint64_t kMicrosecondsPerSecond = 1'000'000ULL;

template <typename Env> std::uint64_t run(int num_readers) {
  Env env;
  std::atomic<bool> running{true};
  std::vector<std::uint64_t> counts(static_cast<size_t>(num_readers), 0U);

  std::thread producer_thread([&env, &running]() {
    while (running.load(std::memory_order_relaxed)) {
      env.push();
    }
  });

  std::vector<std::thread> consumers;
  consumers.reserve(static_cast<size_t>(num_readers));
  for (int reader_idx = 0; reader_idx < num_readers; ++reader_idx) {
    consumers.emplace_back([&env, &running, &counts, reader_idx]() {
      auto consumer = env.make_consumer();
      std::uint64_t local_count = 0;
      while (running.load(std::memory_order_relaxed)) {
        if (consumer.try_pop()) {
          ++local_count;
        } else {
          std::this_thread::yield();
        }
      }
      while (consumer.try_pop()) {
        ++local_count;
      }
      counts[static_cast<size_t>(reader_idx)] = local_count;
    });
  }

  auto start = std::chrono::steady_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(kDurationMs));
  running.store(false, std::memory_order_relaxed);

  producer_thread.join();
  for (auto &consumer : consumers) {
    consumer.join();
  }

  env.verify();

  auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::steady_clock::now() - start)
                        .count();

  std::uint64_t total = 0;
  for (auto count : counts) {
    total += count;
  }
  EXPECT(total > 0, "no messages were consumed");
  return total * kMicrosecondsPerSecond /
         static_cast<std::uint64_t>(elapsed_us);
}

struct SlowBenchEnv {
  CustomQueues::SlowQueueTS<int> queue;
  CustomQueues::SlowProducer<int> producer{&queue};

  void push() { producer.Push(0); }

  struct Consumer {
    CustomQueues::SlowConsumer<int> inner;
    bool try_pop() { return inner.TryPop().has_value(); }
  };

  Consumer make_consumer() {
    return Consumer{CustomQueues::SlowConsumer<int>{&queue}};
  }

  void verify() const {
    EXPECT(queue.empty(),
           "SlowQueue not empty after benchmark - messages were dropped");
  }
};

struct FastBenchEnv {
  static constexpr std::uint64_t kBufSize = 64ULL * 1024 * 1024; // 64 MB ring buffer
  static constexpr std::size_t kAllocSize =
      (sizeof(CustomQueues::FastQueue) + kBufSize +
       CustomQueues::CACHE_LINE_SIZE - 1) /
      CustomQueues::CACHE_LINE_SIZE * CustomQueues::CACHE_LINE_SIZE;

  // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
  std::uint8_t *raw{static_cast<std::uint8_t *>(
      std::aligned_alloc(CustomQueues::CACHE_LINE_SIZE, kAllocSize))};
  CustomQueues::FastQueue *qPtr{new (raw) CustomQueues::FastQueue{}};
  CustomQueues::QProducer producer{qPtr, kBufSize};
  const std::array<std::byte, sizeof(int)> payload{};
  mutable std::atomic<uint64_t> sharedConsumerPos{0};

  FastBenchEnv() { EXPECT(raw != nullptr, "aligned_alloc failed"); }
  FastBenchEnv(const FastBenchEnv &) = delete;
  FastBenchEnv(FastBenchEnv &&) = delete;
  FastBenchEnv &operator=(const FastBenchEnv &) = delete;
  FastBenchEnv &operator=(FastBenchEnv &&) = delete;
  // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
  ~FastBenchEnv() { std::free(raw); }

  void push() { producer.Write(payload); }

  struct Consumer {
    explicit Consumer(CustomQueues::QConsumerShared src) : inner(src) {}
    CustomQueues::QConsumerShared inner;
    std::array<std::byte, sizeof(int)> buf{};
    bool try_pop() { return inner.TryRead(buf) > 0; }
  };

  void verify() const {
    auto readCounter = qPtr->mReadCounter.load(std::memory_order_acquire);
    auto writeCounter = qPtr->mWriteCounter.load(std::memory_order_acquire);
    auto consumerPos = sharedConsumerPos.load(std::memory_order_acquire);
    EXPECT(readCounter == writeCounter,
           "FastQueue read/write counters out of sync after benchmark");
    EXPECT(consumerPos == readCounter,
           "FastQueue: not all messages consumed after benchmark");
  }

  [[nodiscard]] Consumer make_consumer() const {
    return Consumer{CustomQueues::QConsumerShared{qPtr, kBufSize, &sharedConsumerPos}};
  }
};

struct BenchEntry {
  std::string_view name;
  std::function<std::uint64_t(int)> bench;
};

// Add new queue implementations here.
const std::vector<BenchEntry> kQueues = {
    // {"SlowQueueTS", run<SlowBenchEnv>},
    {"FastQueue", run<FastBenchEnv>},
};

} // namespace

int main() {
  std::cout << "queue_name,num_readers,messages_per_second\n";
  for (const auto &entry : kQueues) {
    for (int num_readers = 1; num_readers <= kMaxReaders; ++num_readers) {
      std::uint64_t msgs_per_sec = entry.bench(num_readers);
      std::cout << entry.name << "," << num_readers << "," << msgs_per_sec
                << "\n";
    }
  }
  return 0;
}
