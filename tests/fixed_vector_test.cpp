/**************************************************************************/
/*  fixed_vector_test.cpp                                                 */
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
 * @file tests/fixed_vector_test.cpp
 * @brief Unit tests for deterministic `FixedVector` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/fixed_vector.hpp>

#include <gtest/gtest.h>

#include <type_traits>
#include <utility>

namespace {

struct MoveOnly final {
  int value;

  explicit MoveOnly(int v) : value(v) {}
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) noexcept = default;
  MoveOnly& operator=(MoveOnly&&) noexcept = default;
  ~MoveOnly() = default;
};

static_assert(!std::is_trivially_copyable_v<zcore::FixedVector<int, 4>>);

TEST(FixedVectorTest, DefaultConstructedIsEmptyWithKnownCapacity) {
  const zcore::FixedVector<int, 4> values;
  EXPECT_TRUE(values.Empty());
  EXPECT_EQ(values.Size(), 0U);
  EXPECT_EQ(values.Capacity(), 4U);
  EXPECT_EQ(values.RemainingCapacity(), 4U);
  EXPECT_EQ(values.Data(), nullptr);
}

TEST(FixedVectorTest, PushBackAndIndexingAreDeterministic) {
  zcore::FixedVector<int, 4> values;
  values.PushBack(10);
  values.PushBack(20);
  static_cast<void>(values.EmplaceBack(30));

  ASSERT_EQ(values.Size(), 3U);
  EXPECT_EQ(values.Front(), 10);
  EXPECT_EQ(values.Back(), 30);
  EXPECT_EQ(values[1], 20);
  EXPECT_FALSE(values.Empty());
}

TEST(FixedVectorTest, TryPushBackRejectsCapacityOverflow) {
  zcore::FixedVector<int, 2> values;
  EXPECT_TRUE(values.TryPushBack(1));
  EXPECT_TRUE(values.TryPushBack(2));
  EXPECT_FALSE(values.TryPushBack(3));
  EXPECT_TRUE(values.Full());
  EXPECT_EQ(values.Size(), 2U);
}

TEST(FixedVectorTest, TryAtAndPopOperationsAreBoundsAware) {
  zcore::FixedVector<int, 3> values;
  values.PushBack(7);
  values.PushBack(8);

  auto* valid = values.TryAt(1U);
  ASSERT_NE(valid, nullptr);
  EXPECT_EQ(*valid, 8);
  EXPECT_EQ(values.TryAt(2U), nullptr);

  EXPECT_TRUE(values.TryPopBack());
  EXPECT_EQ(values.Size(), 1U);
  EXPECT_TRUE(values.TryPopBack());
  EXPECT_FALSE(values.TryPopBack());
}

TEST(FixedVectorTest, TryPopBackValueSupportsMoveOnlyElements) {
  zcore::FixedVector<MoveOnly, 2> values;
  static_cast<void>(values.EmplaceBack(11));
  static_cast<void>(values.EmplaceBack(22));

  auto last = values.TryPopBackValue();
  ASSERT_TRUE(last.HasValue());
  EXPECT_EQ(last.Value().value, 22);
  EXPECT_EQ(values.Size(), 1U);
}

TEST(FixedVectorTest, CopyAndMoveConstructionPreserveValueOrder) {
  zcore::FixedVector<int, 4> source;
  source.PushBack(3);
  source.PushBack(5);
  source.PushBack(8);

  const zcore::FixedVector<int, 4> copied(source);
  ASSERT_EQ(copied.Size(), 3U);
  EXPECT_EQ(copied[0], 3);
  EXPECT_EQ(copied[1], 5);
  EXPECT_EQ(copied[2], 8);

  zcore::FixedVector<int, 4> moved(std::move(source));
  ASSERT_EQ(moved.Size(), 3U);
  EXPECT_EQ(moved[0], 3);
  EXPECT_EQ(moved[1], 5);
  EXPECT_EQ(moved[2], 8);
}

TEST(FixedVectorTest, AsSliceAndAsSliceMutExposeContiguousViews) {
  zcore::FixedVector<int, 3> values;
  values.PushBack(1);
  values.PushBack(2);
  values.PushBack(3);

  const zcore::SliceMut<int> mutableView = values.AsSliceMut();
  mutableView[1] = 42;

  const zcore::Slice<const int> constView = values.AsSlice();
  ASSERT_EQ(constView.Size(), 3U);
  EXPECT_EQ(constView[0], 1);
  EXPECT_EQ(constView[1], 42);
  EXPECT_EQ(constView[2], 3);
}

TEST(FixedVectorTest, ZeroCapacityVectorRejectsInsertions) {
  zcore::FixedVector<int, 0> values;
  EXPECT_TRUE(values.Empty());
  EXPECT_TRUE(values.Full());
  EXPECT_FALSE(values.TryPushBack(9));
  EXPECT_EQ(values.Size(), 0U);
}

#if GTEST_HAS_DEATH_TEST
TEST(FixedVectorContractTest, PushBackOnFullTerminates) {
  EXPECT_DEATH(([]() {
                 zcore::FixedVector<int, 1> values;
                 values.PushBack(1);
                 values.PushBack(2);
               }()),
               "");
}

TEST(FixedVectorContractTest, FrontOnEmptyTerminates) {
  EXPECT_DEATH(([]() {
                 zcore::FixedVector<int, 2> values;
                 static_cast<void>(values.Front());
               }()),
               "");
}

TEST(FixedVectorContractTest, IndexOutOfBoundsTerminates) {
  EXPECT_DEATH(([]() {
                 zcore::FixedVector<int, 2> values;
                 values.PushBack(1);
                 static_cast<void>(values[1]);
               }()),
               "");
}
#endif

}  // namespace

