#include "fast_queue.hpp"
#include <array>
#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

namespace {

constexpr uint64_t kQueueSize = uint64_t{8} * 1024; // 8 KB
constexpr size_t kReadBufSize = 64;

// Allocate a FastQueue with backing buffer via placement-compatible aligned
// storage
struct QueueStorage {
  alignas(CustomQueues::CACHE_LINE_SIZE)
      std::array<uint8_t, sizeof(CustomQueues::FastQueue) + kQueueSize> raw{};

  CustomQueues::FastQueue *get() {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<CustomQueues::FastQueue *>(raw.data());
  }
};

} // namespace

TEST(FastQueueTest, WriteAndReadSingleMessage) {
  QueueStorage storage;
  auto *queue = storage.get();
  new (queue) CustomQueues::FastQueue{};

  CustomQueues::QProducer producer(queue);
  CustomQueues::QConsumer consumer(queue, kQueueSize);

  const std::array<std::byte, 4> msg = {std::byte{0xDE}, std::byte{0xAD},
                                        std::byte{0xBE}, std::byte{0xEF}};
  producer.Write(msg);

  std::array<std::byte, kReadBufSize> readBuf{};
  int32_t bytesRead = consumer.TryRead(readBuf);

  ASSERT_EQ(bytesRead, 4);
  EXPECT_EQ(std::memcmp(readBuf.data(), msg.data(), 4), 0);
}

TEST(FastQueueTest, TryReadOnEmptyReturnsZero) {
  QueueStorage storage;
  auto *queue = storage.get();
  new (queue) CustomQueues::FastQueue{};

  CustomQueues::QConsumer consumer(queue, kQueueSize);

  std::array<std::byte, kReadBufSize> readBuf{};
  EXPECT_EQ(consumer.TryRead(readBuf), 0);
}

TEST(FastQueueTest, FifoOrdering) {
  QueueStorage storage;
  auto *queue = storage.get();
  new (queue) CustomQueues::FastQueue{};

  CustomQueues::QProducer producer(queue);
  CustomQueues::QConsumer consumer(queue, kQueueSize);

  constexpr int kNumMessages = 10;
  for (int i = 0; i < kNumMessages; ++i) {
    auto val = static_cast<uint32_t>(i);
    auto bytes = std::as_bytes(std::span(&val, 1));
    producer.Write(bytes);
  }

  for (int i = 0; i < kNumMessages; ++i) {
    std::array<std::byte, kReadBufSize> readBuf{};
    int32_t bytesRead = consumer.TryRead(readBuf);
    ASSERT_EQ(bytesRead, static_cast<int32_t>(sizeof(uint32_t)));

    uint32_t val = 0;
    std::memcpy(&val, readBuf.data(), sizeof(uint32_t));
    EXPECT_EQ(val, static_cast<uint32_t>(i));
  }

  // Nothing left
  std::array<std::byte, kReadBufSize> readBuf{};
  EXPECT_EQ(consumer.TryRead(readBuf), 0);
}

TEST(FastQueueTest, VariableLengthMessages) {
  QueueStorage storage;
  auto *queue = storage.get();
  new (queue) CustomQueues::FastQueue{};

  CustomQueues::QProducer producer(queue);
  CustomQueues::QConsumer consumer(queue, kQueueSize);

  // Write messages of different sizes
  const std::array<std::byte, 1> small = {std::byte{0x01}};
  const std::array<std::byte, 8> medium = {
      std::byte{0x10}, std::byte{0x20}, std::byte{0x30}, std::byte{0x40},
      std::byte{0x50}, std::byte{0x60}, std::byte{0x70}, std::byte{0x80}};

  producer.Write(small);
  producer.Write(medium);

  std::array<std::byte, kReadBufSize> readBuf{};

  int32_t bytesRead = consumer.TryRead(readBuf);
  ASSERT_EQ(bytesRead, 1);
  EXPECT_EQ(readBuf[0], std::byte{0x01});

  bytesRead = consumer.TryRead(readBuf);
  ASSERT_EQ(bytesRead, 8);
  EXPECT_EQ(std::memcmp(readBuf.data(), medium.data(), 8), 0);
}

TEST(FastQueueTest, ConcurrentProducerConsumer) {
  QueueStorage storage;
  auto *queue = storage.get();
  new (queue) CustomQueues::FastQueue{};

  CustomQueues::QProducer producer(queue);
  CustomQueues::QConsumer consumer(queue, kQueueSize);

  constexpr int kNumItems = 500;
  std::vector<uint32_t> received;
  received.reserve(kNumItems);

  std::thread producerThread([&producer]() {
    for (uint32_t idx = 0; idx < kNumItems; ++idx) {
      auto bytes = std::as_bytes(std::span(&idx, 1));
      producer.Write(bytes);
    }
  });

  std::thread consumerThread([&consumer, &received]() {
    while (static_cast<int>(received.size()) < kNumItems) {
      std::array<std::byte, kReadBufSize> readBuf{};
      int32_t bytesRead = consumer.TryRead(readBuf);
      if (bytesRead > 0) {
        uint32_t val = 0;
        std::memcpy(&val, readBuf.data(), sizeof(uint32_t));
        received.push_back(val);
      } else {
        std::this_thread::yield();
      }
    }
  });

  producerThread.join();
  consumerThread.join();

  ASSERT_EQ(static_cast<int>(received.size()), kNumItems);
  for (uint32_t idx = 0; idx < kNumItems; ++idx) {
    EXPECT_EQ(received[idx], idx);
  }
}
