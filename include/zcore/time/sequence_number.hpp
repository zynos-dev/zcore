/**************************************************************************/
/*  sequence_number.hpp                                                   */
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
 * @file include/zcore/time/sequence_number.hpp
 * @brief Wrap-aware monotonically advancing sequence token.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/sequence_number.hpp>
 * zcore::SequenceNumber seq;
 * seq.Increment();
 * @endcode
 */

#pragma once

#include <compare>
#include <limits>
#include <zcore/foundation.hpp>

namespace zcore {

/**
 * @brief Unsigned 64-bit sequence token with wrap-aware ordering helpers.
 */
class [[nodiscard("SequenceNumber must be handled explicitly.")]] SequenceNumber final {
public:
    using Rep = u64;

    /// @brief Half-range threshold used for wrap-aware ordering checks.
    static constexpr Rep kHalfRange = Rep{1} << (std::numeric_limits<Rep>::digits - 1U);

    /// @brief Constructs zero sequence number.
    constexpr SequenceNumber() noexcept : Value_(0ULL)
    {
    }

    /**
   * @brief Constructs sequence number from raw value.
   * @param value Raw sequence token.
   */
    constexpr explicit SequenceNumber(Rep value) noexcept : Value_(value)
    {
    }

    constexpr SequenceNumber(const SequenceNumber&) noexcept = default;
    constexpr SequenceNumber& operator=(const SequenceNumber&) noexcept = default;
    constexpr SequenceNumber(SequenceNumber&&) noexcept = default;
    constexpr SequenceNumber& operator=(SequenceNumber&&) noexcept = default;
    ~SequenceNumber() = default;

    /// @brief Returns zero sequence value.
    [[nodiscard]] static constexpr SequenceNumber Zero() noexcept
    {
        return SequenceNumber(0ULL);
    }

    /// @brief Returns maximum representable sequence value.
    [[nodiscard]] static constexpr SequenceNumber Max() noexcept
    {
        return SequenceNumber(std::numeric_limits<Rep>::max());
    }

    /// @brief Constructs from raw value.
    [[nodiscard]] static constexpr SequenceNumber FromRawUnchecked(Rep value) noexcept
    {
        return SequenceNumber(value);
    }

    /// @brief Returns raw sequence token.
    [[nodiscard]] constexpr Rep Raw() const noexcept
    {
        return Value_;
    }

    /// @brief Returns `true` when raw value is zero.
    [[nodiscard]] constexpr bool IsZero() const noexcept
    {
        return Value_ == 0ULL;
    }

    /// @brief Returns next sequence number (wraps on overflow).
    [[nodiscard]] constexpr SequenceNumber Next() const noexcept
    {
        return SequenceNumber(Value_ + 1ULL);
    }

    /// @brief Returns previous sequence number (wraps on underflow).
    [[nodiscard]] constexpr SequenceNumber Previous() const noexcept
    {
        return SequenceNumber(Value_ - 1ULL);
    }

    /// @brief Increments sequence number (wraps on overflow).
    constexpr void Increment() noexcept
    {
        Value_ += 1ULL;
    }

    /// @brief Decrements sequence number (wraps on underflow).
    constexpr void Decrement() noexcept
    {
        Value_ -= 1ULL;
    }

    /// @brief Returns sequence number advanced by raw delta (wrap-aware add).
    [[nodiscard]] constexpr SequenceNumber AdvancedBy(Rep delta) const noexcept
    {
        return SequenceNumber(Value_ + delta);
    }

    /// @brief Advances sequence number by raw delta (wrap-aware add).
    constexpr void Advance(Rep delta) noexcept
    {
        Value_ += delta;
    }

    /**
   * @brief Returns `true` when this sequence is newer than `other`.
   *
   * Uses half-range comparison suitable for modulo sequence spaces.
   */
    [[nodiscard]] constexpr bool IsNewerThan(SequenceNumber other) const noexcept
    {
        const Rep distance = Value_ - other.Value_;
        return distance != 0ULL && distance < kHalfRange;
    }

    /**
   * @brief Returns `true` when this sequence is older than `other`.
   *
   * Uses half-range comparison suitable for modulo sequence spaces.
   */
    [[nodiscard]] constexpr bool IsOlderThan(SequenceNumber other) const noexcept
    {
        return other.IsNewerThan(*this);
    }

    /// @brief Returns forward modular distance from this sequence to `other`.
    [[nodiscard]] constexpr Rep ForwardDistanceTo(SequenceNumber other) const noexcept
    {
        return other.Value_ - Value_;
    }

    /// @brief Returns backward modular distance from this sequence to `other`.
    [[nodiscard]] constexpr Rep BackwardDistanceTo(SequenceNumber other) const noexcept
    {
        return Value_ - other.Value_;
    }

    [[nodiscard]] constexpr explicit operator Rep() const noexcept
    {
        return Value_;
    }

    [[nodiscard]] constexpr bool operator==(const SequenceNumber&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const SequenceNumber&) const noexcept = default;

private:
    Rep Value_;
};

} // namespace zcore
