/**************************************************************************/
/*  duration_test.cpp                                                     */
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
 * @file tests/duration_test.cpp
 * @brief Unit tests for deterministic `Duration` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/duration.hpp>

#include <gtest/gtest.h>

#include <limits>
#include <type_traits>

namespace {

static_assert(sizeof(zcore::Duration) == sizeof(zcore::i64));
static_assert(std::is_trivially_copyable_v<zcore::Duration>);

TEST(DurationTest, DefaultConstructedIsZero) {
  const zcore::Duration value;
  EXPECT_TRUE(value.IsZero());
  EXPECT_EQ(value.AsNanoseconds(), 0LL);
}

TEST(DurationTest, UnitFactoriesConvertToNanoseconds) {
  EXPECT_EQ(zcore::Duration::FromNanoseconds(7).AsNanoseconds(), 7LL);
  EXPECT_EQ(zcore::Duration::FromMicroseconds(3).AsNanoseconds(), 3'000LL);
  EXPECT_EQ(zcore::Duration::FromMilliseconds(2).AsNanoseconds(), 2'000'000LL);
  EXPECT_EQ(zcore::Duration::FromSeconds(4).AsNanoseconds(), 4'000'000'000LL);
  EXPECT_EQ(zcore::Duration::FromMinutes(1).AsNanoseconds(), 60'000'000'000LL);
  EXPECT_EQ(zcore::Duration::FromHours(2).AsNanoseconds(), 7'200'000'000'000LL);
}

TEST(DurationTest, UnitAccessorsTruncateTowardZero) {
  const zcore::Duration positive = zcore::Duration::FromNanoseconds(1'999'999LL);
  EXPECT_EQ(positive.AsMicroseconds(), 1'999LL);
  EXPECT_EQ(positive.AsMilliseconds(), 1LL);

  const zcore::Duration negative = zcore::Duration::FromNanoseconds(-1'999'999LL);
  EXPECT_EQ(negative.AsMicroseconds(), -1'999LL);
  EXPECT_EQ(negative.AsMilliseconds(), -1LL);
}

TEST(DurationTest, ComparisonAndSignQueriesAreDeterministic) {
  const zcore::Duration a = zcore::Duration::FromMilliseconds(5);
  const zcore::Duration b = zcore::Duration::FromMilliseconds(9);
  const zcore::Duration c = zcore::Duration::FromMilliseconds(-3);

  EXPECT_LT(a, b);
  EXPECT_TRUE(b.IsPositive());
  EXPECT_TRUE(c.IsNegative());
  EXPECT_FALSE(c.IsPositive());
}

TEST(DurationTest, AdditionAndSubtractionAreSaturating) {
  const zcore::Duration max = zcore::Duration::Max();
  const zcore::Duration min = zcore::Duration::Min();
  const zcore::Duration one = zcore::Duration::FromNanoseconds(1);

  EXPECT_EQ((max + one).AsNanoseconds(), std::numeric_limits<zcore::i64>::max());
  EXPECT_EQ((min - one).AsNanoseconds(), std::numeric_limits<zcore::i64>::min());

  zcore::Duration value = zcore::Duration::FromMilliseconds(10);
  value += zcore::Duration::FromMilliseconds(5);
  EXPECT_EQ(value.AsMilliseconds(), 15LL);
  value -= zcore::Duration::FromMilliseconds(3);
  EXPECT_EQ(value.AsMilliseconds(), 12LL);
}

TEST(DurationTest, NegationAndAbsoluteAreSaturating) {
  const zcore::Duration positive = zcore::Duration::FromSeconds(3);
  const zcore::Duration negative = -positive;
  EXPECT_EQ(negative.AsSeconds(), -3LL);
  EXPECT_EQ(negative.Abs().AsSeconds(), 3LL);

  const zcore::Duration min = zcore::Duration::Min();
  EXPECT_EQ((-min).AsNanoseconds(), std::numeric_limits<zcore::i64>::max());
  EXPECT_EQ(min.Abs().AsNanoseconds(), std::numeric_limits<zcore::i64>::max());
}

TEST(DurationTest, FactoryScalingSaturatesOnOverflow) {
  const zcore::i64 max = std::numeric_limits<zcore::i64>::max();
  const zcore::i64 min = std::numeric_limits<zcore::i64>::min();

  EXPECT_EQ(zcore::Duration::FromHours(max).AsNanoseconds(), max);
  EXPECT_EQ(zcore::Duration::FromHours(min).AsNanoseconds(), min);
}

}  // namespace
