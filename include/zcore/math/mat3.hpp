/**************************************************************************/
/*  math/mat3.hpp                                                         */
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
 * @file include/zcore/math/mat3.hpp
 * @brief Deterministic 3x3 matrix primitive.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/mat3.hpp>
 * const zcore::Vec3 out = zcore::Mat3::Identity() * zcore::Vec3(1.0F, 2.0F, 3.0F);
 * @endcode
 */

#pragma once

#include <compare>
#include <zcore/math/vec3.hpp>

namespace zcore {

/**
 * @brief Row-major 3x3 matrix with value semantics.
 */
class [[nodiscard("Mat3 must be handled explicitly.")]] Mat3 final {
public:
    using Scalar = float;

    /// @brief Constructs identity matrix.
    constexpr Mat3() noexcept : Row0_(1.0F, 0.0F, 0.0F), Row1_(0.0F, 1.0F, 0.0F), Row2_(0.0F, 0.0F, 1.0F)
    {
    }

    /**
   * @brief Constructs matrix from row vectors.
   * @param row0 First row.
   * @param row1 Second row.
   * @param row2 Third row.
   */
    constexpr Mat3(Vec3 row0, Vec3 row1, Vec3 row2) noexcept : Row0_(row0), Row1_(row1), Row2_(row2)
    {
    }

    /// @brief Returns identity matrix.
    [[nodiscard]] static constexpr Mat3 Identity() noexcept
    {
        return Mat3();
    }

    /// @brief Returns zero matrix.
    [[nodiscard]] static constexpr Mat3 Zero() noexcept
    {
        return Mat3(Vec3::Zero(), Vec3::Zero(), Vec3::Zero());
    }

    /// @brief Returns first row.
    [[nodiscard]] constexpr Vec3 Row0() const noexcept
    {
        return Row0_;
    }

    /// @brief Returns second row.
    [[nodiscard]] constexpr Vec3 Row1() const noexcept
    {
        return Row1_;
    }

    /// @brief Returns third row.
    [[nodiscard]] constexpr Vec3 Row2() const noexcept
    {
        return Row2_;
    }

    /// @brief Returns matrix-vector product.
    [[nodiscard]] constexpr Vec3 operator*(Vec3 vector) const noexcept
    {
        return Vec3(Row0_.Dot(vector), Row1_.Dot(vector), Row2_.Dot(vector));
    }

    constexpr Mat3& operator+=(Mat3 other) noexcept
    {
        Row0_ += other.Row0_;
        Row1_ += other.Row1_;
        Row2_ += other.Row2_;
        return *this;
    }

    constexpr Mat3& operator-=(Mat3 other) noexcept
    {
        Row0_ -= other.Row0_;
        Row1_ -= other.Row1_;
        Row2_ -= other.Row2_;
        return *this;
    }

    constexpr Mat3& operator*=(Scalar scalar) noexcept
    {
        Row0_ *= scalar;
        Row1_ *= scalar;
        Row2_ *= scalar;
        return *this;
    }

    [[nodiscard]] constexpr bool operator==(const Mat3&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const Mat3&) const noexcept = default;

private:
    [[nodiscard]] constexpr Vec3 Column0() const noexcept
    {
        return Vec3(Row0_.x, Row1_.x, Row2_.x);
    }

    [[nodiscard]] constexpr Vec3 Column1() const noexcept
    {
        return Vec3(Row0_.y, Row1_.y, Row2_.y);
    }

    [[nodiscard]] constexpr Vec3 Column2() const noexcept
    {
        return Vec3(Row0_.z, Row1_.z, Row2_.z);
    }

    friend constexpr Mat3 operator*(Mat3 lhs, Mat3 rhs) noexcept;

    Vec3 Row0_;
    Vec3 Row1_;
    Vec3 Row2_;
};

[[nodiscard]] constexpr Mat3 operator+(Mat3 lhs, Mat3 rhs) noexcept
{
    lhs += rhs;
    return lhs;
}

[[nodiscard]] constexpr Mat3 operator-(Mat3 lhs, Mat3 rhs) noexcept
{
    lhs -= rhs;
    return lhs;
}

[[nodiscard]] constexpr Mat3 operator*(Mat3 matrix, Mat3::Scalar scalar) noexcept
{
    matrix *= scalar;
    return matrix;
}

[[nodiscard]] constexpr Mat3 operator*(Mat3::Scalar scalar, Mat3 matrix) noexcept
{
    matrix *= scalar;
    return matrix;
}

[[nodiscard]] constexpr Mat3 operator*(Mat3 lhs, Mat3 rhs) noexcept
{
    const Vec3 rhsCol0 = rhs.Column0();
    const Vec3 rhsCol1 = rhs.Column1();
    const Vec3 rhsCol2 = rhs.Column2();
    return Mat3(Vec3(lhs.Row0().Dot(rhsCol0), lhs.Row0().Dot(rhsCol1), lhs.Row0().Dot(rhsCol2)),
                Vec3(lhs.Row1().Dot(rhsCol0), lhs.Row1().Dot(rhsCol1), lhs.Row1().Dot(rhsCol2)),
                Vec3(lhs.Row2().Dot(rhsCol0), lhs.Row2().Dot(rhsCol1), lhs.Row2().Dot(rhsCol2)));
}

} // namespace zcore
