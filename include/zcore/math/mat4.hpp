/**************************************************************************/
/*  math/mat4.hpp                                                         */
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
 * @file include/zcore/math/mat4.hpp
 * @brief Deterministic 4x4 matrix primitive.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/mat4.hpp>
 * const zcore::Vec4 out = zcore::Mat4::Identity() * zcore::Vec4(1.0F, 2.0F, 3.0F, 1.0F);
 * @endcode
 */

#pragma once

#include <compare>
#include <zcore/math/vec4.hpp>

namespace zcore {

/**
 * @brief Row-major 4x4 matrix with value semantics.
 */
class [[nodiscard("Mat4 must be handled explicitly.")]] Mat4 final {
public:
    using Scalar = float;

    /// @brief Constructs identity matrix.
    constexpr Mat4() noexcept
            : Row0_(1.0F, 0.0F, 0.0F, 0.0F)
            , Row1_(0.0F, 1.0F, 0.0F, 0.0F)
            , Row2_(0.0F, 0.0F, 1.0F, 0.0F)
            , Row3_(0.0F, 0.0F, 0.0F, 1.0F)
    {
    }

    /**
   * @brief Constructs matrix from row vectors.
   * @param row0 First row.
   * @param row1 Second row.
   * @param row2 Third row.
   * @param row3 Fourth row.
   */
    constexpr Mat4(Vec4 row0, Vec4 row1, Vec4 row2, Vec4 row3) noexcept : Row0_(row0), Row1_(row1), Row2_(row2), Row3_(row3)
    {
    }

    /// @brief Returns identity matrix.
    [[nodiscard]] static constexpr Mat4 Identity() noexcept
    {
        return Mat4();
    }

    /// @brief Returns zero matrix.
    [[nodiscard]] static constexpr Mat4 Zero() noexcept
    {
        return Mat4(Vec4::Zero(), Vec4::Zero(), Vec4::Zero(), Vec4::Zero());
    }

    /// @brief Returns first row.
    [[nodiscard]] constexpr Vec4 Row0() const noexcept
    {
        return Row0_;
    }

    /// @brief Returns second row.
    [[nodiscard]] constexpr Vec4 Row1() const noexcept
    {
        return Row1_;
    }

    /// @brief Returns third row.
    [[nodiscard]] constexpr Vec4 Row2() const noexcept
    {
        return Row2_;
    }

    /// @brief Returns fourth row.
    [[nodiscard]] constexpr Vec4 Row3() const noexcept
    {
        return Row3_;
    }

    /// @brief Returns matrix-vector product.
    [[nodiscard]] constexpr Vec4 operator*(Vec4 vector) const noexcept
    {
        return Vec4(Row0_.Dot(vector), Row1_.Dot(vector), Row2_.Dot(vector), Row3_.Dot(vector));
    }

    constexpr Mat4& operator+=(Mat4 other) noexcept
    {
        Row0_ += other.Row0_;
        Row1_ += other.Row1_;
        Row2_ += other.Row2_;
        Row3_ += other.Row3_;
        return *this;
    }

    constexpr Mat4& operator-=(Mat4 other) noexcept
    {
        Row0_ -= other.Row0_;
        Row1_ -= other.Row1_;
        Row2_ -= other.Row2_;
        Row3_ -= other.Row3_;
        return *this;
    }

    constexpr Mat4& operator*=(Scalar scalar) noexcept
    {
        Row0_ *= scalar;
        Row1_ *= scalar;
        Row2_ *= scalar;
        Row3_ *= scalar;
        return *this;
    }

    [[nodiscard]] constexpr bool operator==(const Mat4&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const Mat4&) const noexcept = default;

private:
    [[nodiscard]] constexpr Vec4 Column0() const noexcept
    {
        return Vec4(Row0_.x, Row1_.x, Row2_.x, Row3_.x);
    }

    [[nodiscard]] constexpr Vec4 Column1() const noexcept
    {
        return Vec4(Row0_.y, Row1_.y, Row2_.y, Row3_.y);
    }

    [[nodiscard]] constexpr Vec4 Column2() const noexcept
    {
        return Vec4(Row0_.z, Row1_.z, Row2_.z, Row3_.z);
    }

    [[nodiscard]] constexpr Vec4 Column3() const noexcept
    {
        return Vec4(Row0_.w, Row1_.w, Row2_.w, Row3_.w);
    }

    friend constexpr Mat4 operator*(Mat4 lhs, Mat4 rhs) noexcept;

    Vec4 Row0_;
    Vec4 Row1_;
    Vec4 Row2_;
    Vec4 Row3_;
};

[[nodiscard]] constexpr Mat4 operator+(Mat4 lhs, Mat4 rhs) noexcept
{
    lhs += rhs;
    return lhs;
}

[[nodiscard]] constexpr Mat4 operator-(Mat4 lhs, Mat4 rhs) noexcept
{
    lhs -= rhs;
    return lhs;
}

[[nodiscard]] constexpr Mat4 operator*(Mat4 matrix, Mat4::Scalar scalar) noexcept
{
    matrix *= scalar;
    return matrix;
}

[[nodiscard]] constexpr Mat4 operator*(Mat4::Scalar scalar, Mat4 matrix) noexcept
{
    matrix *= scalar;
    return matrix;
}

[[nodiscard]] constexpr Mat4 operator*(Mat4 lhs, Mat4 rhs) noexcept
{
    const Vec4 rhsCol0 = rhs.Column0();
    const Vec4 rhsCol1 = rhs.Column1();
    const Vec4 rhsCol2 = rhs.Column2();
    const Vec4 rhsCol3 = rhs.Column3();
    return Mat4(Vec4(lhs.Row0().Dot(rhsCol0), lhs.Row0().Dot(rhsCol1), lhs.Row0().Dot(rhsCol2), lhs.Row0().Dot(rhsCol3)),
                Vec4(lhs.Row1().Dot(rhsCol0), lhs.Row1().Dot(rhsCol1), lhs.Row1().Dot(rhsCol2), lhs.Row1().Dot(rhsCol3)),
                Vec4(lhs.Row2().Dot(rhsCol0), lhs.Row2().Dot(rhsCol1), lhs.Row2().Dot(rhsCol2), lhs.Row2().Dot(rhsCol3)),
                Vec4(lhs.Row3().Dot(rhsCol0), lhs.Row3().Dot(rhsCol1), lhs.Row3().Dot(rhsCol2), lhs.Row3().Dot(rhsCol3)));
}

} // namespace zcore
