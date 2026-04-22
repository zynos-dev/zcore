/**************************************************************************/
/*  math/transform3.hpp                                                   */
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
 * @file include/zcore/math/transform3.hpp
 * @brief Deterministic 3D transform primitive (translation, rotation, scale).
 * @par Usage
 * @code{.cpp}
 * #include <zcore/transform3.hpp>
 * const zcore::Vec3 out = zcore::Transform3::Identity().TransformPoint(zcore::Vec3::UnitX());
 * @endcode
 */

#pragma once

#include <compare>
#include <zcore/math/mat4.hpp>
#include <zcore/math/quat.hpp>
#include <zcore/math/vec3.hpp>

namespace zcore {

/**
 * @brief 3D TRS transform with explicit component storage.
 */
class [[nodiscard("Transform3 must be handled explicitly.")]] Transform3 final {
public:
    /// @brief Constructs identity transform.
    constexpr Transform3() noexcept : Translation_(Vec3::Zero()), Rotation_(Quat::Identity()), Scale_(Vec3::One())
    {
    }

    /**
   * @brief Constructs transform from explicit components.
   * @param translation World translation.
   * @param rotation World rotation quaternion.
   * @param scale Non-uniform scale.
   */
    constexpr Transform3(Vec3 translation, Quat rotation, Vec3 scale) noexcept
            : Translation_(translation), Rotation_(rotation), Scale_(scale)
    {
    }

    /// @brief Returns identity transform.
    [[nodiscard]] static constexpr Transform3 Identity() noexcept
    {
        return Transform3();
    }

    /// @brief Returns translation component.
    [[nodiscard]] constexpr Vec3 Translation() const noexcept
    {
        return Translation_;
    }

    /// @brief Returns rotation component.
    [[nodiscard]] constexpr Quat Rotation() const noexcept
    {
        return Rotation_;
    }

    /// @brief Returns scale component.
    [[nodiscard]] constexpr Vec3 Scale() const noexcept
    {
        return Scale_;
    }

    /// @brief Sets translation component.
    constexpr void SetTranslation(Vec3 translation) noexcept
    {
        Translation_ = translation;
    }

    /// @brief Sets rotation component.
    constexpr void SetRotation(Quat rotation) noexcept
    {
        Rotation_ = rotation;
    }

    /// @brief Sets scale component.
    constexpr void SetScale(Vec3 scale) noexcept
    {
        Scale_ = scale;
    }

    /// @brief Applies transform to a point.
    [[nodiscard]] constexpr Vec3 TransformPoint(Vec3 point) const noexcept
    {
        const Vec3 scaled = point.Hadamard(Scale_);
        const Vec3 rotated = Rotation_.RotateVector(scaled);
        return rotated + Translation_;
    }

    /// @brief Applies transform to a direction (no translation).
    [[nodiscard]] constexpr Vec3 TransformDirection(Vec3 direction) const noexcept
    {
        return Rotation_.RotateVector(direction.Hadamard(Scale_));
    }

    /// @brief Returns row-major matrix representation of the transform.
    [[nodiscard]] constexpr Mat4 ToMatrix() const noexcept
    {
        const float xx = Rotation_.x * Rotation_.x;
        const float yy = Rotation_.y * Rotation_.y;
        const float zz = Rotation_.z * Rotation_.z;
        const float xy = Rotation_.x * Rotation_.y;
        const float xz = Rotation_.x * Rotation_.z;
        const float yz = Rotation_.y * Rotation_.z;
        const float wx = Rotation_.w * Rotation_.x;
        const float wy = Rotation_.w * Rotation_.y;
        const float wz = Rotation_.w * Rotation_.z;

        const float r00 = 1.0F - (2.0F * (yy + zz));
        const float r01 = 2.0F * (xy - wz);
        const float r02 = 2.0F * (xz + wy);
        const float r10 = 2.0F * (xy + wz);
        const float r11 = 1.0F - (2.0F * (xx + zz));
        const float r12 = 2.0F * (yz - wx);
        const float r20 = 2.0F * (xz - wy);
        const float r21 = 2.0F * (yz + wx);
        const float r22 = 1.0F - (2.0F * (xx + yy));

        return Mat4(Vec4(r00 * Scale_.x, r01 * Scale_.y, r02 * Scale_.z, Translation_.x),
                    Vec4(r10 * Scale_.x, r11 * Scale_.y, r12 * Scale_.z, Translation_.y),
                    Vec4(r20 * Scale_.x, r21 * Scale_.y, r22 * Scale_.z, Translation_.z),
                    Vec4(0.0F, 0.0F, 0.0F, 1.0F));
    }

    [[nodiscard]] constexpr bool operator==(const Transform3&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const Transform3&) const noexcept = default;

private:
    Vec3 Translation_;
    Quat Rotation_;
    Vec3 Scale_;
};

} // namespace zcore
