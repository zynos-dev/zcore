/**************************************************************************/
/*  sequence_number_test.cpp                                              */
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
 * @file tests/sequence_number_test.cpp
 * @brief Unit tests for wrap-aware `SequenceNumber` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/sequence_number.hpp>

#include <gtest/gtest.h>

#include <limits>
#include <type_traits>

namespace {

static_assert(sizeof(zcore::SequenceNumber) == sizeof(zcore::u64));
static_assert(std::is_trivially_copyable_v<zcore::SequenceNumber>);

TEST(SequenceNumberTest, DefaultConstructedIsZero) {
  const zcore::SequenceNumber value;
  EXPECT_TRUE(value.IsZero());
  EXPECT_EQ(value.Raw(), 0ULL);
}

TEST(SequenceNumberTest, RawFactoryAndAccessAreDeterministic) {
  const zcore::SequenceNumber value = zcore::SequenceNumber::FromRawUnchecked(42ULL);
  EXPECT_EQ(value.Raw(), 42ULL);
  EXPECT_EQ(static_cast<zcore::u64>(value), 42ULL);
}

TEST(SequenceNumberTest, IncrementNextAndAdvanceWrapAsModuloSpace) {
  zcore::SequenceNumber value = zcore::SequenceNumber::Max();
  value.Increment();
  EXPECT_TRUE(value.IsZero());

  const zcore::SequenceNumber next = value.Next();
  EXPECT_EQ(next.Raw(), 1ULL);
  EXPECT_EQ(next.Previous().Raw(), 0ULL);

  value.Advance(10ULL);
  EXPECT_EQ(value.Raw(), 10ULL);
}

TEST(SequenceNumberTest, WrapAwareOrderingHandlesNearWrapBoundary) {
  const zcore::SequenceNumber wrapZero = zcore::SequenceNumber::Zero();
  const zcore::SequenceNumber max = zcore::SequenceNumber::Max();
  const zcore::SequenceNumber ten = zcore::SequenceNumber::FromRawUnchecked(10ULL);
  const zcore::SequenceNumber nine = zcore::SequenceNumber::FromRawUnchecked(9ULL);

  EXPECT_TRUE(wrapZero.IsNewerThan(max));
  EXPECT_TRUE(max.IsOlderThan(wrapZero));
  EXPECT_TRUE(ten.IsNewerThan(nine));
  EXPECT_TRUE(nine.IsOlderThan(ten));
}

TEST(SequenceNumberTest, HalfRangeComparisonIsIntentionallyAmbiguous) {
  const zcore::u64 half = zcore::SequenceNumber::kHalfRange;
  const zcore::SequenceNumber a = zcore::SequenceNumber::FromRawUnchecked(1ULL);
  const zcore::SequenceNumber b = zcore::SequenceNumber::FromRawUnchecked(1ULL + half);

  EXPECT_FALSE(a.IsNewerThan(b));
  EXPECT_FALSE(b.IsNewerThan(a));
  EXPECT_FALSE(a.IsOlderThan(b));
  EXPECT_FALSE(b.IsOlderThan(a));
}

TEST(SequenceNumberTest, ForwardAndBackwardDistanceAreModuloConsistent) {
  const zcore::SequenceNumber max = zcore::SequenceNumber::Max();
  const zcore::SequenceNumber zero = zcore::SequenceNumber::Zero();

  EXPECT_EQ(max.ForwardDistanceTo(zero), 1ULL);
  EXPECT_EQ(zero.BackwardDistanceTo(max), 1ULL);
  EXPECT_EQ(zero.ForwardDistanceTo(max), std::numeric_limits<zcore::u64>::max());
}

}  // namespace

