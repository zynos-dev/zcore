/**************************************************************************/
/*  math/vec2.hpp                                                         */
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
 * @file include/zcore/math/vec2.hpp
 * @brief Deterministic 2D vector primitive.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/vec2.hpp>
 * const zcore::Vec2 sum = zcore::Vec2(1.0F, 2.0F) + zcore::Vec2(3.0F, 4.0F);
 * @endcode
 */

#pragma once

#include <compare>

namespace zcore {

/**
 * @brief 2D vector with value semantics and no hidden allocation.
 */
struct [[nodiscard("Vec2 must be handled explicitly.")]] Vec2 final {
    using Scalar = float;

    Scalar x;
    Scalar y;

    /// @brief Constructs zero vector.
    constexpr Vec2() noexcept : x(0.0F), y(0.0F)
    {
    }

    /**
   * @brief Constructs vector from components.
   * @param xValue X component.
   * @param yValue Y component.
   */
    constexpr Vec2(Scalar xValue, Scalar yValue) noexcept : x(xValue), y(yValue)
    {
    }

    /// @brief Returns zero vector.
    [[nodiscard]] static constexpr Vec2 Zero() noexcept
    {
        return Vec2(0.0F, 0.0F);
    }

    /// @brief Returns all-ones vector.
    [[nodiscard]] static constexpr Vec2 One() noexcept
    {
        return Vec2(1.0F, 1.0F);
    }

    /// @brief Returns unit x-axis vector.
    [[nodiscard]] static constexpr Vec2 UnitX() noexcept
    {
        return Vec2(1.0F, 0.0F);
    }

    /// @brief Returns unit y-axis vector.
    [[nodiscard]] static constexpr Vec2 UnitY() noexcept
    {
        return Vec2(0.0F, 1.0F);
    }

    /// @brief Returns squared vector length.
    [[nodiscard]] constexpr Scalar LengthSquared() const noexcept
    {
        return Dot(*this);
    }

    /// @brief Returns dot product with another vector.
    [[nodiscard]] constexpr Scalar Dot(Vec2 other) const noexcept
    {
        return (x * other.x) + (y * other.y);
    }

    /// @brief Returns component-wise multiplication result.
    [[nodiscard]] constexpr Vec2 Hadamard(Vec2 other) const noexcept
    {
        return Vec2(x * other.x, y * other.y);
    }

    constexpr Vec2& operator+=(Vec2 other) noexcept
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    constexpr Vec2& operator-=(Vec2 other) noexcept
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    constexpr Vec2& operator*=(Scalar scalar) noexcept
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    constexpr Vec2& operator/=(Scalar scalar) noexcept
    {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    [[nodiscard]] constexpr Vec2 operator-() const noexcept
    {
        return Vec2(-x, -y);
    }

    [[nodiscard]] constexpr bool operator==(const Vec2&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const Vec2&) const noexcept = default;
};

[[nodiscard]] constexpr Vec2 operator+(Vec2 lhs, Vec2 rhs) noexcept
{
    lhs += rhs;
    return lhs;
}

[[nodiscard]] constexpr Vec2 operator-(Vec2 lhs, Vec2 rhs) noexcept
{
    lhs -= rhs;
    return lhs;
}

[[nodiscard]] constexpr Vec2 operator*(Vec2 vector, Vec2::Scalar scalar) noexcept
{
    vector *= scalar;
    return vector;
}

[[nodiscard]] constexpr Vec2 operator*(Vec2::Scalar scalar, Vec2 vector) noexcept
{
    vector *= scalar;
    return vector;
}

[[nodiscard]] constexpr Vec2 operator/(Vec2 vector, Vec2::Scalar scalar) noexcept
{
    vector /= scalar;
    return vector;
}

} // namespace zcore
