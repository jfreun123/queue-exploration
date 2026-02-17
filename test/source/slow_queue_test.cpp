#include "slow_queue.hpp"
#include <gtest/gtest.h>

TEST(SlowQueueTest, PushAndPopReturnsValue) {
  constexpr int kValue = 42;
  CustomQueues::SlowQueueTS<int> queue;
  queue.push(kValue);
  EXPECT_EQ(queue.pop(), std::optional<int>{kValue});
}

TEST(SlowQueueTest, PopOnEmptyReturnsNullopt) {
  CustomQueues::SlowQueueTS<int> queue;
  EXPECT_EQ(queue.pop(), std::nullopt);
}

TEST(SlowQueueTest, FifoOrdering) {
  CustomQueues::SlowQueueTS<int> queue;
  queue.push(1);
  queue.push(2);
  queue.push(3);
  EXPECT_EQ(queue.pop(), std::optional<int>{1});
  EXPECT_EQ(queue.pop(), std::optional<int>{2});
  EXPECT_EQ(queue.pop(), std::optional<int>{3});
}

TEST(SlowQueueTest, EmptyAndSize) {
  constexpr int kPushValue = 10;
  CustomQueues::SlowQueueTS<int> queue;
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0U);
  queue.push(kPushValue);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.size(), 1U);
  queue.pop();
  EXPECT_TRUE(queue.empty());
}
