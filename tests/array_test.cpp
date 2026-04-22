/**************************************************************************/
/*  array_test.cpp                                                        */
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
 * @file tests/array_test.cpp
 * @brief Unit tests for deterministic `Array` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/array.hpp>

#include <gtest/gtest.h>

#include <array>
#include <type_traits>

namespace {

static_assert(sizeof(zcore::Array<int, 4>) == sizeof(std::array<int, 4>));
static_assert(std::is_trivially_copyable_v<zcore::Array<int, 4>>);

TEST(ArrayTest, DefaultConstructedHasDeterministicExtentAndStorage) {
  zcore::Array<int, 4> values;

  EXPECT_EQ(values.Size(), 4U);
  EXPECT_EQ(values.Extent(), 4U);
  EXPECT_FALSE(values.Empty());
  ASSERT_NE(values.Data(), nullptr);
  EXPECT_EQ(values[0], 0);
  EXPECT_EQ(values[1], 0);
  EXPECT_EQ(values[2], 0);
  EXPECT_EQ(values[3], 0);
}

TEST(ArrayTest, ConstructsFromStdArrayAndCArray) {
  const std::array<int, 3> stdValues = {2, 4, 6};
  const zcore::Array<int, 3> fromStd(stdValues);

  const int cValues[3] = {3, 6, 9};
  const zcore::Array<int, 3> fromC(cValues);

  EXPECT_EQ(fromStd[0], 2);
  EXPECT_EQ(fromStd[2], 6);
  EXPECT_EQ(fromC[0], 3);
  EXPECT_EQ(fromC[2], 9);
}

TEST(ArrayTest, IndexingTryAtFrontBackAndFillAreDeterministic) {
  zcore::Array<int, 3> values;
  values.Fill(5);
  values[1] = 7;

  EXPECT_EQ(values.Front(), 5);
  EXPECT_EQ(values.Back(), 5);
  EXPECT_EQ(values[1], 7);
  EXPECT_NE(values.TryAt(1), nullptr);
  EXPECT_EQ(values.TryAt(3), nullptr);
}

TEST(ArrayTest, AsSliceAndAsSliceMutExposeContiguousViews) {
  zcore::Array<int, 3> values;
  values.Fill(1);

  const zcore::SliceMut<int> mutableView = values.AsSliceMut();
  mutableView[2] = 42;

  const zcore::Slice<const int> constView = values.AsSlice();
  ASSERT_EQ(constView.Size(), 3U);
  EXPECT_EQ(constView[0], 1);
  EXPECT_EQ(constView[1], 1);
  EXPECT_EQ(constView[2], 42);

  const zcore::Slice<const int> implicitConstView = values;
  EXPECT_EQ(implicitConstView[2], 42);
}

TEST(ArrayTest, ZeroExtentArrayIsAlwaysEmptyAndProvidesEmptyViews) {
  const zcore::Array<int, 0> values;

  EXPECT_TRUE(values.Empty());
  EXPECT_EQ(values.Size(), 0U);
  EXPECT_EQ(values.Data(), nullptr);
  EXPECT_EQ(values.begin(), nullptr);
  EXPECT_EQ(values.end(), nullptr);
  EXPECT_EQ(values.TryAt(0), nullptr);

  const zcore::Slice<const int> view = values.AsSlice();
  EXPECT_TRUE(view.EmptyView());
  EXPECT_EQ(view.Data(), nullptr);
}

#if GTEST_HAS_DEATH_TEST
TEST(ArrayContractTest, IndexOutOfBoundsTerminates) {
  EXPECT_DEATH(([]() {
                 zcore::Array<int, 2> values;
                 static_cast<void>(values[2]);
               }()),
               "");
}

TEST(ArrayContractTest, FrontOnZeroExtentTerminates) {
  EXPECT_DEATH(([]() {
                 zcore::Array<int, 0> values;
                 static_cast<void>(values.Front());
               }()),
               "");
}

TEST(ArrayContractTest, BackOnZeroExtentTerminates) {
  EXPECT_DEATH(([]() {
                 zcore::Array<int, 0> values;
                 static_cast<void>(values.Back());
               }()),
               "");
}
#endif

}  // namespace
