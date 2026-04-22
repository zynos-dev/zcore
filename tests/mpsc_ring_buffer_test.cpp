/**************************************************************************/
/*  mpsc_ring_buffer_test.cpp                                             */
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
 * @file tests/mpsc_ring_buffer_test.cpp
 * @brief Unit tests for `MpscRingBuffer<T, CapacityV>` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/mpsc_ring_buffer.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <stdexcept>
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

struct ThrowingMove final {
  int Value = 0;

  ThrowingMove() = default;
  explicit ThrowingMove(const int value) : Value(value) {}
  ThrowingMove(const ThrowingMove&) = delete;
  ThrowingMove& operator=(const ThrowingMove&) = delete;
  ThrowingMove(ThrowingMove&&) noexcept(false) {  // NOLINT(performance-noexcept-move-constructor,bugprone-exception-escape)
    throw std::runtime_error("throwing move");
  }
  ThrowingMove& operator=(ThrowingMove&&) = delete;
  ~ThrowingMove() = default;
};

static_assert(!std::is_copy_constructible_v<zcore::MpscRingBuffer<int, 8>>);
static_assert(!std::is_copy_assignable_v<zcore::MpscRingBuffer<int, 8>>);
static_assert(!std::is_move_constructible_v<zcore::MpscRingBuffer<int, 8>>);
static_assert(!std::is_move_assignable_v<zcore::MpscRingBuffer<int, 8>>);

TEST(MpscRingBufferTest, DefaultConstructedIsEmptyWithDeterministicCapacity) {
  const zcore::MpscRingBuffer<int, 8> queue;
  EXPECT_TRUE(queue.Empty());
  EXPECT_FALSE(queue.Full());
  EXPECT_EQ(queue.SizeApprox(), 0U);
  EXPECT_EQ(queue.Capacity(), 8U);
}

TEST(MpscRingBufferTest, PushPopAndFullStateFollowFifoContract) {
  zcore::MpscRingBuffer<int, 3> queue;
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
  EXPECT_FALSE(queue.TryPop().HasValue());
}

TEST(MpscRingBufferTest, MoveOnlyValuesAreSupportedViaEmplaceAndPop) {
  zcore::MpscRingBuffer<MoveOnly, 2> queue;
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

TEST(MpscRingBufferTest, ClearDoesNotMoveValuesForThrowingMoveTypes) {
  zcore::MpscRingBuffer<ThrowingMove, 4> queue;
  EXPECT_TRUE(queue.TryEmplace(3));
  EXPECT_TRUE(queue.TryEmplace(5));

  queue.Clear();
  EXPECT_TRUE(queue.Empty());
}

TEST(MpscRingBufferTest, MultipleProducersSingleConsumerDrainAllValuesWithoutLoss) {
  constexpr int kProducerCount = 4;
  constexpr int kValuesPerProducer = 5000;
  constexpr int kTotalValues = kProducerCount * kValuesPerProducer;

  zcore::MpscRingBuffer<int, 1024> queue;
  std::vector<std::thread> producers;
  producers.reserve(kProducerCount);

  std::atomic<bool> start = false;
  for (int producer = 0; producer < kProducerCount; ++producer) {
    producers.emplace_back([producer, &queue, &start]() {
      while (!start.load(std::memory_order_acquire)) {
        std::this_thread::yield();
      }
      for (int offset = 0; offset < kValuesPerProducer; ++offset) {
        const int value = (producer * kValuesPerProducer) + offset;
        while (!queue.TryPush(value)) {
          std::this_thread::yield();
        }
      }
    });
  }

  std::vector<int> seen(static_cast<std::size_t>(kTotalValues), 0);
  start.store(true, std::memory_order_release);

  int consumed = 0;
  while (consumed < kTotalValues) {
    auto value = queue.TryPop();
    if (!value.HasValue()) {
      std::this_thread::yield();
      continue;
    }

    const int decoded = value.Value();
    ASSERT_GE(decoded, 0);
    ASSERT_LT(decoded, kTotalValues);
    ++seen[static_cast<std::size_t>(decoded)];
    ++consumed;
  }

  for (std::thread& producer : producers) {
    producer.join();
  }

  for (std::size_t index = 0; index < seen.size(); ++index) {
    EXPECT_EQ(seen[index], 1) << "duplicate or missing value at index " << index;
  }
  EXPECT_TRUE(queue.Empty());
}

}  // namespace
