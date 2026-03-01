#pragma once

#include <mutex>
#include <optional>
#include <queue>

namespace CustomQueues {

template <typename T> class SlowQueueTS {
public:
  void push(const T &value) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(value);
  }

  void push(T &&value) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(std::move(value));
  }

  std::optional<T> pop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
      return std::nullopt;
    }
    T value = std::move(queue_.front());
    queue_.pop();
    return value;
  }

  bool empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
  }

private:
  mutable std::mutex mutex_;
  std::queue<T> queue_;
};

template <typename T> class SlowProducer {
public:
  explicit SlowProducer(SlowQueueTS<T> *q) : mQ(q) {}

  void Push(const T &value) { mQ->push(value); }
  void Push(T &&value) { mQ->push(std::move(value)); }

private:
  SlowQueueTS<T> *mQ;
};

template <typename T> class SlowConsumer {
public:
  explicit SlowConsumer(SlowQueueTS<T> *q) : mQ(q) {}

  std::optional<T> TryPop() { return mQ->pop(); }

private:
  SlowQueueTS<T> *mQ;
};

} // namespace CustomQueues
