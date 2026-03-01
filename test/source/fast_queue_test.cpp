#include "fast_queue.hpp"
#include "queue_test_suite.hpp"

#include <array>
#include <cstring>
#include <gtest/gtest.h>
#include <span>

namespace {

constexpr uint64_t kQueueSize = uint64_t{8} * 1024; // 8 KB

struct QueueStorage {
  alignas(CustomQueues::CACHE_LINE_SIZE)
      std::array<uint8_t, sizeof(CustomQueues::FastQueue) + kQueueSize> raw{};

  CustomQueues::FastQueue *get() {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<CustomQueues::FastQueue *>(raw.data());
  }
};

struct FastQueueEnv {
  QueueStorage storage;
  CustomQueues::FastQueue *qPtr;
  CustomQueues::QProducer producer;
  CustomQueues::QConsumer consumer;

  FastQueueEnv()
      : qPtr(new (storage.get()) CustomQueues::FastQueue{})
      , producer(qPtr)
      , consumer(qPtr, kQueueSize) {}

  void push(int val) {
    producer.Write(std::as_bytes(std::span<const int>(&val, 1)));
  }

  std::optional<int> tryPop() {
    std::array<std::byte, sizeof(int)> buf{};
    int32_t bytesRead = consumer.TryRead(buf);
    if (bytesRead == 0) {
      return std::nullopt;
    }
    int val = 0;
    std::memcpy(&val, buf.data(), sizeof(int));
    return val;
  }
};

} // namespace

INSTANTIATE_TYPED_TEST_SUITE_P(FastQueue, QueueTest, FastQueueEnv);

// Fast-queue-specific: variable-length byte messages have no typed equivalent.
TEST(FastQueueTest, VariableLengthMessages) {
  QueueStorage storage;
  auto *queue = storage.get();
  new (queue) CustomQueues::FastQueue{};

  CustomQueues::QProducer producer(queue);
  CustomQueues::QConsumer consumer(queue, kQueueSize);

  const std::array<std::byte, 1> small = {std::byte{0x01}};
  const std::array<std::byte, 8> medium = {
      std::byte{0x10}, std::byte{0x20}, std::byte{0x30}, std::byte{0x40},
      std::byte{0x50}, std::byte{0x60}, std::byte{0x70}, std::byte{0x80}};

  producer.Write(small);
  producer.Write(medium);

  constexpr size_t kReadBufSize = 64;
  std::array<std::byte, kReadBufSize> readBuf{};

  int32_t bytesRead = consumer.TryRead(readBuf);
  ASSERT_EQ(bytesRead, 1);
  EXPECT_EQ(readBuf[0], std::byte{0x01});

  bytesRead = consumer.TryRead(readBuf);
  ASSERT_EQ(bytesRead, 8);
  EXPECT_EQ(std::memcmp(readBuf.data(), medium.data(), 8), 0);
}
