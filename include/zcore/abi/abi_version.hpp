/**************************************************************************/
/*  abi_version.hpp                                                       */
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
 * @file include/zcore/abi/abi_version.hpp
 * @brief Deterministic ABI epoch/revision version value type.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/abi_version.hpp>
 * constexpr zcore::AbiVersion abi(1U, 0U);
 * @endcode
 */

#pragma once

#include <cstddef>
#include <functional>
#include <zcore/foundation.hpp>
#include <zcore/hash/customization.hpp>

namespace zcore {

/**
 * @brief ABI version pair (`epoch.revision`) for binary-compatibility contracts.
 *
 * Compatibility rule:
 * - same epoch required
 * - provider revision must be >= required revision
 */
class [[nodiscard("AbiVersion must be handled explicitly.")]] AbiVersion final {
public:
    using PartType = u32;

    /// @brief Constructs zero ABI version (`0.0`).
    constexpr AbiVersion() noexcept : Epoch_(0U), Revision_(0U)
    {
    }

    /**
   * @brief Constructs ABI version from raw parts.
   * @param epoch ABI epoch (breaking boundary).
   * @param revision ABI revision (backward-compatible additions within epoch).
   */
    constexpr AbiVersion(PartType epoch, PartType revision) noexcept : Epoch_(epoch), Revision_(revision)
    {
    }

    constexpr AbiVersion(const AbiVersion&) noexcept = default;
    constexpr AbiVersion& operator=(const AbiVersion&) noexcept = default;
    constexpr AbiVersion(AbiVersion&&) noexcept = default;
    constexpr AbiVersion& operator=(AbiVersion&&) noexcept = default;
    ~AbiVersion() = default;

    /// @brief Returns zero ABI version (`0.0`).
    [[nodiscard]] static constexpr AbiVersion Zero() noexcept
    {
        return AbiVersion();
    }

    /**
   * @brief Constructs ABI version from raw parts without validation.
   * @param epoch ABI epoch.
   * @param revision ABI revision.
   */
    [[nodiscard]] static constexpr AbiVersion FromRawUnchecked(PartType epoch, PartType revision) noexcept
    {
        return AbiVersion(epoch, revision);
    }

    /// @brief Returns ABI epoch part.
    [[nodiscard]] constexpr PartType Epoch() const noexcept
    {
        return Epoch_;
    }

    /// @brief Returns ABI revision part.
    [[nodiscard]] constexpr PartType Revision() const noexcept
    {
        return Revision_;
    }

    /// @brief Returns `true` when version equals `0.0`.
    [[nodiscard]] constexpr bool IsZero() const noexcept
    {
        return Epoch_ == 0U && Revision_ == 0U;
    }

    /// @brief Sets epoch and resets revision to zero.
    constexpr void SetEpoch(PartType epoch) noexcept
    {
        Epoch_ = epoch;
        Revision_ = 0U;
    }

    /// @brief Sets revision within current epoch.
    constexpr void SetRevision(PartType revision) noexcept
    {
        Revision_ = revision;
    }

    /// @brief Increments epoch and resets revision to zero.
    constexpr void BumpEpoch() noexcept
    {
        ++Epoch_;
        Revision_ = 0U;
    }

    /// @brief Increments revision.
    constexpr void BumpRevision() noexcept
    {
        ++Revision_;
    }

    /// @brief Resets to zero ABI version (`0.0`).
    constexpr void Reset() noexcept
    {
        Epoch_ = 0U;
        Revision_ = 0U;
    }

    /**
   * @brief Returns `true` when provider can satisfy `required` ABI version.
   *
   * Compatibility requires same epoch and provider revision >= required revision.
   */
    [[nodiscard]] constexpr bool IsCompatibleWith(AbiVersion required) const noexcept
    {
        return Epoch_ == required.Epoch_ && Revision_ >= required.Revision_;
    }

    [[nodiscard]] constexpr bool operator==(const AbiVersion&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const AbiVersion&) const noexcept = default;

private:
    PartType Epoch_;
    PartType Revision_;
};

} // namespace zcore

namespace zcore::hash {

/**
 * @brief `zcore::hash` specialization for `zcore::AbiVersion`.
 */
template <>
struct Hash<zcore::AbiVersion, void> final {
    [[nodiscard]] constexpr Hash64 operator()(const zcore::AbiVersion& value, u64 seed = 0ULL) const noexcept
    {
        const Hash64 epochDigest = Hash<zcore::AbiVersion::PartType>{}(value.Epoch(), seed);
        return Hash<zcore::AbiVersion::PartType>{}(value.Revision(), epochDigest);
    }
};

} // namespace zcore::hash

namespace std {

/**
 * @brief `std::hash` adapter for `zcore::AbiVersion`.
 */
template <>
struct hash<zcore::AbiVersion> final {
    [[nodiscard]] std::size_t operator()(const zcore::AbiVersion& value) const noexcept
    {
        return zcore::hash::DigestToSizeT(zcore::hash::Hash<zcore::AbiVersion>{}(value));
    }
};

} // namespace std
