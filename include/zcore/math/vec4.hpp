/**************************************************************************/
/*  math/vec4.hpp                                                         */
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
 * @file include/zcore/math/vec4.hpp
 * @brief Deterministic 4D vector primitive.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/vec4.hpp>
 * const zcore::Vec4 sum = zcore::Vec4::One() + zcore::Vec4(1.0F, 2.0F, 3.0F, 4.0F);
 * @endcode
 */

#pragma once

#include <compare>

namespace zcore {

/**
 * @brief 4D vector with value semantics and no hidden allocation.
 */
struct [[nodiscard("Vec4 must be handled explicitly.")]] Vec4 final {
    using Scalar = float;

    Scalar x;
    Scalar y;
    Scalar z;
    Scalar w;

    /// @brief Constructs zero vector.
    constexpr Vec4() noexcept : x(0.0F), y(0.0F), z(0.0F), w(0.0F)
    {
    }

    /**
   * @brief Constructs vector from components.
   * @param xValue X component.
   * @param yValue Y component.
   * @param zValue Z component.
   * @param wValue W component.
   */
    constexpr Vec4(Scalar xValue, Scalar yValue, Scalar zValue, Scalar wValue) noexcept : x(xValue), y(yValue), z(zValue), w(wValue)
    {
    }

    /// @brief Returns zero vector.
    [[nodiscard]] static constexpr Vec4 Zero() noexcept
    {
        return Vec4(0.0F, 0.0F, 0.0F, 0.0F);
    }

    /// @brief Returns all-ones vector.
    [[nodiscard]] static constexpr Vec4 One() noexcept
    {
        return Vec4(1.0F, 1.0F, 1.0F, 1.0F);
    }

    /// @brief Returns squared vector length.
    [[nodiscard]] constexpr Scalar LengthSquared() const noexcept
    {
        return Dot(*this);
    }

    /// @brief Returns dot product with another vector.
    [[nodiscard]] constexpr Scalar Dot(Vec4 other) const noexcept
    {
        return (x * other.x) + (y * other.y) + (z * other.z) + (w * other.w);
    }

    /// @brief Returns component-wise multiplication result.
    [[nodiscard]] constexpr Vec4 Hadamard(Vec4 other) const noexcept
    {
        return Vec4(x * other.x, y * other.y, z * other.z, w * other.w);
    }

    constexpr Vec4& operator+=(Vec4 other) noexcept
    {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    constexpr Vec4& operator-=(Vec4 other) noexcept
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    constexpr Vec4& operator*=(Scalar scalar) noexcept
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        w *= scalar;
        return *this;
    }

    constexpr Vec4& operator/=(Scalar scalar) noexcept
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        w /= scalar;
        return *this;
    }

    [[nodiscard]] constexpr Vec4 operator-() const noexcept
    {
        return Vec4(-x, -y, -z, -w);
    }

    [[nodiscard]] constexpr bool operator==(const Vec4&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const Vec4&) const noexcept = default;
};

[[nodiscard]] constexpr Vec4 operator+(Vec4 lhs, Vec4 rhs) noexcept
{
    lhs += rhs;
    return lhs;
}

[[nodiscard]] constexpr Vec4 operator-(Vec4 lhs, Vec4 rhs) noexcept
{
    lhs -= rhs;
    return lhs;
}

[[nodiscard]] constexpr Vec4 operator*(Vec4 vector, Vec4::Scalar scalar) noexcept
{
    vector *= scalar;
    return vector;
}

[[nodiscard]] constexpr Vec4 operator*(Vec4::Scalar scalar, Vec4 vector) noexcept
{
    vector *= scalar;
    return vector;
}

[[nodiscard]] constexpr Vec4 operator/(Vec4 vector, Vec4::Scalar scalar) noexcept
{
    vector /= scalar;
    return vector;
}

} // namespace zcore
