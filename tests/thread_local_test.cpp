/**************************************************************************/
/*  thread_local_test.cpp                                                 */
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
 * @file tests/thread_local_test.cpp
 * @brief Unit tests for `ThreadLocal<T, TagT>` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/thread_local.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <type_traits>

namespace {

struct TagA final {};
struct TagB final {};

struct CopyConstructOnly final {
  int Value = 0;

  CopyConstructOnly() = default;
  explicit CopyConstructOnly(const int value) : Value(value) {}
  CopyConstructOnly(const CopyConstructOnly&) = default;
  CopyConstructOnly& operator=(const CopyConstructOnly&) = delete;
  CopyConstructOnly(CopyConstructOnly&&) noexcept = default;
  CopyConstructOnly& operator=(CopyConstructOnly&&) = delete;
  ~CopyConstructOnly() = default;
};

struct MoveConstructOnly final {
  int Value = 0;

  MoveConstructOnly() = default;
  explicit MoveConstructOnly(const int value) : Value(value) {}
  MoveConstructOnly(const MoveConstructOnly&) = delete;
  MoveConstructOnly& operator=(const MoveConstructOnly&) = delete;
  MoveConstructOnly(MoveConstructOnly&&) noexcept = default;
  MoveConstructOnly& operator=(MoveConstructOnly&&) = delete;
  ~MoveConstructOnly() = default;
};

static_assert(std::is_copy_constructible_v<zcore::ThreadLocal<int>>);
static_assert(std::is_copy_assignable_v<zcore::ThreadLocal<int>>);
static_assert(std::is_move_constructible_v<zcore::ThreadLocal<int>>);
static_assert(std::is_move_assignable_v<zcore::ThreadLocal<int>>);

TEST(ThreadLocalTest, DefaultConstructedIsEmpty) {
  zcore::ThreadLocal<int> local;
  local.Reset();

  EXPECT_TRUE(local.IsEmpty());
  EXPECT_FALSE(local.HasValue());
  EXPECT_EQ(local.TryValue(), nullptr);
}

TEST(ThreadLocalTest, SetValueResetAndTryValueAreDeterministic) {
  zcore::ThreadLocal<int> local;
  local.Reset();

  local.Set(7);
  ASSERT_TRUE(local.HasValue());
  EXPECT_EQ(local.Value(), 7);
  ASSERT_NE(local.TryValue(), nullptr);
  EXPECT_EQ(*local.TryValue(), 7);

  local.Reset();
  EXPECT_TRUE(local.IsEmpty());
  EXPECT_EQ(local.TryValue(), nullptr);
}

TEST(ThreadLocalTest, SetSupportsConstructibleButNonAssignableTypes) {
  zcore::ThreadLocal<CopyConstructOnly, TagA> byCopy;
  zcore::ThreadLocal<MoveConstructOnly, TagB> byMove;
  byCopy.Reset();
  byMove.Reset();

  const CopyConstructOnly copyValue(13);
  byCopy.Set(copyValue);
  ASSERT_TRUE(byCopy.HasValue());
  EXPECT_EQ(byCopy.Value().Value, 13);

  byMove.Set(MoveConstructOnly(21));
  ASSERT_TRUE(byMove.HasValue());
  EXPECT_EQ(byMove.Value().Value, 21);
}

TEST(ThreadLocalTest, ValueOrEmplaceConstructsOncePerThreadSlot) {
  zcore::ThreadLocal<int> local;
  local.Reset();

  const int& first = local.ValueOrEmplace(11);
  const int& second = local.ValueOrEmplace(22);
  EXPECT_EQ(first, 11);
  EXPECT_EQ(second, 11);
  EXPECT_EQ(local.Value(), 11);
}

TEST(ThreadLocalTest, InstancesShareSameSlotForSameValueAndTagTypes) {
  zcore::ThreadLocal<int> first;
  zcore::ThreadLocal<int> second;
  first.Reset();

  first.Set(19);
  EXPECT_TRUE(second.HasValue());
  EXPECT_EQ(second.Value(), 19);
}

TEST(ThreadLocalTest, DifferentTagsIsolateSlotsForSameValueType) {
  zcore::ThreadLocal<int, TagA> first;
  zcore::ThreadLocal<int, TagB> second;
  first.Reset();
  second.Reset();

  first.Set(5);
  EXPECT_TRUE(first.HasValue());
  EXPECT_EQ(first.Value(), 5);
  EXPECT_TRUE(second.IsEmpty());

  second.Set(9);
  EXPECT_EQ(first.Value(), 5);
  EXPECT_EQ(second.Value(), 9);
}

TEST(ThreadLocalTest, ThreadsObserveIndependentSlots) {
  zcore::ThreadLocal<int> local;
  local.Reset();
  local.Set(41);

  std::atomic<bool> workerSawEmpty = false;
  std::atomic<int> workerValue = 0;

  std::thread worker([&local, &workerSawEmpty, &workerValue]() {
    workerSawEmpty.store(local.IsEmpty(), std::memory_order_release);
    local.Set(77);
    workerValue.store(local.Value(), std::memory_order_release);
  });
  worker.join();

  EXPECT_TRUE(workerSawEmpty.load(std::memory_order_acquire));
  EXPECT_EQ(workerValue.load(std::memory_order_acquire), 77);
  EXPECT_EQ(local.Value(), 41);
}

#if GTEST_HAS_DEATH_TEST
TEST(ThreadLocalContractTest, ValueOnEmptyTerminates) {
  EXPECT_DEATH(
      {
        zcore::ThreadLocal<int> local;
        local.Reset();
        static_cast<void>(local.Value());
      },
      "");
}
#endif

}  // namespace
