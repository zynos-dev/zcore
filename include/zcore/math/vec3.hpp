/**************************************************************************/
/*  math/vec3.hpp                                                         */
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
 * @file include/zcore/math/vec3.hpp
 * @brief Deterministic 3D vector primitive.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/vec3.hpp>
 * const zcore::Vec3 cross = zcore::Vec3::UnitX().Cross(zcore::Vec3::UnitY());
 * @endcode
 */

#pragma once

#include <compare>

namespace zcore {

/**
 * @brief 3D vector with value semantics and no hidden allocation.
 */
struct [[nodiscard("Vec3 must be handled explicitly.")]] Vec3 final {
    using Scalar = float;

    Scalar x;
    Scalar y;
    Scalar z;

    /// @brief Constructs zero vector.
    constexpr Vec3() noexcept : x(0.0F), y(0.0F), z(0.0F)
    {
    }

    /**
   * @brief Constructs vector from components.
   * @param xValue X component.
   * @param yValue Y component.
   * @param zValue Z component.
   */
    constexpr Vec3(Scalar xValue, Scalar yValue, Scalar zValue) noexcept : x(xValue), y(yValue), z(zValue)
    {
    }

    /// @brief Returns zero vector.
    [[nodiscard]] static constexpr Vec3 Zero() noexcept
    {
        return Vec3(0.0F, 0.0F, 0.0F);
    }

    /// @brief Returns all-ones vector.
    [[nodiscard]] static constexpr Vec3 One() noexcept
    {
        return Vec3(1.0F, 1.0F, 1.0F);
    }

    /// @brief Returns unit x-axis vector.
    [[nodiscard]] static constexpr Vec3 UnitX() noexcept
    {
        return Vec3(1.0F, 0.0F, 0.0F);
    }

    /// @brief Returns unit y-axis vector.
    [[nodiscard]] static constexpr Vec3 UnitY() noexcept
    {
        return Vec3(0.0F, 1.0F, 0.0F);
    }

    /// @brief Returns unit z-axis vector.
    [[nodiscard]] static constexpr Vec3 UnitZ() noexcept
    {
        return Vec3(0.0F, 0.0F, 1.0F);
    }

    /// @brief Returns squared vector length.
    [[nodiscard]] constexpr Scalar LengthSquared() const noexcept
    {
        return Dot(*this);
    }

    /// @brief Returns dot product with another vector.
    [[nodiscard]] constexpr Scalar Dot(Vec3 other) const noexcept
    {
        return (x * other.x) + (y * other.y) + (z * other.z);
    }

    /// @brief Returns cross product with another vector.
    [[nodiscard]] constexpr Vec3 Cross(Vec3 other) const noexcept
    {
        return Vec3((y * other.z) - (z * other.y), (z * other.x) - (x * other.z), (x * other.y) - (y * other.x));
    }

    /// @brief Returns component-wise multiplication result.
    [[nodiscard]] constexpr Vec3 Hadamard(Vec3 other) const noexcept
    {
        return Vec3(x * other.x, y * other.y, z * other.z);
    }

    constexpr Vec3& operator+=(Vec3 other) noexcept
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    constexpr Vec3& operator-=(Vec3 other) noexcept
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    constexpr Vec3& operator*=(Scalar scalar) noexcept
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    constexpr Vec3& operator/=(Scalar scalar) noexcept
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    [[nodiscard]] constexpr Vec3 operator-() const noexcept
    {
        return Vec3(-x, -y, -z);
    }

    [[nodiscard]] constexpr bool operator==(const Vec3&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const Vec3&) const noexcept = default;
};

[[nodiscard]] constexpr Vec3 operator+(Vec3 lhs, Vec3 rhs) noexcept
{
    lhs += rhs;
    return lhs;
}

[[nodiscard]] constexpr Vec3 operator-(Vec3 lhs, Vec3 rhs) noexcept
{
    lhs -= rhs;
    return lhs;
}

[[nodiscard]] constexpr Vec3 operator*(Vec3 vector, Vec3::Scalar scalar) noexcept
{
    vector *= scalar;
    return vector;
}

[[nodiscard]] constexpr Vec3 operator*(Vec3::Scalar scalar, Vec3 vector) noexcept
{
    vector *= scalar;
    return vector;
}

[[nodiscard]] constexpr Vec3 operator/(Vec3 vector, Vec3::Scalar scalar) noexcept
{
    vector /= scalar;
    return vector;
}

} // namespace zcore
