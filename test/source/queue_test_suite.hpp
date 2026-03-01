#pragma once

#include <gtest/gtest.h>
#include <optional>
#include <thread>
#include <vector>

// Shared typed test suite for any queue environment.
//
// QueueEnv requirements:
//   void push(int val)
//   std::optional<int> tryPop()

template <typename QueueEnv>
class QueueTest : public ::testing::Test {
protected:
  QueueEnv env;
};

TYPED_TEST_SUITE_P(QueueTest);

TYPED_TEST_P(QueueTest, PushAndPopReturnsSingleValue) {
  constexpr int kValue = 42;
  this->env.push(kValue);
  EXPECT_EQ(this->env.tryPop(), std::optional<int>{kValue});
}

TYPED_TEST_P(QueueTest, TryPopOnEmptyReturnsNullopt) {
  EXPECT_EQ(this->env.tryPop(), std::nullopt);
}

TYPED_TEST_P(QueueTest, FifoOrdering) {
  constexpr int kNumMessages = 10;
  for (int i = 0; i < kNumMessages; ++i) {
    this->env.push(i);
  }
  for (int i = 0; i < kNumMessages; ++i) {
    EXPECT_EQ(this->env.tryPop(), std::optional<int>{i});
  }
  EXPECT_EQ(this->env.tryPop(), std::nullopt);
}

TYPED_TEST_P(QueueTest, ConcurrentProducerConsumer) {
  constexpr int kNumItems = 500;
  std::vector<int> received;
  received.reserve(kNumItems);

  std::thread producerThread([this]() {
    for (int idx = 0; idx < kNumItems; ++idx) {
      this->env.push(idx);
    }
  });

  std::thread consumerThread([this, &received]() {
    while (static_cast<int>(received.size()) < kNumItems) {
      auto opt = this->env.tryPop();
      if (opt.has_value()) {
        received.push_back(opt.value());
      } else {
        std::this_thread::yield();
      }
    }
  });

  producerThread.join();
  consumerThread.join();

  ASSERT_EQ(static_cast<int>(received.size()), kNumItems);
  for (int idx = 0; idx < kNumItems; ++idx) {
    EXPECT_EQ(received[static_cast<size_t>(idx)], idx);
  }
}

REGISTER_TYPED_TEST_SUITE_P(QueueTest,
                             PushAndPopReturnsSingleValue,
                             TryPopOnEmptyReturnsNullopt,
                             FifoOrdering,
                             ConcurrentProducerConsumer);
