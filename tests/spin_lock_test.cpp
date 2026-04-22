/**************************************************************************/
/*  spin_lock_test.cpp                                                    */
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
 * @file tests/spin_lock_test.cpp
 * @brief Unit tests for `SpinLock` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/spin_lock.hpp>

#include <gtest/gtest.h>

#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

namespace {

static_assert(!std::is_copy_constructible_v<zcore::SpinLock>);
static_assert(!std::is_copy_assignable_v<zcore::SpinLock>);
static_assert(!std::is_move_constructible_v<zcore::SpinLock>);
static_assert(!std::is_move_assignable_v<zcore::SpinLock>);

TEST(SpinLockTest, TryLockReflectsExclusiveOwnership) {
  zcore::SpinLock lock;

  EXPECT_FALSE(lock.IsLocked());
  EXPECT_TRUE(lock.TryLock());
  EXPECT_TRUE(lock.IsLocked());
  EXPECT_FALSE(lock.TryLock());
  lock.Unlock();
  EXPECT_FALSE(lock.IsLocked());
}

TEST(SpinLockTest, LockGuardAdapterProvidesBasicLockable) {
  zcore::SpinLock lock;
  int value = 0;

  {
    const std::lock_guard<zcore::SpinLock> guard(lock);
    value = 13;
    EXPECT_TRUE(lock.IsLocked());
  }

  EXPECT_EQ(value, 13);
  EXPECT_FALSE(lock.IsLocked());
}

TEST(SpinLockTest, ProtectsSharedStateAcrossThreads) {
  zcore::SpinLock lock;
  int counter = 0;

  constexpr int kThreadCount = 4;
  constexpr int kIterationsPerThread = 2000;

  std::vector<std::thread> workers;
  workers.reserve(kThreadCount);
  for (int threadIndex = 0; threadIndex < kThreadCount; ++threadIndex) {
    workers.emplace_back([&lock, &counter]() {
      for (int iteration = 0; iteration < kIterationsPerThread; ++iteration) {
        const std::lock_guard<zcore::SpinLock> guard(lock);
        ++counter;
      }
    });
  }

  for (std::thread& worker : workers) {
    worker.join();
  }

  EXPECT_EQ(counter, kThreadCount * kIterationsPerThread);
}

}  // namespace
