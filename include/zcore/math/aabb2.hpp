/**************************************************************************/
/*  math/aabb2.hpp                                                        */
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
 * @file include/zcore/math/aabb2.hpp
 * @brief Deterministic 2D axis-aligned bounding box primitive.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/aabb2.hpp>
 * const zcore::Aabb2 bounds(zcore::Vec2(-1.0F, -1.0F), zcore::Vec2(1.0F, 1.0F));
 * @endcode
 */

#pragma once

#include <compare>
#include <zcore/math/vec2.hpp>

namespace zcore {

/**
 * @brief 2D axis-aligned bounds with canonicalized min/max corners.
 */
class [[nodiscard("Aabb2 must be handled explicitly.")]] Aabb2 final {
public:
    /// @brief Constructs zero-sized bounds at origin.
    constexpr Aabb2() noexcept : Min_(Vec2::Zero()), Max_(Vec2::Zero())
    {
    }

    /**
   * @brief Constructs bounds from corners; order is canonicalized.
   * @param minCorner Candidate minimum corner.
   * @param maxCorner Candidate maximum corner.
   */
    constexpr Aabb2(Vec2 minCorner, Vec2 maxCorner) noexcept
            : Min_(ComponentMin(minCorner, maxCorner)), Max_(ComponentMax(minCorner, maxCorner))
    {
    }

    /// @brief Returns zero-sized bounds at origin.
    [[nodiscard]] static constexpr Aabb2 Zero() noexcept
    {
        return Aabb2();
    }

    /**
   * @brief Constructs bounds from center and half extents.
   * @param center Center point.
   * @param halfExtents Non-negative half extents (component-wise absolute value applied).
   */
    [[nodiscard]] static constexpr Aabb2 FromCenterAndHalfExtents(Vec2 center, Vec2 halfExtents) noexcept
    {
        const Vec2 half(Abs(halfExtents.x), Abs(halfExtents.y));
        return Aabb2(center - half, center + half);
    }

    /// @brief Returns min corner.
    [[nodiscard]] constexpr Vec2 Min() const noexcept
    {
        return Min_;
    }

    /// @brief Returns max corner.
    [[nodiscard]] constexpr Vec2 Max() const noexcept
    {
        return Max_;
    }

    /// @brief Returns bounds center.
    [[nodiscard]] constexpr Vec2 Center() const noexcept
    {
        return (Min_ + Max_) * 0.5F;
    }

    /// @brief Returns full extents.
    [[nodiscard]] constexpr Vec2 Extents() const noexcept
    {
        return Max_ - Min_;
    }

    /// @brief Returns `true` when point is within inclusive bounds.
    [[nodiscard]] constexpr bool Contains(Vec2 point) const noexcept
    {
        return point.x >= Min_.x && point.x <= Max_.x && point.y >= Min_.y && point.y <= Max_.y;
    }

    /// @brief Returns `true` when bounds overlap (inclusive edges).
    [[nodiscard]] constexpr bool Intersects(Aabb2 other) const noexcept
    {
        return !(Max_.x < other.Min_.x || Min_.x > other.Max_.x || Max_.y < other.Min_.y || Min_.y > other.Max_.y);
    }

    /// @brief Expands bounds to include point.
    constexpr void ExpandToInclude(Vec2 point) noexcept
    {
        Min_ = ComponentMin(Min_, point);
        Max_ = ComponentMax(Max_, point);
    }

    /// @brief Expands bounds to include another bounds.
    constexpr void Merge(Aabb2 other) noexcept
    {
        Min_ = ComponentMin(Min_, other.Min_);
        Max_ = ComponentMax(Max_, other.Max_);
    }

    [[nodiscard]] constexpr bool operator==(const Aabb2&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const Aabb2&) const noexcept = default;

private:
    [[nodiscard]] static constexpr Vec2 ComponentMin(Vec2 lhs, Vec2 rhs) noexcept
    {
        return Vec2(lhs.x < rhs.x ? lhs.x : rhs.x, lhs.y < rhs.y ? lhs.y : rhs.y);
    }

    [[nodiscard]] static constexpr Vec2 ComponentMax(Vec2 lhs, Vec2 rhs) noexcept
    {
        return Vec2(lhs.x > rhs.x ? lhs.x : rhs.x, lhs.y > rhs.y ? lhs.y : rhs.y);
    }

    [[nodiscard]] static constexpr float Abs(float value) noexcept
    {
        return value < 0.0F ? -value : value;
    }

    Vec2 Min_;
    Vec2 Max_;
};

} // namespace zcore
