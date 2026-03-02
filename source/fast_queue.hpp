#pragma once

#include "expect.hpp"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>

namespace CustomQueues {

constexpr size_t CACHE_LINE_SIZE = 64;

// Sentinel written at the end of the buffer when a message doesn't fit before
// the wrap point. Consumers skip these records.
constexpr int32_t kPaddingSentinel = std::numeric_limits<int32_t>::min();

// Round n up to the next multiple of 4 (required for buffer alignment).
constexpr uint64_t align4(uint64_t n) noexcept {
  return (n + 3) & ~uint64_t{3};
}

struct FastQueue {
  // Both counters are written by the producer, read by the consumer(s).
  // Before/after a Write operation, both counters contain the same value.

  // [WriteCounter,ReadCounter] defines the area where data can be read
  alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> mReadCounter{0};

  // [ReadCounter,WriteCounter] defines the area where data is being written to
  alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> mWriteCounter{0};

  // Written by consumer(s) after each read to signal that buffer space has
  // been freed. The producer spins on this to enforce the ring-buffer bound.
  alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> mConsumeCounter{0};

  [[nodiscard]] uint8_t *buffer() noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<uint8_t *>(this) + sizeof(FastQueue);
  }
};

// Advance *counter to newVal if newVal is larger (fetch_max via CAS loop).
// Used by QConsumerShared where multiple threads may advance concurrently.
inline void advanceCounter(std::atomic<uint64_t> &counter,
                           uint64_t newVal) noexcept {
  uint64_t cur = counter.load(std::memory_order_relaxed);
  while (cur < newVal) {
    if (counter.compare_exchange_weak(cur, newVal, std::memory_order_release,
                                      std::memory_order_relaxed))
      break;
  }
}

class QProducer {
public:
  explicit QProducer(FastQueue *q, uint64_t queueSize)
      : mQ(q), mQueueSize(queueSize) {
    EXPECT(queueSize % 4 == 0, "queue size must be a multiple of 4");
  }

  void Write(std::span<const std::byte> buffer) {
    const int32_t size = static_cast<int32_t>(buffer.size());
    const uint64_t alignedPayloadSize =
        align4(uint64_t{sizeof(int32_t)} + static_cast<uint64_t>(size));
    EXPECT(alignedPayloadSize <= mQueueSize, "message larger than queue");

    uint64_t offset = mLocalCounter % mQueueSize;
    const uint64_t remaining = mQueueSize - offset;

    if (alignedPayloadSize > remaining && remaining > 0) {
      // Wait until there is space for the padding record.
      while (mLocalCounter + remaining -
                 mQ->mConsumeCounter.load(std::memory_order_acquire) >
             mQueueSize) {
      }
      mLocalCounter += remaining;
      mQ->mWriteCounter.store(mLocalCounter, std::memory_order_release);
      std::memcpy(mQ->buffer() + offset, &kPaddingSentinel, sizeof(int32_t));
      mQ->mReadCounter.store(mLocalCounter, std::memory_order_release);
      offset = 0;
    }

    // Wait until there is space for the message.
    while (mLocalCounter + alignedPayloadSize -
               mQ->mConsumeCounter.load(std::memory_order_acquire) >
           mQueueSize) {
    }

    mLocalCounter += alignedPayloadSize;
    mQ->mWriteCounter.store(mLocalCounter, std::memory_order_release);

    std::memcpy(mQ->buffer() + offset, &size, sizeof(int32_t));
    std::memcpy(mQ->buffer() + offset + sizeof(int32_t), buffer.data(),
                static_cast<size_t>(size));

    mQ->mReadCounter.store(mLocalCounter, std::memory_order_release);
  }

private:
  FastQueue *mQ;
  uint64_t mQueueSize;
  uint64_t mLocalCounter = 0;
};

class QConsumer {
public:
  explicit QConsumer(FastQueue *q, uint64_t queueSize)
      : mQ(q), mQueueSize(queueSize) {}

  int32_t TryRead(std::span<std::byte> buffer) {
    if (mLocalCounter == mQ->mReadCounter.load(std::memory_order_acquire))
      return 0;

    uint64_t offset = mLocalCounter % mQueueSize;
    int32_t size;
    std::memcpy(&size, mQ->buffer() + offset, sizeof(int32_t));

    if (size == kPaddingSentinel) [[unlikely]] {
      mLocalCounter += mQueueSize - offset;
      mQ->mConsumeCounter.store(mLocalCounter, std::memory_order_release);
      if (mLocalCounter == mQ->mReadCounter.load(std::memory_order_acquire))
        return 0;
      offset = 0;
      std::memcpy(&size, mQ->buffer(), sizeof(int32_t));
    }

    uint64_t writeCounter = mQ->mWriteCounter.load(std::memory_order_acquire);
    EXPECT(writeCounter - mLocalCounter <= mQueueSize, "queue overflow");
    EXPECT(size <= static_cast<int32_t>(buffer.size()),
           "buffer space isn't large enough");

    std::memcpy(buffer.data(), mQ->buffer() + offset + sizeof(int32_t),
                static_cast<size_t>(size));

    const uint64_t alignedPayloadSize =
        align4(uint64_t{sizeof(int32_t)} + static_cast<uint64_t>(size));
    mLocalCounter += alignedPayloadSize;
    mQ->mConsumeCounter.store(mLocalCounter, std::memory_order_release);

    writeCounter = mQ->mWriteCounter.load(std::memory_order_acquire);
    EXPECT(writeCounter - mLocalCounter <= mQueueSize, "queue overflow");

    return size;
  }

private:
  FastQueue *mQ;
  uint64_t mQueueSize;
  uint64_t mLocalCounter = 0;
};

// Like QConsumer but shares a single read position across multiple instances
// via CAS, enabling work-stealing across threads instead of broadcast.
class QConsumerShared {
public:
  explicit QConsumerShared(FastQueue *q, uint64_t queueSize,
                           std::atomic<uint64_t> *sharedPos)
      : mQ(q), mQueueSize(queueSize), mSharedPos(sharedPos) {}

  int32_t TryRead(std::span<std::byte> buffer) {
    uint64_t pos = mSharedPos->load(std::memory_order_acquire);
    while (true) {
      if (pos >= mQ->mReadCounter.load(std::memory_order_acquire))
        return 0;

      const uint64_t offset = pos % mQueueSize;
      int32_t size;
      std::memcpy(&size, mQ->buffer() + offset, sizeof(int32_t));

      const uint64_t payloadSize =
          (size == kPaddingSentinel)
              ? mQueueSize - offset
              : align4(uint64_t{sizeof(int32_t)} + static_cast<uint64_t>(size));

      if (mSharedPos->compare_exchange_weak(pos, pos + payloadSize,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
        if (size == kPaddingSentinel) {
          pos += payloadSize;
          advanceCounter(mQ->mConsumeCounter, pos);
          continue;
        }
        EXPECT(mQ->mWriteCounter.load(std::memory_order_acquire) - pos <=
                   mQueueSize,
               "queue overflow");
        EXPECT(size <= static_cast<int32_t>(buffer.size()),
               "buffer space isn't large enough");
        std::memcpy(buffer.data(), mQ->buffer() + offset + sizeof(int32_t),
                    static_cast<size_t>(size));
        advanceCounter(mQ->mConsumeCounter, pos + payloadSize);
        return size;
      }
      // CAS failed: pos was updated to the current shared position. Retry.
    }
  }

private:
  FastQueue *mQ;
  uint64_t mQueueSize;
  std::atomic<uint64_t> *mSharedPos;
};

} // namespace CustomQueues
