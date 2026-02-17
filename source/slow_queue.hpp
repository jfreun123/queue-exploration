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

} // namespace CustomQueues
