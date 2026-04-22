/**************************************************************************/
/*  duration.hpp                                                          */
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
 * @file include/zcore/time/duration.hpp
 * @brief Deterministic signed time-span value in nanoseconds.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/duration.hpp>
 * const zcore::Duration tick = zcore::Duration::FromMilliseconds(16);
 * @endcode
 */

#pragma once

#include <compare>
#include <limits>
#include <zcore/foundation.hpp>

namespace zcore {

/**
 * @brief Signed time span with nanosecond precision.
 *
 * Arithmetic and unit factory conversions are saturating to avoid overflow UB.
 */
class [[nodiscard("Duration must be handled explicitly.")]] Duration final {
public:
    using Rep = i64;

    /// @brief Nanoseconds per microsecond.
    static constexpr Rep kNanosecondsPerMicrosecond = 1'000LL;
    /// @brief Nanoseconds per millisecond.
    static constexpr Rep kNanosecondsPerMillisecond = 1'000'000LL;
    /// @brief Nanoseconds per second.
    static constexpr Rep kNanosecondsPerSecond = 1'000'000'000LL;
    /// @brief Nanoseconds per minute.
    static constexpr Rep kNanosecondsPerMinute = 60LL * kNanosecondsPerSecond;
    /// @brief Nanoseconds per hour.
    static constexpr Rep kNanosecondsPerHour = 60LL * kNanosecondsPerMinute;

    /// @brief Constructs zero duration.
    constexpr Duration() noexcept : Nanoseconds_(0LL)
    {
    }

    /**
   * @brief Constructs duration from raw nanoseconds.
   * @param nanoseconds Raw signed nanosecond value.
   */
    constexpr explicit Duration(Rep nanoseconds) noexcept : Nanoseconds_(nanoseconds)
    {
    }

    constexpr Duration(const Duration&) noexcept = default;
    constexpr Duration& operator=(const Duration&) noexcept = default;
    constexpr Duration(Duration&&) noexcept = default;
    constexpr Duration& operator=(Duration&&) noexcept = default;
    ~Duration() = default;

    /// @brief Returns zero duration.
    [[nodiscard]] static constexpr Duration Zero() noexcept
    {
        return Duration(0LL);
    }

    /// @brief Returns minimum representable duration.
    [[nodiscard]] static constexpr Duration Min() noexcept
    {
        return Duration(std::numeric_limits<Rep>::min());
    }

    /// @brief Returns maximum representable duration.
    [[nodiscard]] static constexpr Duration Max() noexcept
    {
        return Duration(std::numeric_limits<Rep>::max());
    }

    /// @brief Constructs from nanoseconds.
    [[nodiscard]] static constexpr Duration FromNanoseconds(Rep value) noexcept
    {
        return Duration(value);
    }

    /// @brief Constructs from microseconds (saturating).
    [[nodiscard]] static constexpr Duration FromMicroseconds(Rep value) noexcept
    {
        return Duration(SaturatingScale(value, kNanosecondsPerMicrosecond));
    }

    /// @brief Constructs from milliseconds (saturating).
    [[nodiscard]] static constexpr Duration FromMilliseconds(Rep value) noexcept
    {
        return Duration(SaturatingScale(value, kNanosecondsPerMillisecond));
    }

    /// @brief Constructs from seconds (saturating).
    [[nodiscard]] static constexpr Duration FromSeconds(Rep value) noexcept
    {
        return Duration(SaturatingScale(value, kNanosecondsPerSecond));
    }

    /// @brief Constructs from minutes (saturating).
    [[nodiscard]] static constexpr Duration FromMinutes(Rep value) noexcept
    {
        return Duration(SaturatingScale(value, kNanosecondsPerMinute));
    }

    /// @brief Constructs from hours (saturating).
    [[nodiscard]] static constexpr Duration FromHours(Rep value) noexcept
    {
        return Duration(SaturatingScale(value, kNanosecondsPerHour));
    }

    /// @brief Returns duration as raw nanoseconds.
    [[nodiscard]] constexpr Rep AsNanoseconds() const noexcept
    {
        return Nanoseconds_;
    }

    /// @brief Returns duration as microseconds (truncating toward zero).
    [[nodiscard]] constexpr Rep AsMicroseconds() const noexcept
    {
        return Nanoseconds_ / kNanosecondsPerMicrosecond;
    }

    /// @brief Returns duration as milliseconds (truncating toward zero).
    [[nodiscard]] constexpr Rep AsMilliseconds() const noexcept
    {
        return Nanoseconds_ / kNanosecondsPerMillisecond;
    }

    /// @brief Returns duration as seconds (truncating toward zero).
    [[nodiscard]] constexpr Rep AsSeconds() const noexcept
    {
        return Nanoseconds_ / kNanosecondsPerSecond;
    }

    /// @brief Returns duration as minutes (truncating toward zero).
    [[nodiscard]] constexpr Rep AsMinutes() const noexcept
    {
        return Nanoseconds_ / kNanosecondsPerMinute;
    }

    /// @brief Returns duration as hours (truncating toward zero).
    [[nodiscard]] constexpr Rep AsHours() const noexcept
    {
        return Nanoseconds_ / kNanosecondsPerHour;
    }

    /// @brief Returns `true` when duration equals zero.
    [[nodiscard]] constexpr bool IsZero() const noexcept
    {
        return Nanoseconds_ == 0LL;
    }

    /// @brief Returns `true` when duration is positive.
    [[nodiscard]] constexpr bool IsPositive() const noexcept
    {
        return Nanoseconds_ > 0LL;
    }

    /// @brief Returns `true` when duration is negative.
    [[nodiscard]] constexpr bool IsNegative() const noexcept
    {
        return Nanoseconds_ < 0LL;
    }

    /// @brief Returns absolute value (saturating for `Rep::min`).
    [[nodiscard]] constexpr Duration Abs() const noexcept
    {
        if (Nanoseconds_ == std::numeric_limits<Rep>::min()) {
            return Max();
        }
        return Duration(Nanoseconds_ < 0LL ? -Nanoseconds_ : Nanoseconds_);
    }

    [[nodiscard]] constexpr explicit operator Rep() const noexcept
    {
        return Nanoseconds_;
    }

    [[nodiscard]] constexpr Duration operator-() const noexcept
    {
        if (Nanoseconds_ == std::numeric_limits<Rep>::min()) {
            return Max();
        }
        return Duration(-Nanoseconds_);
    }

    constexpr Duration& operator+=(Duration other) noexcept
    {
        Nanoseconds_ = SaturatingAdd(Nanoseconds_, other.Nanoseconds_);
        return *this;
    }

    constexpr Duration& operator-=(Duration other) noexcept
    {
        Nanoseconds_ = SaturatingSub(Nanoseconds_, other.Nanoseconds_);
        return *this;
    }

    [[nodiscard]] constexpr bool operator==(const Duration&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const Duration&) const noexcept = default;

private:
    [[nodiscard]] static constexpr Rep SaturatingScale(Rep value, Rep factor) noexcept
    {
        const Rep max = std::numeric_limits<Rep>::max();
        const Rep min = std::numeric_limits<Rep>::min();
        if (value > 0LL && value > (max / factor)) {
            return max;
        }
        if (value < 0LL && value < (min / factor)) {
            return min;
        }
        return value * factor;
    }

    [[nodiscard]] static constexpr Rep SaturatingAdd(Rep lhs, Rep rhs) noexcept
    {
        const Rep max = std::numeric_limits<Rep>::max();
        const Rep min = std::numeric_limits<Rep>::min();
        if (rhs > 0LL && lhs > (max - rhs)) {
            return max;
        }
        if (rhs < 0LL && lhs < (min - rhs)) {
            return min;
        }
        return lhs + rhs;
    }

    [[nodiscard]] static constexpr Rep SaturatingSub(Rep lhs, Rep rhs) noexcept
    {
        const Rep max = std::numeric_limits<Rep>::max();
        const Rep min = std::numeric_limits<Rep>::min();
        if (rhs > 0LL && lhs < (min + rhs)) {
            return min;
        }
        if (rhs < 0LL && lhs > (max + rhs)) {
            return max;
        }
        return lhs - rhs;
    }

    Rep Nanoseconds_;
};

[[nodiscard]] constexpr Duration operator+(Duration lhs, Duration rhs) noexcept
{
    lhs += rhs;
    return lhs;
}

[[nodiscard]] constexpr Duration operator-(Duration lhs, Duration rhs) noexcept
{
    lhs -= rhs;
    return lhs;
}

} // namespace zcore
