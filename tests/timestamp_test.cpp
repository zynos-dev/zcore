/**************************************************************************/
/*  timestamp_test.cpp                                                    */
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
 * @file tests/timestamp_test.cpp
 * @brief Unit tests for deterministic `Timestamp` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/timestamp.hpp>

#include <gtest/gtest.h>

#include <limits>
#include <type_traits>

namespace {

static_assert(sizeof(zcore::Timestamp) == sizeof(zcore::u64));
static_assert(std::is_trivially_copyable_v<zcore::Timestamp>);

TEST(TimestampTest, DefaultConstructedIsZero) {
  const zcore::Timestamp value;
  EXPECT_TRUE(value.IsZero());
  EXPECT_EQ(value.AsNanoseconds(), 0ULL);
}

TEST(TimestampTest, RawFactoryAndAccessAreDeterministic) {
  const zcore::Timestamp value = zcore::Timestamp::FromNanoseconds(1234ULL);
  EXPECT_EQ(value.AsNanoseconds(), 1234ULL);
  EXPECT_EQ(static_cast<zcore::u64>(value), 1234ULL);
}

TEST(TimestampTest, ComparisonIsRawPointOrdered) {
  const zcore::Timestamp a = zcore::Timestamp::FromNanoseconds(100ULL);
  const zcore::Timestamp b = zcore::Timestamp::FromNanoseconds(200ULL);
  EXPECT_LT(a, b);
}

TEST(TimestampTest, PointShiftWithDurationIsSaturatingToUnsignedRange) {
  const zcore::Timestamp max = zcore::Timestamp::Max();
  const zcore::Timestamp zero = zcore::Timestamp::Zero();
  const zcore::Duration one = zcore::Duration::FromNanoseconds(1LL);

  EXPECT_EQ((max + one).AsNanoseconds(), std::numeric_limits<zcore::u64>::max());
  EXPECT_EQ((zero - one).AsNanoseconds(), 0ULL);
}

TEST(TimestampTest, DifferenceReturnsDurationWithSaturation) {
  const zcore::Timestamp max = zcore::Timestamp::Max();
  const zcore::Timestamp zero = zcore::Timestamp::Zero();
  const zcore::Timestamp mid = zcore::Timestamp::FromNanoseconds(200ULL);

  EXPECT_EQ((mid - zero).AsNanoseconds(), 200LL);
  EXPECT_EQ((zero - mid).AsNanoseconds(), -200LL);
  EXPECT_EQ((max - zero).AsNanoseconds(), zcore::Duration::Max().AsNanoseconds());
  EXPECT_EQ((zero - max).AsNanoseconds(), zcore::Duration::Min().AsNanoseconds());
}

TEST(TimestampTest, CompoundPointShiftMatchesBinaryOperators) {
  zcore::Timestamp value = zcore::Timestamp::FromNanoseconds(5000ULL);
  const zcore::Duration delta = zcore::Duration::FromNanoseconds(250LL);
  value += delta;
  EXPECT_EQ(value.AsNanoseconds(), 5250ULL);
  value -= delta;
  EXPECT_EQ(value.AsNanoseconds(), 5000ULL);
}

}  // namespace

