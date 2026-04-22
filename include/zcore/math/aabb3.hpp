/**************************************************************************/
/*  math/aabb3.hpp                                                        */
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
 * @file include/zcore/math/aabb3.hpp
 * @brief Deterministic 3D axis-aligned bounding box primitive.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/aabb3.hpp>
 * const zcore::Aabb3 bounds(zcore::Vec3(-1.0F, -1.0F, -1.0F), zcore::Vec3(1.0F, 1.0F, 1.0F));
 * @endcode
 */

#pragma once

#include <compare>
#include <zcore/math/vec3.hpp>

namespace zcore {

/**
 * @brief 3D axis-aligned bounds with canonicalized min/max corners.
 */
class [[nodiscard("Aabb3 must be handled explicitly.")]] Aabb3 final {
public:
    /// @brief Constructs zero-sized bounds at origin.
    constexpr Aabb3() noexcept : Min_(Vec3::Zero()), Max_(Vec3::Zero())
    {
    }

    /**
   * @brief Constructs bounds from corners; order is canonicalized.
   * @param minCorner Candidate minimum corner.
   * @param maxCorner Candidate maximum corner.
   */
    constexpr Aabb3(Vec3 minCorner, Vec3 maxCorner) noexcept
            : Min_(ComponentMin(minCorner, maxCorner)), Max_(ComponentMax(minCorner, maxCorner))
    {
    }

    /// @brief Returns zero-sized bounds at origin.
    [[nodiscard]] static constexpr Aabb3 Zero() noexcept
    {
        return Aabb3();
    }

    /**
   * @brief Constructs bounds from center and half extents.
   * @param center Center point.
   * @param halfExtents Non-negative half extents (component-wise absolute value applied).
   */
    [[nodiscard]] static constexpr Aabb3 FromCenterAndHalfExtents(Vec3 center, Vec3 halfExtents) noexcept
    {
        const Vec3 half(Abs(halfExtents.x), Abs(halfExtents.y), Abs(halfExtents.z));
        return Aabb3(center - half, center + half);
    }

    /// @brief Returns min corner.
    [[nodiscard]] constexpr Vec3 Min() const noexcept
    {
        return Min_;
    }

    /// @brief Returns max corner.
    [[nodiscard]] constexpr Vec3 Max() const noexcept
    {
        return Max_;
    }

    /// @brief Returns bounds center.
    [[nodiscard]] constexpr Vec3 Center() const noexcept
    {
        return (Min_ + Max_) * 0.5F;
    }

    /// @brief Returns full extents.
    [[nodiscard]] constexpr Vec3 Extents() const noexcept
    {
        return Max_ - Min_;
    }

    /// @brief Returns `true` when point is within inclusive bounds.
    [[nodiscard]] constexpr bool Contains(Vec3 point) const noexcept
    {
        return point.x >= Min_.x && point.x <= Max_.x && point.y >= Min_.y && point.y <= Max_.y && point.z >= Min_.z
               && point.z <= Max_.z;
    }

    /// @brief Returns `true` when bounds overlap (inclusive edges).
    [[nodiscard]] constexpr bool Intersects(Aabb3 other) const noexcept
    {
        return !(Max_.x < other.Min_.x || Min_.x > other.Max_.x || Max_.y < other.Min_.y || Min_.y > other.Max_.y
                 || Max_.z < other.Min_.z || Min_.z > other.Max_.z);
    }

    /// @brief Expands bounds to include point.
    constexpr void ExpandToInclude(Vec3 point) noexcept
    {
        Min_ = ComponentMin(Min_, point);
        Max_ = ComponentMax(Max_, point);
    }

    /// @brief Expands bounds to include another bounds.
    constexpr void Merge(Aabb3 other) noexcept
    {
        Min_ = ComponentMin(Min_, other.Min_);
        Max_ = ComponentMax(Max_, other.Max_);
    }

    [[nodiscard]] constexpr bool operator==(const Aabb3&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const Aabb3&) const noexcept = default;

private:
    [[nodiscard]] static constexpr Vec3 ComponentMin(Vec3 lhs, Vec3 rhs) noexcept
    {
        return Vec3(lhs.x < rhs.x ? lhs.x : rhs.x, lhs.y < rhs.y ? lhs.y : rhs.y, lhs.z < rhs.z ? lhs.z : rhs.z);
    }

    [[nodiscard]] static constexpr Vec3 ComponentMax(Vec3 lhs, Vec3 rhs) noexcept
    {
        return Vec3(lhs.x > rhs.x ? lhs.x : rhs.x, lhs.y > rhs.y ? lhs.y : rhs.y, lhs.z > rhs.z ? lhs.z : rhs.z);
    }

    [[nodiscard]] static constexpr float Abs(float value) noexcept
    {
        return value < 0.0F ? -value : value;
    }

    Vec3 Min_;
    Vec3 Max_;
};

} // namespace zcore
