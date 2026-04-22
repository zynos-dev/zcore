/**************************************************************************/
/*  math/quat.hpp                                                         */
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
 * @file include/zcore/math/quat.hpp
 * @brief Deterministic quaternion primitive for 3D rotation.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/quat.hpp>
 * const zcore::Quat identity = zcore::Quat::Identity();
 * @endcode
 */

#pragma once

#include <compare>
#include <zcore/math/vec3.hpp>

namespace zcore {

struct Quat;
[[nodiscard]] constexpr Quat operator*(Quat lhs, Quat rhs) noexcept;

/**
 * @brief Quaternion value type in `(x, y, z, w)` storage order.
 */
struct [[nodiscard("Quat must be handled explicitly.")]] Quat final {
    using Scalar = float;

    Scalar x;
    Scalar y;
    Scalar z;
    Scalar w;

    /// @brief Constructs identity quaternion.
    constexpr Quat() noexcept : x(0.0F), y(0.0F), z(0.0F), w(1.0F)
    {
    }

    /**
   * @brief Constructs quaternion from components.
   * @param xValue X component.
   * @param yValue Y component.
   * @param zValue Z component.
   * @param wValue W component.
   */
    constexpr Quat(Scalar xValue, Scalar yValue, Scalar zValue, Scalar wValue) noexcept : x(xValue), y(yValue), z(zValue), w(wValue)
    {
    }

    /// @brief Returns identity quaternion.
    [[nodiscard]] static constexpr Quat Identity() noexcept
    {
        return Quat(0.0F, 0.0F, 0.0F, 1.0F);
    }

    /// @brief Returns zero quaternion.
    [[nodiscard]] static constexpr Quat Zero() noexcept
    {
        return Quat(0.0F, 0.0F, 0.0F, 0.0F);
    }

    /// @brief Returns squared quaternion magnitude.
    [[nodiscard]] constexpr Scalar NormSquared() const noexcept
    {
        return (x * x) + (y * y) + (z * z) + (w * w);
    }

    /// @brief Returns quaternion conjugate.
    [[nodiscard]] constexpr Quat Conjugated() const noexcept
    {
        return Quat(-x, -y, -z, w);
    }

    /// @brief Rotates a vector using quaternion multiplication.
    [[nodiscard]] constexpr Vec3 RotateVector(Vec3 vector) const noexcept
    {
        const Quat pure(vector.x, vector.y, vector.z, 0.0F);
        const Quat rotated = (*this) * pure * Conjugated();
        return Vec3(rotated.x, rotated.y, rotated.z);
    }

    constexpr Quat& operator*=(Quat other) noexcept
    {
        *this = (*this) * other;
        return *this;
    }

    [[nodiscard]] constexpr bool operator==(const Quat&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const Quat&) const noexcept = default;
};

[[nodiscard]] constexpr Quat operator*(Quat lhs, Quat rhs) noexcept
{
    return Quat((lhs.w * rhs.x) + (lhs.x * rhs.w) + (lhs.y * rhs.z) - (lhs.z * rhs.y),
                (lhs.w * rhs.y) - (lhs.x * rhs.z) + (lhs.y * rhs.w) + (lhs.z * rhs.x),
                (lhs.w * rhs.z) + (lhs.x * rhs.y) - (lhs.y * rhs.x) + (lhs.z * rhs.w),
                (lhs.w * rhs.w) - (lhs.x * rhs.x) - (lhs.y * rhs.y) - (lhs.z * rhs.z));
}

} // namespace zcore
