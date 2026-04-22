/**************************************************************************/
/*  timestamp.hpp                                                         */
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
 * @file include/zcore/time/timestamp.hpp
 * @brief Deterministic unsigned nanosecond timestamp value.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/timestamp.hpp>
 * const zcore::Timestamp startedAt = zcore::Timestamp::FromNanoseconds(1000);
 * @endcode
 */

#pragma once

#include <compare>
#include <limits>
#include <zcore/time/duration.hpp>

namespace zcore {

/**
 * @brief Unsigned absolute timestamp represented as nanoseconds.
 *
 * Timestamp shifts with `Duration` are saturating to range `[0, Max()]`.
 */
class [[nodiscard("Timestamp must be handled explicitly.")]] Timestamp final {
public:
    using Rep = u64;

    /// @brief Constructs zero timestamp.
    constexpr Timestamp() noexcept : Nanoseconds_(0ULL)
    {
    }

    /**
   * @brief Constructs timestamp from raw nanoseconds.
   * @param nanoseconds Raw unsigned nanosecond timestamp.
   */
    constexpr explicit Timestamp(Rep nanoseconds) noexcept : Nanoseconds_(nanoseconds)
    {
    }

    constexpr Timestamp(const Timestamp&) noexcept = default;
    constexpr Timestamp& operator=(const Timestamp&) noexcept = default;
    constexpr Timestamp(Timestamp&&) noexcept = default;
    constexpr Timestamp& operator=(Timestamp&&) noexcept = default;
    ~Timestamp() = default;

    /// @brief Returns zero timestamp.
    [[nodiscard]] static constexpr Timestamp Zero() noexcept
    {
        return Timestamp(0ULL);
    }

    /// @brief Returns maximum representable timestamp.
    [[nodiscard]] static constexpr Timestamp Max() noexcept
    {
        return Timestamp(std::numeric_limits<Rep>::max());
    }

    /// @brief Constructs from raw nanoseconds.
    [[nodiscard]] static constexpr Timestamp FromNanoseconds(Rep value) noexcept
    {
        return Timestamp(value);
    }

    /// @brief Returns raw nanosecond timestamp.
    [[nodiscard]] constexpr Rep AsNanoseconds() const noexcept
    {
        return Nanoseconds_;
    }

    /// @brief Returns `true` when timestamp equals zero.
    [[nodiscard]] constexpr bool IsZero() const noexcept
    {
        return Nanoseconds_ == 0ULL;
    }

    [[nodiscard]] constexpr explicit operator Rep() const noexcept
    {
        return Nanoseconds_;
    }

    /// @brief Saturating timestamp shift by signed duration.
    constexpr Timestamp& operator+=(Duration delta) noexcept
    {
        Nanoseconds_ = ShiftByDuration(Nanoseconds_, delta);
        return *this;
    }

    /// @brief Saturating timestamp shift by negative signed duration.
    constexpr Timestamp& operator-=(Duration delta) noexcept
    {
        Nanoseconds_ = ShiftByDuration(Nanoseconds_, -delta);
        return *this;
    }

    [[nodiscard]] constexpr bool operator==(const Timestamp&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const Timestamp&) const noexcept = default;

private:
    [[nodiscard]] static constexpr Rep SaturatingAdd(Rep lhs, Rep rhs) noexcept
    {
        const Rep max = std::numeric_limits<Rep>::max();
        if (lhs > (max - rhs)) {
            return max;
        }
        return lhs + rhs;
    }

    [[nodiscard]] static constexpr Rep SaturatingSub(Rep lhs, Rep rhs) noexcept
    {
        if (lhs < rhs) {
            return 0ULL;
        }
        return lhs - rhs;
    }

    [[nodiscard]] static constexpr Rep ShiftByDuration(Rep base, Duration delta) noexcept
    {
        const Duration::Rep signedDelta = delta.AsNanoseconds();
        if (signedDelta >= 0LL) {
            return SaturatingAdd(base, static_cast<Rep>(signedDelta));
        }

        const Rep magnitude = signedDelta == std::numeric_limits<Duration::Rep>::min()
                                      ? (Rep{1} << (std::numeric_limits<Rep>::digits - 1U))
                                      : static_cast<Rep>(-signedDelta);
        return SaturatingSub(base, magnitude);
    }

    Rep Nanoseconds_;
};

[[nodiscard]] constexpr Timestamp operator+(Timestamp point, Duration delta) noexcept
{
    point += delta;
    return point;
}

[[nodiscard]] constexpr Timestamp operator-(Timestamp point, Duration delta) noexcept
{
    point -= delta;
    return point;
}

[[nodiscard]] constexpr Duration operator-(Timestamp lhs, Timestamp rhs) noexcept
{
    if (lhs == rhs) {
        return Duration::Zero();
    }

    const u64 maxPositive = static_cast<u64>(std::numeric_limits<i64>::max());
    const u64 maxNegativeMagnitude = maxPositive + 1ULL;

    if (lhs > rhs) {
        const u64 diff = lhs.AsNanoseconds() - rhs.AsNanoseconds();
        if (diff > maxPositive) {
            return Duration::Max();
        }
        return Duration::FromNanoseconds(static_cast<i64>(diff));
    }

    const u64 diff = rhs.AsNanoseconds() - lhs.AsNanoseconds();
    if (diff > maxNegativeMagnitude) {
        return Duration::Min();
    }
    if (diff == maxNegativeMagnitude) {
        return Duration::FromNanoseconds(std::numeric_limits<i64>::min());
    }
    return Duration::FromNanoseconds(-static_cast<i64>(diff));
}

} // namespace zcore
