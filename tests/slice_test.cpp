/**************************************************************************/
/*  slice_test.cpp                                                        */
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
 * @file tests/slice_test.cpp
 * @brief Unit tests for Slice and SliceMut bounds contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/slice.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <type_traits>

namespace {

static_assert(std::is_trivially_copyable_v<zcore::Slice<int>>);
static_assert(std::is_trivially_copyable_v<zcore::SliceMut<int>>);
static_assert(std::is_same_v<zcore::ByteSpan, zcore::Slice<const zcore::Byte>>);
static_assert(std::is_same_v<zcore::ByteSpanMut, zcore::SliceMut<zcore::Byte>>);

TEST(SliceTest, DefaultConstructedSliceIsEmpty) {
  const zcore::Slice<int> values;
  EXPECT_TRUE(values.EmptyView());
  EXPECT_EQ(values.Size(), 0U);
  EXPECT_EQ(values.Data(), nullptr);
}

TEST(SliceTest, ConstructsFromArrayAndSpan) {
  int values[]{1, 2, 3};
  const zcore::Slice<int> fromArray(values);
  ASSERT_EQ(fromArray.Size(), 3U);
  EXPECT_EQ(fromArray[0], 1);
  EXPECT_EQ(fromArray[2], 3);

  std::span<const int> view(values);
  const zcore::Slice<int> fromSpan(view);
  EXPECT_EQ(fromSpan.Size(), 3U);
}

TEST(SliceTest, TryFromRawRejectsNullNonZeroRanges) {
  auto invalid = zcore::Slice<int>::TryFromRaw(nullptr, 2U);
  EXPECT_FALSE(invalid.HasValue());

  auto validEmpty = zcore::Slice<int>::TryFromRaw(nullptr, 0U);
  ASSERT_TRUE(validEmpty.HasValue());
  EXPECT_TRUE(validEmpty.Value().EmptyView());
}

TEST(SliceTest, SubsliceFirstLastAreBoundsChecked) {
  const int values[]{10, 20, 30, 40};
  zcore::Slice<int> slice(values);

  auto first = slice.First(2U);
  ASSERT_TRUE(first.HasValue());
  EXPECT_EQ(first.Value()[1], 20);

  auto last = slice.Last(2U);
  ASSERT_TRUE(last.HasValue());
  EXPECT_EQ(last.Value()[0], 30);

  auto mid = slice.Subslice(1U, 2U);
  ASSERT_TRUE(mid.HasValue());
  EXPECT_EQ(mid.Value()[0], 20);
  EXPECT_EQ(mid.Value()[1], 30);

  auto outOfRange = slice.Subslice(3U, 2U);
  EXPECT_FALSE(outOfRange.HasValue());
}

TEST(SliceMutTest, MutationsReflectInBackingStorage) {
  int values[]{3, 4, 5};  // NOLINT(misc-const-correctness)
  zcore::SliceMut<int> mutableSlice(values);
  mutableSlice[1] = 42;

  EXPECT_EQ(values[1], 42);
  auto pointer = mutableSlice.TryAt(1U);
  ASSERT_NE(pointer, nullptr);
  EXPECT_EQ(*pointer, 42);
}

TEST(SliceMutTest, RemovePrefixAndSuffixAdjustView) {
  int values[]{1, 2, 3, 4, 5};  // NOLINT(misc-const-correctness)
  zcore::SliceMut<int> mutableSlice(values);

  EXPECT_TRUE(mutableSlice.RemovePrefix(1U));
  EXPECT_TRUE(mutableSlice.RemoveSuffix(2U));
  ASSERT_EQ(mutableSlice.Size(), 2U);
  EXPECT_EQ(mutableSlice[0], 2);
  EXPECT_EQ(mutableSlice[1], 3);

  EXPECT_FALSE(mutableSlice.RemovePrefix(3U));
}

TEST(SliceMutTest, ConvertsToConstSlice) {
  int values[]{9, 8};  // NOLINT(misc-const-correctness)
  zcore::SliceMut<int> mutableSlice(values);
  zcore::Slice<const int> constSlice = mutableSlice;

  EXPECT_EQ(constSlice.Size(), 2U);
  EXPECT_EQ(constSlice[0], 9);
}

TEST(SliceByteViewTest, ByteViewsMatchObjectRepresentationSize) {
  std::array<std::uint32_t, 3> values{1U, 2U, 3U};
  zcore::SliceMut<std::uint32_t> mutableSlice(values);

  zcore::ByteSpan bytes = zcore::AsBytes(mutableSlice);
  zcore::ByteSpanMut writableBytes = zcore::AsWritableBytes(mutableSlice);

  EXPECT_EQ(bytes.Size(), values.size() * sizeof(std::uint32_t));
  EXPECT_EQ(writableBytes.Size(), values.size() * sizeof(std::uint32_t));
}

#if GTEST_HAS_DEATH_TEST
TEST(SliceTest, CheckedRawConstructorTerminatesForNullNonZeroRange) {
  EXPECT_DEATH(
      {
        auto invalid = zcore::Slice<int>(nullptr, 1U);
        static_cast<void>(invalid);
      },
      "");
}

TEST(SliceMutTest, CheckedRawConstructorTerminatesForNullNonZeroRange) {
  EXPECT_DEATH(
      {
        auto invalid = zcore::SliceMut<int>(nullptr, 1U);
        static_cast<void>(invalid);
      },
      "");
}
#endif

}  // namespace
