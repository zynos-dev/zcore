/**************************************************************************/
/*  instant_test.cpp                                                      */
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
 * @file tests/instant_test.cpp
 * @brief Unit tests for deterministic `Instant` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/instant.hpp>

#include <gtest/gtest.h>

#include <limits>
#include <type_traits>

namespace {

static_assert(sizeof(zcore::Instant) == sizeof(zcore::i64));
static_assert(std::is_trivially_copyable_v<zcore::Instant>);

TEST(InstantTest, DefaultConstructedIsZero) {
  const zcore::Instant value;
  EXPECT_TRUE(value.IsZero());
  EXPECT_EQ(value.AsNanoseconds(), 0LL);
}

TEST(InstantTest, RawFactoryAndAccessAreDeterministic) {
  const zcore::Instant value = zcore::Instant::FromNanoseconds(12345);
  EXPECT_EQ(value.AsNanoseconds(), 12345LL);
  EXPECT_EQ(static_cast<zcore::i64>(value), 12345LL);
}

TEST(InstantTest, ComparisonIsPointOrdered) {
  const zcore::Instant a = zcore::Instant::FromNanoseconds(100);
  const zcore::Instant b = zcore::Instant::FromNanoseconds(200);
  EXPECT_LT(a, b);
  EXPECT_EQ((b - a).AsNanoseconds(), 100LL);
}

TEST(InstantTest, PointShiftWithDurationIsSaturating) {
  const zcore::Instant max = zcore::Instant::Max();
  const zcore::Instant min = zcore::Instant::Min();
  const zcore::Duration one = zcore::Duration::FromNanoseconds(1);

  EXPECT_EQ((max + one).AsNanoseconds(), std::numeric_limits<zcore::i64>::max());
  EXPECT_EQ((min - one).AsNanoseconds(), std::numeric_limits<zcore::i64>::min());
}

TEST(InstantTest, DifferenceReturnsDurationWithSaturation) {
  const zcore::Instant max = zcore::Instant::Max();
  const zcore::Instant min = zcore::Instant::Min();
  const zcore::Duration maxDuration = zcore::Duration::Max();
  const zcore::Duration minDuration = zcore::Duration::Min();

  EXPECT_EQ((max - min).AsNanoseconds(), maxDuration.AsNanoseconds());
  EXPECT_EQ((min - max).AsNanoseconds(), minDuration.AsNanoseconds());
}

TEST(InstantTest, CompoundPointShiftMatchesBinaryOperators) {
  zcore::Instant value = zcore::Instant::FromNanoseconds(5000);
  const zcore::Duration delta = zcore::Duration::FromNanoseconds(250);
  value += delta;
  EXPECT_EQ(value.AsNanoseconds(), 5250LL);
  value -= delta;
  EXPECT_EQ(value.AsNanoseconds(), 5000LL);
}

}  // namespace
