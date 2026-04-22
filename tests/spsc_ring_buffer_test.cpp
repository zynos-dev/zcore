/**************************************************************************/
/*  spsc_ring_buffer_test.cpp                                             */
/**************************************************************************/
/*  zcore                                                                 */
/*  Copyright (c) 2026 Logan Yagadics                                     */
/*  Copyright (c) 2026 zcore Contributors                                 */
/*                                                                        */
/*  This file is part of the zcore project.                               */
/*  Use of this source code is governed by the license defined            */
/*  in the LICENSE file at the root of this repository.                   */
/**************************************************************************/
/**
 * @file tests/spsc_ring_buffer_test.cpp
 * @brief Unit tests for `SpscRingBuffer<T, CapacityV>` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/spsc_ring_buffer.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <type_traits>
#include <vector>

namespace {

struct MoveOnly final {
  int Value = 0;

  MoveOnly() = default;
  explicit MoveOnly(const int value) : Value(value) {}
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) noexcept = default;
  MoveOnly& operator=(MoveOnly&&) noexcept = default;
  ~MoveOnly() = default;
};

static_assert(!std::is_copy_constructible_v<zcore::SpscRingBuffer<int, 8>>);
static_assert(!std::is_copy_assignable_v<zcore::SpscRingBuffer<int, 8>>);
static_assert(!std::is_move_constructible_v<zcore::SpscRingBuffer<int, 8>>);
static_assert(!std::is_move_assignable_v<zcore::SpscRingBuffer<int, 8>>);

TEST(SpscRingBufferTest, DefaultConstructedIsEmptyWithDeterministicCapacity) {
  const zcore::SpscRingBuffer<int, 8> queue;
  EXPECT_TRUE(queue.Empty());
  EXPECT_FALSE(queue.Full());
  EXPECT_EQ(queue.SizeApprox(), 0U);
  EXPECT_EQ(queue.Capacity(), 8U);
}

TEST(SpscRingBufferTest, PushPopAndFullStateFollowFifoContract) {
  zcore::SpscRingBuffer<int, 3> queue;
  EXPECT_TRUE(queue.TryPush(1));
  EXPECT_TRUE(queue.TryPush(2));
  EXPECT_TRUE(queue.TryPush(3));
  EXPECT_TRUE(queue.Full());
  EXPECT_FALSE(queue.TryPush(4));

  auto first = queue.TryPop();
  auto second = queue.TryPop();
  auto third = queue.TryPop();
  ASSERT_TRUE(first.HasValue());
  ASSERT_TRUE(second.HasValue());
  ASSERT_TRUE(third.HasValue());
  EXPECT_EQ(first.Value(), 1);
  EXPECT_EQ(second.Value(), 2);
  EXPECT_EQ(third.Value(), 3);
  EXPECT_TRUE(queue.Empty());
  EXPECT_FALSE(queue.TryPop().HasValue());
}

TEST(SpscRingBufferTest, WrapAroundPreservesOrderAcrossCapacityBoundary) {
  zcore::SpscRingBuffer<int, 4> queue;
  for (int value = 0; value < 20; ++value) {
    EXPECT_TRUE(queue.TryPush(value));
    const auto popped = queue.TryPop();
    ASSERT_TRUE(popped.HasValue());
    EXPECT_EQ(popped.Value(), value);
  }
  EXPECT_TRUE(queue.Empty());
}

TEST(SpscRingBufferTest, MoveOnlyValuesAreSupportedViaEmplaceAndPop) {
  zcore::SpscRingBuffer<MoveOnly, 2> queue;
  EXPECT_TRUE(queue.TryEmplace(7));
  EXPECT_TRUE(queue.TryEmplace(9));
  EXPECT_FALSE(queue.TryEmplace(11));

  auto first = queue.TryPop();
  auto second = queue.TryPop();
  ASSERT_TRUE(first.HasValue());
  ASSERT_TRUE(second.HasValue());
  EXPECT_EQ(first.Value().Value, 7);
  EXPECT_EQ(second.Value().Value, 9);
  EXPECT_FALSE(queue.TryPop().HasValue());
}

TEST(SpscRingBufferTest, SingleProducerSingleConsumerThreadsDrainAllValues) {
  constexpr int kTotalValues = 40000;
  zcore::SpscRingBuffer<int, 1024> queue;
  std::vector<int> consumed;
  consumed.reserve(kTotalValues);

  std::atomic<bool> producerDone = false;
  std::thread producer([&queue, &producerDone]() {
    for (int value = 0; value < kTotalValues; ++value) {
      while (!queue.TryPush(value)) {
        std::this_thread::yield();
      }
    }
    producerDone.store(true, std::memory_order_release);
  });

  while (consumed.size() < static_cast<std::size_t>(kTotalValues)) {
    auto value = queue.TryPop();
    if (value.HasValue()) {
      consumed.push_back(value.Value());
      continue;
    }
    if (!producerDone.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
  }

  producer.join();
  ASSERT_EQ(consumed.size(), static_cast<std::size_t>(kTotalValues));
  for (int index = 0; index < kTotalValues; ++index) {
    EXPECT_EQ(consumed[static_cast<std::size_t>(index)], index);
  }
  EXPECT_TRUE(queue.Empty());
}

}  // namespace
