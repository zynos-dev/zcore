/**************************************************************************/
/*  synchronized_test.cpp                                                 */
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
 * @file tests/synchronized_test.cpp
 * @brief Unit tests for `Synchronized<T>` guarded access contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/synchronized.hpp>

#include <gtest/gtest.h>

#include <thread>
#include <type_traits>
#include <vector>

namespace {

struct MoveOnlyValue final {
  int Payload = 0;

  MoveOnlyValue() = default;
  explicit MoveOnlyValue(const int payload) : Payload(payload) {}
  MoveOnlyValue(const MoveOnlyValue&) = delete;
  MoveOnlyValue& operator=(const MoveOnlyValue&) = delete;
  MoveOnlyValue(MoveOnlyValue&&) noexcept = default;
  MoveOnlyValue& operator=(MoveOnlyValue&&) noexcept = default;
  ~MoveOnlyValue() = default;
};

using IntSync = zcore::Synchronized<int>;
using IntGuard = IntSync::Guard;
using IntConstGuard = IntSync::ConstGuard;

static_assert(!std::is_copy_constructible_v<IntSync>);
static_assert(!std::is_copy_assignable_v<IntSync>);
static_assert(!std::is_move_constructible_v<IntSync>);
static_assert(!std::is_move_assignable_v<IntSync>);
static_assert(!std::is_copy_constructible_v<IntGuard>);
static_assert(!std::is_copy_assignable_v<IntGuard>);
static_assert(std::is_move_constructible_v<IntGuard>);
static_assert(std::is_move_assignable_v<IntGuard>);
static_assert(!std::is_copy_constructible_v<IntConstGuard>);
static_assert(!std::is_copy_assignable_v<IntConstGuard>);
static_assert(std::is_move_constructible_v<IntConstGuard>);
static_assert(std::is_move_assignable_v<IntConstGuard>);

TEST(SynchronizedTest, DefaultConstructedValueIsAccessibleThroughGuard) {
  zcore::Synchronized<int> value;
  auto guard = value.Lock();
  EXPECT_EQ(*guard, 0);
}

TEST(SynchronizedTest, GuardProvidesMutableAccessAndReleasesOnScopeExit) {
  zcore::Synchronized<int> value(7);

  {
    auto guard = value.Lock();
    EXPECT_EQ(guard.Value(), 7);
    guard.Value() = 15;
  }

  auto verify = value.Lock();
  EXPECT_EQ(verify.Value(), 15);
}

TEST(SynchronizedTest, TryLockFailsWhileAlreadyLocked) {
  zcore::Synchronized<int> value(9);
  auto guard = value.Lock();
  EXPECT_FALSE(value.TryLock().HasValue());
}

TEST(SynchronizedTest, ConstGuardProvidesReadOnlyAccess) {
  const zcore::Synchronized<int> value(31);
  const zcore::Synchronized<int>& constRef = value;

  auto guard = constRef.Lock();
  EXPECT_EQ(guard.Value(), 31);
  EXPECT_EQ(*guard, 31);
}

TEST(SynchronizedTest, MoveOnlyValueTypeIsSupported) {
  zcore::Synchronized<MoveOnlyValue> value(MoveOnlyValue(5));

  auto guard = value.Lock();
  EXPECT_EQ(guard->Payload, 5);
  guard->Payload = 22;
}

TEST(SynchronizedTest, ConcurrentWritersAreSerializedByInternalMutex) {
  zcore::Synchronized<int> counter(0);
  constexpr int kThreadCount = 4;
  constexpr int kIterationsPerThread = 3000;

  std::vector<std::thread> workers;
  workers.reserve(kThreadCount);
  for (int threadIndex = 0; threadIndex < kThreadCount; ++threadIndex) {
    workers.emplace_back([&counter]() {
      for (int iteration = 0; iteration < kIterationsPerThread; ++iteration) {
        auto guard = counter.Lock();
        ++guard.Value();
      }
    });
  }

  for (std::thread& worker : workers) {
    worker.join();
  }

  auto result = counter.Lock();
  EXPECT_EQ(result.Value(), kThreadCount * kIterationsPerThread);
}

}  // namespace
