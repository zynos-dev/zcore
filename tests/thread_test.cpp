/**************************************************************************/
/*  thread_test.cpp                                                       */
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
 * @file tests/thread_test.cpp
 * @brief Unit tests for `Thread` ownership and synchronization contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/thread.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <type_traits>
#include <utility>

namespace {

static_assert(!std::is_copy_constructible_v<zcore::Thread>);
static_assert(!std::is_copy_assignable_v<zcore::Thread>);
static_assert(std::is_move_constructible_v<zcore::Thread>);
static_assert(std::is_move_assignable_v<zcore::Thread>);

TEST(ThreadTest, DefaultConstructedIsNonJoinableWithInvalidId) {
  const zcore::Thread thread;
  EXPECT_FALSE(thread.Joinable());
  EXPECT_TRUE(thread.GetId().IsInvalid());
}

TEST(ThreadTest, JoinWaitsForOwnedThreadAndClearsJoinableState) {
  std::atomic<bool> ran = false;
  zcore::Thread thread(std::thread([&ran]() {
    ran.store(true, std::memory_order_release);
  }));

  EXPECT_TRUE(thread.Joinable());
  EXPECT_TRUE(thread.GetId().IsValid());

  thread.Join();
  EXPECT_FALSE(thread.Joinable());
  EXPECT_TRUE(ran.load(std::memory_order_acquire));
  EXPECT_TRUE(thread.GetId().IsInvalid());
}

TEST(ThreadTest, DetachReleasesOwnershipAndThreadContinues) {
  std::atomic<bool> ran = false;
  zcore::Thread thread(std::thread([&ran]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    ran.store(true, std::memory_order_release);
  }));

  thread.Detach();
  EXPECT_FALSE(thread.Joinable());

  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
  while (!ran.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::yield();
  }
  EXPECT_TRUE(ran.load(std::memory_order_acquire));
}

TEST(ThreadTest, MoveConstructionTransfersOwnership) {
  zcore::Thread source(std::thread([]() {}));
  ASSERT_TRUE(source.Joinable());

  zcore::Thread moved(std::move(source));
  EXPECT_TRUE(moved.Joinable());

  moved.Join();
  EXPECT_FALSE(moved.Joinable());
}

TEST(ThreadTest, MoveAssignmentRequiresEmptyTargetAndTransfersOwnership) {
  zcore::Thread source(std::thread([]() {}));
  zcore::Thread target;

  target = std::move(source);
  EXPECT_TRUE(target.Joinable());

  target.Join();
  EXPECT_FALSE(target.Joinable());
}

TEST(ThreadTest, CurrentIdIsValidAndStableWithinCallingThread) {
  const zcore::ThreadId a = zcore::Thread::CurrentId();
  const zcore::ThreadId b = zcore::Thread::CurrentId();
  EXPECT_TRUE(a.IsValid());
  EXPECT_EQ(a, b);
}

#if GTEST_HAS_DEATH_TEST
TEST(ThreadContractTest, JoinOnNonJoinableTerminates) {
  EXPECT_DEATH(
      {
        zcore::Thread thread;
        thread.Join();
      },
      "");
}

TEST(ThreadContractTest, DetachOnNonJoinableTerminates) {
  EXPECT_DEATH(
      {
        zcore::Thread thread;
        thread.Detach();
      },
      "");
}

TEST(ThreadContractTest, DestroyJoinableThreadTerminates) {
  EXPECT_DEATH(
      {
        const zcore::Thread thread(std::thread([]() {
          std::this_thread::sleep_for(std::chrono::milliseconds(4));
        }));
      },
      "");
}

TEST(ThreadContractTest, MoveAssignIntoJoinableTargetTerminates) {
  EXPECT_DEATH(
      {
        zcore::Thread target(std::thread([]() {
          std::this_thread::sleep_for(std::chrono::milliseconds(4));
        }));
        zcore::Thread source(std::thread([]() {}));
        target = std::move(source);
      },
      "");
}
#endif

}  // namespace
