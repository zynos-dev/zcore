/**************************************************************************/
/*  mutex_test.cpp                                                        */
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
 * @file tests/mutex_test.cpp
 * @brief Unit tests for `Mutex` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/mutex.hpp>

#include <gtest/gtest.h>

#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

namespace {

static_assert(!std::is_copy_constructible_v<zcore::Mutex>);
static_assert(!std::is_copy_assignable_v<zcore::Mutex>);
static_assert(!std::is_move_constructible_v<zcore::Mutex>);
static_assert(!std::is_move_assignable_v<zcore::Mutex>);

TEST(MutexTest, TryLockReflectsExclusiveOwnership) {
  zcore::Mutex mutex;

  EXPECT_TRUE(mutex.TryLock());
  EXPECT_FALSE(mutex.TryLock());
  mutex.Unlock();
  EXPECT_TRUE(mutex.TryLock());
  mutex.Unlock();
}

TEST(MutexTest, LockGuardAdapterProvidesBasicLockable) {
  zcore::Mutex mutex;
  int value = 0;

  {
    const std::lock_guard<zcore::Mutex> guard(mutex);
    value = 17;
    EXPECT_FALSE(mutex.TryLock());
  }

  EXPECT_EQ(value, 17);
  EXPECT_TRUE(mutex.TryLock());
  mutex.Unlock();
}

TEST(MutexTest, ProtectsSharedStateAcrossThreads) {
  zcore::Mutex mutex;
  int counter = 0;

  constexpr int kThreadCount = 4;
  constexpr int kIterationsPerThread = 2000;

  std::vector<std::thread> workers;
  workers.reserve(kThreadCount);
  for (int threadIndex = 0; threadIndex < kThreadCount; ++threadIndex) {
    workers.emplace_back([&mutex, &counter]() {
      for (int iteration = 0; iteration < kIterationsPerThread; ++iteration) {
        const std::lock_guard<zcore::Mutex> guard(mutex);
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
