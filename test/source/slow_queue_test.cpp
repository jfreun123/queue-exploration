#include "slow_queue.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

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

TEST(SlowQueueTest, ConcurrentPushAndPop) {
  constexpr int kNumItems = 1000;
  CustomQueues::SlowQueueTS<int> queue;
  std::vector<int> popped;
  popped.reserve(static_cast<size_t>(kNumItems));

  std::thread producer([&queue]() {
    for (int idx = 0; idx < kNumItems; ++idx) {
      queue.push(idx);
    }
  });

  std::thread consumer([&queue, &popped]() {
    while (static_cast<int>(popped.size()) < kNumItems) {
      auto opt = queue.pop();
      if (opt.has_value()) {
        popped.push_back(opt.value());
      } else {
        std::this_thread::yield();
      }
    }
  });

  producer.join();
  consumer.join();

  ASSERT_EQ(static_cast<int>(popped.size()), kNumItems);
  for (int idx = 0; idx < kNumItems; ++idx) {
    EXPECT_EQ(popped[static_cast<size_t>(idx)], idx);
  }
}
