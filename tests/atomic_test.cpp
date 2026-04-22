/**************************************************************************/
/*  atomic_test.cpp                                                       */
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
 * @file tests/atomic_test.cpp
 * @brief Unit tests for `Atomic<T>` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/atomic.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <thread>
#include <type_traits>
#include <vector>

namespace {

struct TrivialPod final {
  std::uint32_t Value;
};

static_assert(std::is_trivially_copyable_v<TrivialPod>);
static_assert(!std::is_copy_constructible_v<zcore::Atomic<std::uint32_t>>);
static_assert(!std::is_copy_assignable_v<zcore::Atomic<std::uint32_t>>);
static_assert(!std::is_move_constructible_v<zcore::Atomic<std::uint32_t>>);
static_assert(!std::is_move_assignable_v<zcore::Atomic<std::uint32_t>>);
static_assert(requires(zcore::Atomic<std::uint32_t> value) {
  value.FetchAdd(1U);
});
static_assert(!zcore::detail::AtomicFetchAddable<TrivialPod>);

TEST(AtomicTest, DefaultConstructedIsValueInitialized) {
  const zcore::Atomic<std::uint32_t> value;
  EXPECT_EQ(value.Load(), 0U);
}

TEST(AtomicTest, StoreLoadAndExchangeAreDeterministic) {
  zcore::Atomic<std::uint32_t> value(7U);
  EXPECT_EQ(value.Load(), 7U);

  value.Store(11U);
  EXPECT_EQ(value.Load(), 11U);

  const std::uint32_t previous = value.Exchange(23U);
  EXPECT_EQ(previous, 11U);
  EXPECT_EQ(value.Load(), 23U);
}

TEST(AtomicTest, CompareExchangeStrongReportsSuccessAndFailure) {
  zcore::Atomic<std::uint32_t> value(10U);

  std::uint32_t expected = 10U;
  const bool first = value.CompareExchangeStrong(expected, 20U);
  EXPECT_TRUE(first);
  EXPECT_EQ(value.Load(), 20U);
  EXPECT_EQ(expected, 10U);

  expected = 10U;
  const bool second = value.CompareExchangeStrong(expected, 99U);
  EXPECT_FALSE(second);
  EXPECT_EQ(value.Load(), 20U);
  EXPECT_EQ(expected, 20U);
}

TEST(AtomicTest, FetchArithmeticAndBitwiseOperationsReturnPreviousValue) {
  zcore::Atomic<std::uint32_t> value(0b1010U);

  EXPECT_EQ(value.FetchAdd(2U), 0b1010U);
  EXPECT_EQ(value.Load(), 0b1100U);

  EXPECT_EQ(value.FetchSub(1U), 0b1100U);
  EXPECT_EQ(value.Load(), 0b1011U);

  EXPECT_EQ(value.FetchAnd(0b0111U), 0b1011U);
  EXPECT_EQ(value.Load(), 0b0011U);

  EXPECT_EQ(value.FetchOr(0b1000U), 0b0011U);
  EXPECT_EQ(value.Load(), 0b1011U);

  EXPECT_EQ(value.FetchXor(0b1111U), 0b1011U);
  EXPECT_EQ(value.Load(), 0b0100U);
}

TEST(AtomicTest, ConcurrentFetchAddMaintainsDeterministicFinalValue) {
  zcore::Atomic<std::uint32_t> counter(0U);

  constexpr int kThreadCount = 4;
  constexpr int kIterationsPerThread = 4000;

  std::vector<std::thread> workers;
  workers.reserve(kThreadCount);
  for (int threadIndex = 0; threadIndex < kThreadCount; ++threadIndex) {
    workers.emplace_back([&counter]() {
      for (int iteration = 0; iteration < kIterationsPerThread; ++iteration) {
        static_cast<void>(counter.FetchAdd(1U, std::memory_order_relaxed));
      }
    });
  }

  for (std::thread& worker : workers) {
    worker.join();
  }

  EXPECT_EQ(counter.Load(std::memory_order_relaxed), static_cast<std::uint32_t>(kThreadCount * kIterationsPerThread));
}

TEST(AtomicTest, LockFreeQueryIsCallable) {
  const zcore::Atomic<std::uint64_t> value(1ULL);
  const bool runtime = value.IsLockFree();
  static_cast<void>(runtime);
}

}  // namespace
