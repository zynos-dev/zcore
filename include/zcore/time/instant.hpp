/**************************************************************************/
/*  instant.hpp                                                           */
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
 * @file include/zcore/time/instant.hpp
 * @brief Deterministic absolute time point in nanoseconds.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/instant.hpp>
 * const zcore::Instant deadline = zcore::Instant::FromNanoseconds(1000);
 * @endcode
 */

#pragma once

#include <compare>
#include <limits>
#include <zcore/time/duration.hpp>

namespace zcore {

/**
 * @brief Absolute time point represented as signed nanoseconds.
 *
 * Point arithmetic is saturating to avoid overflow UB.
 */
class [[nodiscard("Instant must be handled explicitly.")]] Instant final {
public:
    using Rep = i64;

    /// @brief Constructs zero instant.
    constexpr Instant() noexcept : Nanoseconds_(0LL)
    {
    }

    /**
   * @brief Constructs instant from raw nanosecond point.
   * @param nanoseconds Raw absolute nanosecond value.
   */
    constexpr explicit Instant(Rep nanoseconds) noexcept : Nanoseconds_(nanoseconds)
    {
    }

    constexpr Instant(const Instant&) noexcept = default;
    constexpr Instant& operator=(const Instant&) noexcept = default;
    constexpr Instant(Instant&&) noexcept = default;
    constexpr Instant& operator=(Instant&&) noexcept = default;
    ~Instant() = default;

    /// @brief Returns zero instant.
    [[nodiscard]] static constexpr Instant Zero() noexcept
    {
        return Instant(0LL);
    }

    /// @brief Returns minimum representable instant.
    [[nodiscard]] static constexpr Instant Min() noexcept
    {
        return Instant(std::numeric_limits<Rep>::min());
    }

    /// @brief Returns maximum representable instant.
    [[nodiscard]] static constexpr Instant Max() noexcept
    {
        return Instant(std::numeric_limits<Rep>::max());
    }

    /// @brief Constructs from raw nanoseconds.
    [[nodiscard]] static constexpr Instant FromNanoseconds(Rep value) noexcept
    {
        return Instant(value);
    }

    /// @brief Returns raw nanosecond point.
    [[nodiscard]] constexpr Rep AsNanoseconds() const noexcept
    {
        return Nanoseconds_;
    }

    /// @brief Returns `true` when instant equals zero point.
    [[nodiscard]] constexpr bool IsZero() const noexcept
    {
        return Nanoseconds_ == 0LL;
    }

    [[nodiscard]] constexpr explicit operator Rep() const noexcept
    {
        return Nanoseconds_;
    }

    /// @brief Saturating point shift forward by duration.
    constexpr Instant& operator+=(Duration delta) noexcept
    {
        Nanoseconds_ = SaturatingAdd(Nanoseconds_, delta.AsNanoseconds());
        return *this;
    }

    /// @brief Saturating point shift backward by duration.
    constexpr Instant& operator-=(Duration delta) noexcept
    {
        Nanoseconds_ = SaturatingSub(Nanoseconds_, delta.AsNanoseconds());
        return *this;
    }

    [[nodiscard]] constexpr bool operator==(const Instant&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const Instant&) const noexcept = default;

private:
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

[[nodiscard]] constexpr Instant operator+(Instant point, Duration delta) noexcept
{
    point += delta;
    return point;
}

[[nodiscard]] constexpr Instant operator-(Instant point, Duration delta) noexcept
{
    point -= delta;
    return point;
}

[[nodiscard]] constexpr Duration operator-(Instant lhs, Instant rhs) noexcept
{
    const i64 max = std::numeric_limits<i64>::max();
    const i64 min = std::numeric_limits<i64>::min();
    const i64 left = lhs.AsNanoseconds();
    const i64 right = rhs.AsNanoseconds();
    if (right > 0LL && left < (min + right)) {
        return Duration::Min();
    }
    if (right < 0LL && left > (max + right)) {
        return Duration::Max();
    }
    return Duration::FromNanoseconds(left - right);
}

} // namespace zcore
