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
      , producer(qPtr, kQueueSize)
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

  CustomQueues::QProducer producer(queue, kQueueSize);
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

// 5-byte messages: alignedPayloadSize = align4(4+5) = 12 bytes.
// With a 32-byte ring buffer, two messages occupy offsets [0,12) and [12,24).
// The third message (12 bytes) doesn't fit in the 8 remaining bytes, so the
// producer writes a padding sentinel at offset 24 and wraps to offset 0.
TEST(FastQueueTest, RingBufferWrap) {
  constexpr uint64_t kSmallQueueSize = 32;

  alignas(CustomQueues::CACHE_LINE_SIZE)
      std::array<uint8_t, sizeof(CustomQueues::FastQueue) + kSmallQueueSize>
          raw{};
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto *queue = reinterpret_cast<CustomQueues::FastQueue *>(raw.data());
  new (queue) CustomQueues::FastQueue{};

  CustomQueues::QProducer producer(queue, kSmallQueueSize);
  CustomQueues::QConsumer consumer(queue, kSmallQueueSize);

  const std::array<std::byte, 5> msg1 = {std::byte{0x01}, std::byte{0x02},
                                         std::byte{0x03}, std::byte{0x04},
                                         std::byte{0x05}};
  const std::array<std::byte, 5> msg2 = {std::byte{0x11}, std::byte{0x12},
                                         std::byte{0x13}, std::byte{0x14},
                                         std::byte{0x15}};
  const std::array<std::byte, 5> msg3 = {std::byte{0x21}, std::byte{0x22},
                                         std::byte{0x23}, std::byte{0x24},
                                         std::byte{0x25}};

  producer.Write(msg1);
  producer.Write(msg2);

  // Consume msg1 to free its space before the producer wraps and reuses it.
  constexpr size_t kReadBufSize = 64;
  std::array<std::byte, kReadBufSize> buf{};
  int32_t bytesRead = consumer.TryRead(buf);
  ASSERT_EQ(bytesRead, 5);
  EXPECT_EQ(std::memcmp(buf.data(), msg1.data(), 5), 0);

  // msg3 triggers a padding sentinel at offset 24 and wraps to offset 0.
  producer.Write(msg3);

  bytesRead = consumer.TryRead(buf);
  ASSERT_EQ(bytesRead, 5);
  EXPECT_EQ(std::memcmp(buf.data(), msg2.data(), 5), 0);

  // Consumer skips the sentinel and reads msg3 from offset 0.
  bytesRead = consumer.TryRead(buf);
  ASSERT_EQ(bytesRead, 5);
  EXPECT_EQ(std::memcmp(buf.data(), msg3.data(), 5), 0);

  EXPECT_EQ(consumer.TryRead(buf), 0);
}
