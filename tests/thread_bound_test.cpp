/**************************************************************************/
/*  thread_bound_test.cpp                                                 */
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
 * @file tests/thread_bound_test.cpp
 * @brief Unit tests for `ThreadBound<T>` owner-thread access contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/thread_bound.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <type_traits>

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

static_assert(!std::is_copy_constructible_v<zcore::ThreadBound<int>>);
static_assert(!std::is_copy_assignable_v<zcore::ThreadBound<int>>);
static_assert(!std::is_move_constructible_v<zcore::ThreadBound<int>>);
static_assert(!std::is_move_assignable_v<zcore::ThreadBound<int>>);

TEST(ThreadBoundTest, DefaultConstructedBindsCurrentThreadAndValue) {
  zcore::ThreadBound<int> value;
  EXPECT_TRUE(value.IsOwnerThread());
  EXPECT_TRUE(value.OwnerThreadId().IsValid());
  ASSERT_NE(value.TryValue(), nullptr);
  EXPECT_EQ(*value.TryValue(), 0);
}

TEST(ThreadBoundTest, ConstructedValueIsReadableAndWritableFromOwnerThread) {
  zcore::ThreadBound<int> value(11);
  ASSERT_TRUE(value.IsOwnerThread());
  EXPECT_EQ(value.Value(), 11);

  value.Value() = 29;
  EXPECT_EQ(value.Value(), 29);
}

TEST(ThreadBoundTest, MoveOnlyValueTypeIsSupported) {
  zcore::ThreadBound<MoveOnlyValue> value(MoveOnlyValue(7));
  ASSERT_NE(value.TryValue(), nullptr);
  EXPECT_EQ(value.Value().Payload, 7);
}

TEST(ThreadBoundTest, ForeignThreadTryValueReturnsNullWithoutCrashing) {
  zcore::ThreadBound<int> value(17);
  std::atomic<bool> sawNull = false;

  std::thread worker([&value, &sawNull]() {
    sawNull.store(value.TryValue() == nullptr, std::memory_order_release);
  });
  worker.join();

  EXPECT_TRUE(sawNull.load(std::memory_order_acquire));
  ASSERT_NE(value.TryValue(), nullptr);
  EXPECT_EQ(value.Value(), 17);
}

#if GTEST_HAS_DEATH_TEST
TEST(ThreadBoundContractTest, ForeignThreadValueAccessTerminates) {
  EXPECT_DEATH(
      {
        zcore::ThreadBound<int> value(5);
        std::thread worker([&value]() {
          static_cast<void>(value.Value());
        });
        worker.join();
      },
      "");
}
#endif

}  // namespace

