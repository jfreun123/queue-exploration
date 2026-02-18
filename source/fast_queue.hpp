#pragma once

#include "expect.hpp"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <span>

namespace CustomQueues {

constexpr size_t CACHE_LINE_SIZE = 64;

struct FastQueue {
  // Both counters are written by the producer, read by the consumer(s).
  // Before/after a Write operation, both counters contain the same value.

  // [WriteCounter,ReadCounter] defines the area where data can be read
  alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> mReadCounter{0};

  // [ReadCounter,WriteCounter] defines the area where data is being written to
  alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> mWriteCounter{0};

  alignas(CACHE_LINE_SIZE) uint8_t mBuffer[];
};

class QProducer {
public:
  explicit QProducer(FastQueue *q) : mQ(q), mNextElement(q->mBuffer) {}

  void Write(std::span<const std::byte> buffer) {
    const int32_t size = static_cast<int32_t>(buffer.size());
    const int32_t payloadSize = sizeof(int32_t) + size;

    mLocalCounter += payloadSize;
    mQ->mWriteCounter.store(mLocalCounter, std::memory_order_release);

    std::memcpy(mNextElement, &size, sizeof(int32_t));
    std::memcpy(mNextElement + sizeof(int32_t), buffer.data(), size);

    mQ->mReadCounter.store(mLocalCounter, std::memory_order_release);
    mNextElement += payloadSize;
  }

private:
  FastQueue *mQ;
  uint64_t mLocalCounter = 0;
  uint8_t *mNextElement;
};

class QConsumer {
public:
  explicit QConsumer(FastQueue *q, uint64_t queueSize)
      : mQ(q), mQueueSize(queueSize), mNextElement(q->mBuffer) {}

  int32_t TryRead(std::span<std::byte> buffer) {
    if (mLocalCounter == mQ->mReadCounter.load(std::memory_order_acquire))
      return 0;

    int32_t size;
    std::memcpy(&size, mNextElement, sizeof(int32_t));

    uint64_t writeCounter = mQ->mWriteCounter.load(std::memory_order_acquire);
    EXPECT(writeCounter - mLocalCounter <= mQueueSize, "queue overflow");
    EXPECT(size <= static_cast<int32_t>(buffer.size()),
           "buffer space isn't large enough");

    std::memcpy(buffer.data(), mNextElement + sizeof(int32_t), size);

    const int32_t payloadSize = sizeof(int32_t) + size;
    mLocalCounter += payloadSize;
    mNextElement += payloadSize;

    writeCounter = mQ->mWriteCounter.load(std::memory_order_acquire);
    EXPECT(writeCounter - mLocalCounter <= mQueueSize, "queue overflow");

    return size;
  }

private:
  FastQueue *mQ;
  uint64_t mQueueSize;
  uint64_t mLocalCounter = 0;
  uint8_t *mNextElement;
};

} // namespace CustomQueues
