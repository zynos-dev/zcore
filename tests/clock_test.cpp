/**************************************************************************/
/*  clock_test.cpp                                                        */
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
 * @file tests/clock_test.cpp
 * @brief Unit tests for `Clock` interface contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/clock.hpp>

#include <gtest/gtest.h>

#include <limits>

namespace {

class ManualClock final : public zcore::Clock {
 public:
  explicit ManualClock(zcore::Instant initial, bool monotonic = true) noexcept
      : Current_(initial), Monotonic_(monotonic) {}

  [[nodiscard]] zcore::Instant Now() const noexcept override {
    return Current_;
  }

  [[nodiscard]] bool IsMonotonic() const noexcept override {
    return Monotonic_;
  }

  void Set(zcore::Instant value) noexcept {
    Current_ = value;
  }

  void Advance(zcore::Duration delta) noexcept {
    Current_ += delta;
  }

 private:
  zcore::Instant Current_;
  bool Monotonic_;
};

class DefaultFlagClock final : public zcore::Clock {
 public:
  explicit DefaultFlagClock(zcore::Instant current) noexcept : Current_(current) {}

  [[nodiscard]] zcore::Instant Now() const noexcept override {
    return Current_;
  }

 private:
  zcore::Instant Current_;
};

TEST(ClockTest, NowReturnsCurrentInstant) {
  const ManualClock clock(zcore::Instant::FromNanoseconds(1'000));
  EXPECT_EQ(clock.Now().AsNanoseconds(), 1'000LL);
}

TEST(ClockTest, ElapsedSinceUsesCurrentInstantDifference) {
  ManualClock clock(zcore::Instant::FromNanoseconds(1'500));
  const zcore::Duration elapsed = clock.ElapsedSince(zcore::Instant::FromNanoseconds(1'000));
  EXPECT_EQ(elapsed.AsNanoseconds(), 500LL);

  clock.Advance(zcore::Duration::FromNanoseconds(250));
  EXPECT_EQ(clock.ElapsedSince(zcore::Instant::FromNanoseconds(1'000)).AsNanoseconds(), 750LL);
}

TEST(ClockTest, ElapsedSinceDifferenceSaturatesThroughInstantContract) {
  const ManualClock clock(zcore::Instant::Max());
  const zcore::Duration elapsed = clock.ElapsedSince(zcore::Instant::Min());
  EXPECT_EQ(elapsed.AsNanoseconds(), std::numeric_limits<zcore::i64>::max());
}

TEST(ClockTest, IsMonotonicCanBeOverridden) {
  const ManualClock monotonicClock(zcore::Instant::Zero(), true);
  const ManualClock nonMonotonicClock(zcore::Instant::Zero(), false);

  EXPECT_TRUE(monotonicClock.IsMonotonic());
  EXPECT_FALSE(nonMonotonicClock.IsMonotonic());
}

TEST(ClockTest, IsMonotonicDefaultsToFalse) {
  const DefaultFlagClock clock(zcore::Instant::Zero());
  EXPECT_FALSE(clock.IsMonotonic());
}

}  // namespace
