/**************************************************************************/
/*  condition_variable_test.cpp                                           */
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
 * @file tests/condition_variable_test.cpp
 * @brief Unit tests for `ConditionVariable` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/condition_variable.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <type_traits>

namespace {

static_assert(std::is_same_v<zcore::ConditionVariable::NativeHandleType, std::condition_variable_any>);
static_assert(!std::is_copy_constructible_v<zcore::ConditionVariable>);
static_assert(!std::is_copy_assignable_v<zcore::ConditionVariable>);
static_assert(!std::is_move_constructible_v<zcore::ConditionVariable>);
static_assert(!std::is_move_assignable_v<zcore::ConditionVariable>);

TEST(ConditionVariableTest, NotifyOneWakesSingleWaiter) {
  zcore::ConditionVariable condition;
  zcore::Mutex mutex;
  std::atomic<bool> waiterReady = false;
  bool ready = false;
  bool observed = false;

  std::thread waiter([&condition, &mutex, &waiterReady, &ready, &observed]() {
    std::unique_lock<zcore::Mutex> lock(mutex);
    waiterReady.store(true, std::memory_order_release);
    condition.Wait(lock, [&ready]() {
      return ready;
    });
    observed = ready;
  });

  while (!waiterReady.load(std::memory_order_acquire)) {
    std::this_thread::yield();
  }

  {
    const std::lock_guard<zcore::Mutex> lock(mutex);
    ready = true;
  }
  condition.NotifyOne();
  waiter.join();

  EXPECT_TRUE(observed);
}

TEST(ConditionVariableTest, WaitForTimesOutWhenPredicateNeverSatisfied) {
  zcore::ConditionVariable condition;
  zcore::Mutex mutex;
  std::unique_lock<zcore::Mutex> lock(mutex);

  const auto start = std::chrono::steady_clock::now();
  const bool signaled = condition.WaitFor(
      lock,
      zcore::Duration::FromMilliseconds(15),
      []() {
        return false;
      });
  const auto elapsed = std::chrono::steady_clock::now() - start;

  EXPECT_FALSE(signaled);
  EXPECT_GE(elapsed, std::chrono::milliseconds(10));
}

TEST(ConditionVariableTest, WaitForReturnsTrueAfterNotifyAll) {
  zcore::ConditionVariable condition;
  zcore::Mutex mutex;
  std::atomic<bool> waiterReady = false;
  bool ready = false;
  bool result = false;

  std::thread waiter([&condition, &mutex, &waiterReady, &ready, &result]() {
    std::unique_lock<zcore::Mutex> lock(mutex);
    waiterReady.store(true, std::memory_order_release);
    result = condition.WaitFor(
        lock,
        zcore::Duration::FromSeconds(1),
        [&ready]() {
          return ready;
        });
  });

  while (!waiterReady.load(std::memory_order_acquire)) {
    std::this_thread::yield();
  }

  {
    const std::lock_guard<zcore::Mutex> lock(mutex);
    ready = true;
  }
  condition.NotifyAll();
  waiter.join();

  EXPECT_TRUE(result);
}

}  // namespace
