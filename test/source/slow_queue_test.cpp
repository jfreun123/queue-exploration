#include "slow_queue.hpp"
#include "queue_test_suite.hpp"

struct SlowQueueEnv {
  CustomQueues::SlowQueueTS<int> queue;
  CustomQueues::SlowProducer<int> producer{&queue};
  CustomQueues::SlowConsumer<int> consumer{&queue};

  void push(int val) { producer.Push(val); }
  std::optional<int> tryPop() { return consumer.TryPop(); }
};

INSTANTIATE_TYPED_TEST_SUITE_P(SlowQueue, QueueTest, SlowQueueEnv);
