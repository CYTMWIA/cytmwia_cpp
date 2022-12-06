#include "message_queue.hpp"

#include <gtest/gtest.h>

#include <thread>

using namespace cytmwia;

TEST(MessqgeQueueConstructor, Positive) {
  EXPECT_NO_THROW(MessqgeQueue<int>(1));
  EXPECT_NO_THROW(MessqgeQueue<int>(10));
  EXPECT_NO_THROW(MessqgeQueue<int>(100));
}

TEST(MessqgeQueueConstructor, Zero) {
  EXPECT_THROW(MessqgeQueue<int>(0), std::invalid_argument);
}

TEST(MessqgeQueueInputChannel, Call) {
  MessqgeQueue<int> mq(2);
  EXPECT_NO_THROW(mq.input_channel());
}

TEST(MessqgeQueueOutputChannel, GreaterThanChannelsCount) {
  MessqgeQueue<int> mq(2);
  EXPECT_THROW(mq.output_channel(2), std::out_of_range);
  EXPECT_THROW(mq.output_channel(3), std::out_of_range);
  EXPECT_THROW(mq.output_channel(4), std::out_of_range);
}

TEST(MessqgeQueueOutputChannel, ValidRange) {
  MessqgeQueue<int> mq(2);
  EXPECT_NO_THROW(mq.output_channel(0));
  EXPECT_NO_THROW(mq.output_channel(1));

  auto& out0 = mq.output_channel(0);
  auto& out1 = mq.output_channel(1);
  EXPECT_NE(&out0, &out1);
}

TEST(MessqgeQueueOutputChannelsSize, AfterConstruct) {
  MessqgeQueue<int> mq(3);
  for (const auto& s : mq.output_channels_size()) {
    ASSERT_EQ(s, 0);
  }
}

TEST(MessqgeQueueOutputChannelsSize, MillionPush) {
  MessqgeQueue<int> mq(3);
  auto& in = mq.input_channel();
  for (int i = 0; i < 1000000; i++) {
    in.push(i);
  }
  for (const auto& s : mq.output_channels_size()) {
    ASSERT_EQ(s, 1000000);
  }
}

TEST(MessqgeQueueOutputChannelsSize, DifferentMaxSize) {
  MessqgeQueue<int> mq(3);
  auto& in = mq.input_channel();
  mq.output_channel(0).max_size(1);
  mq.output_channel(1).max_size(10);
  mq.output_channel(2).max_size(100);
  for (int i = 0; i < 200; i++) {
    in.push(i);
  }
  const auto ss = mq.output_channels_size();
  EXPECT_EQ(ss[0], 1);
  EXPECT_EQ(ss[1], 10);
  EXPECT_EQ(ss[2], 100);
}

TEST(InputChannelPush, MillionTimes) {
  MessqgeQueue<int> mq(2);
  auto& in = mq.input_channel();
  auto& out0 = mq.output_channel(0);
  EXPECT_NO_THROW({
    for (int i = 0; i < 1000000; i++) {
      in.push(i);
      ASSERT_EQ(out0.size(), i + 1);
    }
  });
}

TEST(InputChannelPush, MillionTimesWithThreads) {
  MessqgeQueue<int> mq(2);
  auto& in = mq.input_channel();
  auto& out0 = mq.output_channel(0);
  std::thread t1([&]() {
    for (int i = 0; i < 1000000; i++) {
      in.push(i);
    }
  });
  std::thread t2([&]() {
    for (int i = 0; i < 1000000; i++) {
      in.push(i);
    }
  });
  t1.join();
  t2.join();
  EXPECT_EQ(out0.size(), 2000000);
}

TEST(OutputChannelMaxSize, GetDefault) {
  MessqgeQueue<int> mq(2);
  auto& out0 = mq.output_channel(0);
  EXPECT_EQ(out0.max_size(), 0);
}

TEST(OutputChannelMaxSize, SetZero) {
  MessqgeQueue<int> mq(2);
  auto& out0 = mq.output_channel(0);
  EXPECT_NO_THROW(out0.max_size(0));
}

TEST(OutputChannelMaxSize, SetPositiveAndVerifyByGetter) {
  MessqgeQueue<int> mq(2);
  auto& out0 = mq.output_channel(0);
  EXPECT_NO_THROW({
    for (int i = 1; i < 1000000; i++) {
      out0.max_size(i);
      ASSERT_EQ(out0.max_size(), i);
    }
  });
}

TEST(OutputChannelMaxSize, SetPositiveAndVerifyByPop) {
  MessqgeQueue<int> mq(2);
  auto& in = mq.input_channel();
  auto& out0 = mq.output_channel(0);
  for (int i = 0; i < 100; i++) {
    in.push(i);
  }
  for (int i = 90; i > 10; i -= 10) {
    out0.max_size(i);
    ASSERT_EQ(out0.pop(), 100 - i);
  }
  out0.max_size(1);
  EXPECT_EQ(out0.pop(), 99);
}

TEST(OutputChannelMaxSize, SetOne) {
  MessqgeQueue<int> mq(2);
  auto& in = mq.input_channel();
  auto& out0 = mq.output_channel(0);
  out0.max_size(1);
  for (int i = 0; i < 100; i++) {
    in.push(i);
  }
  EXPECT_EQ(out0.pop(), 99);
}

TEST(OutputChannelMaxSize, Stretch) {
  MessqgeQueue<int> mq(2);
  auto& in = mq.input_channel();
  auto& out0 = mq.output_channel(0);
  for (int i = 0; i < 100; i++) {
    in.push(i);
  }
  out0.max_size(10);
  EXPECT_EQ(out0.size(), 10);
  EXPECT_EQ(out0.pop(), 90);
  out0.max_size(20);
  EXPECT_EQ(out0.size(), 9);
  EXPECT_EQ(out0.pop(), 91);
}

TEST(OutputChannelSize, OnlyPush) {
  MessqgeQueue<int> mq(2);
  auto& in = mq.input_channel();
  auto& out0 = mq.output_channel(0);
  EXPECT_EQ(out0.size(), 0);
  EXPECT_NO_THROW({
    for (int i = 0; i < 1000000; i++) {
      in.push(i);
      ASSERT_EQ(out0.size(), i + 1);
    }
  });
}

TEST(OutputChannelSize, PushMillionTimesAndPopMillionTimes) {
  MessqgeQueue<int> mq(2);
  auto& in = mq.input_channel();
  auto& out0 = mq.output_channel(0);
  EXPECT_EQ(out0.size(), 0);
  EXPECT_NO_THROW({
    for (int i = 0; i < 1000000; i++) {
      in.push(i);
      ASSERT_EQ(out0.size(), i + 1);
    }
  });
  EXPECT_NO_THROW({
    for (int i = 0; i < 1000000; i++) {
      out0.pop();
      ASSERT_EQ(out0.size(), 1000000 - 1 - i);
    }
  });
}

TEST(OutputChannelSize, PushAndPopByTurnsMillionTimes) {
  MessqgeQueue<int> mq(2);
  auto& in = mq.input_channel();
  auto& out0 = mq.output_channel(0);
  EXPECT_EQ(out0.size(), 0);
  EXPECT_NO_THROW({
    for (int i = 0; i < 1000000; i++) {
      in.push(i);
      in.push(i);
      ASSERT_EQ(out0.size(), i + 2);
      out0.pop();
      ASSERT_EQ(out0.size(), i + 1);
    }
  });
}

TEST(OutputChannelPop, CheckMissing) {
  MessqgeQueue<int> mq(2);
  auto& in = mq.input_channel();
  auto& out0 = mq.output_channel(0);
  int flags[1000000] = {0};
  for (int i = 0; i < 1000000; i++) {
    in.push(i);
  }
  for (int i = 0; i < 1000000; i++) {
    flags[out0.pop()]++;
  }
  for (int i = 0; i < 1000000; i++) {
    ASSERT_EQ(flags[i], 1);
  }
}

TEST(OutputChannelPop, CheckMissingWithThreads) {
  MessqgeQueue<int> mq(2);
  auto& in = mq.input_channel();
  auto& out0 = mq.output_channel(0);
  int flags[1000000] = {0};
  for (int i = 0; i < 1000000; i++) {
    in.push(i);
  }
  std::thread t1([&]() {
    for (int i = 0; i < 500000; i++) {
      flags[out0.pop()]++;
    }
  });
  std::thread t2([&]() {
    for (int i = 0; i < 500000; i++) {
      flags[out0.pop()]++;
    }
  });
  t1.join();
  t2.join();
  for (int i = 0; i < 1000000; i++) {
    ASSERT_EQ(flags[i], 1);
  }
}

TEST(OutputChannelPop, CheckMissingAllInThreads) {
  MessqgeQueue<int> mq(2);
  auto& in = mq.input_channel();
  auto& out0 = mq.output_channel(0);
  int flags[1000000] = {0};
  std::thread t1([&]() {
    for (int i = 0; i < 1000000; i++) {
      in.push(i);
    }
  });
  std::thread t2([&]() {
    for (int i = 0; i < 500000; i++) {
      flags[out0.pop()]++;
    }
  });
  std::thread t3([&]() {
    for (int i = 0; i < 500000; i++) {
      flags[out0.pop()]++;
    }
  });
  t1.join();
  t2.join();
  t3.join();
  for (int i = 0; i < 1000000; i++) {
    ASSERT_EQ(flags[i], 1);
  }
}
