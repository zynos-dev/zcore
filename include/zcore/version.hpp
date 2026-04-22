/**************************************************************************/
/*  version.hpp                                                           */
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
 * @file include/zcore/version.hpp
 * @brief Deterministic semantic version value type.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/version.hpp>
 * constexpr zcore::Version api(1U, 2U, 3U);
 * @endcode
 */

#pragma once

#include <cstddef>
#include <functional>
#include <zcore/foundation.hpp>
#include <zcore/hash/customization.hpp>

namespace zcore {

/**
 * @brief Semantic version triple (`major.minor.patch`) with value semantics.
 */
class [[nodiscard("Version must be handled explicitly.")]] Version final {
public:
    using PartType = u32;

    /// @brief Constructs zero semantic version (`0.0.0`).
    constexpr Version() noexcept : Major_(0U), Minor_(0U), Patch_(0U)
    {
    }

    /**
   * @brief Constructs semantic version from raw parts.
   * @param major Major version part.
   * @param minor Minor version part.
   * @param patch Patch version part.
   */
    constexpr Version(PartType major, PartType minor, PartType patch) noexcept : Major_(major), Minor_(minor), Patch_(patch)
    {
    }

    constexpr Version(const Version&) noexcept = default;
    constexpr Version& operator=(const Version&) noexcept = default;
    constexpr Version(Version&&) noexcept = default;
    constexpr Version& operator=(Version&&) noexcept = default;
    ~Version() = default;

    /// @brief Returns zero semantic version (`0.0.0`).
    [[nodiscard]] static constexpr Version Zero() noexcept
    {
        return Version();
    }

    /**
   * @brief Constructs semantic version from raw parts without validation.
   * @param major Major version part.
   * @param minor Minor version part.
   * @param patch Patch version part.
   */
    [[nodiscard]] static constexpr Version FromRawUnchecked(PartType major, PartType minor, PartType patch) noexcept
    {
        return Version(major, minor, patch);
    }

    /// @brief Returns major version part.
    [[nodiscard]] constexpr PartType Major() const noexcept
    {
        return Major_;
    }

    /// @brief Returns minor version part.
    [[nodiscard]] constexpr PartType Minor() const noexcept
    {
        return Minor_;
    }

    /// @brief Returns patch version part.
    [[nodiscard]] constexpr PartType Patch() const noexcept
    {
        return Patch_;
    }

    /// @brief Returns `true` when version equals `0.0.0`.
    [[nodiscard]] constexpr bool IsZero() const noexcept
    {
        return Major_ == 0U && Minor_ == 0U && Patch_ == 0U;
    }

    /// @brief Returns `true` when major version is zero.
    [[nodiscard]] constexpr bool IsPreStable() const noexcept
    {
        return Major_ == 0U;
    }

    /// @brief Sets major version and resets minor/patch to zero.
    constexpr void SetMajor(PartType major) noexcept
    {
        Major_ = major;
        Minor_ = 0U;
        Patch_ = 0U;
    }

    /// @brief Sets minor version and resets patch to zero.
    constexpr void SetMinor(PartType minor) noexcept
    {
        Minor_ = minor;
        Patch_ = 0U;
    }

    /// @brief Sets patch version.
    constexpr void SetPatch(PartType patch) noexcept
    {
        Patch_ = patch;
    }

    /// @brief Increments major version and resets minor/patch to zero.
    constexpr void BumpMajor() noexcept
    {
        ++Major_;
        Minor_ = 0U;
        Patch_ = 0U;
    }

    /// @brief Increments minor version and resets patch to zero.
    constexpr void BumpMinor() noexcept
    {
        ++Minor_;
        Patch_ = 0U;
    }

    /// @brief Increments patch version.
    constexpr void BumpPatch() noexcept
    {
        ++Patch_;
    }

    /// @brief Resets to zero semantic version (`0.0.0`).
    constexpr void Reset() noexcept
    {
        Major_ = 0U;
        Minor_ = 0U;
        Patch_ = 0U;
    }

    [[nodiscard]] constexpr bool operator==(const Version&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const Version&) const noexcept = default;

private:
    PartType Major_;
    PartType Minor_;
    PartType Patch_;
};

} // namespace zcore

namespace zcore::hash {

/**
 * @brief `zcore::hash` specialization for `zcore::Version`.
 */
template <>
struct Hash<zcore::Version, void> final {
    [[nodiscard]] constexpr Hash64 operator()(const zcore::Version& value, u64 seed = 0ULL) const noexcept
    {
        const Hash64 majorDigest = Hash<zcore::Version::PartType>{}(value.Major(), seed);
        const Hash64 minorDigest = Hash<zcore::Version::PartType>{}(value.Minor(), majorDigest);
        return Hash<zcore::Version::PartType>{}(value.Patch(), minorDigest);
    }
};

} // namespace zcore::hash

namespace std {

/**
 * @brief `std::hash` adapter for `zcore::Version`.
 */
template <>
struct hash<zcore::Version> final {
    [[nodiscard]] std::size_t operator()(const zcore::Version& value) const noexcept
    {
        return zcore::hash::DigestToSizeT(zcore::hash::Hash<zcore::Version>{}(value));
    }
};

} // namespace std
