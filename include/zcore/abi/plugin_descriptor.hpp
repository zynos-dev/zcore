/**************************************************************************/
/*  plugin_descriptor.hpp                                                 */
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
 * @file include/zcore/abi/plugin_descriptor.hpp
 * @brief Deterministic plugin ABI metadata descriptor.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/plugin_descriptor.hpp>
 * constexpr zcore::InterfaceId kReader = zcore::InterfaceId::FromLiteral("zcore.io.reader.v1");
 * constexpr zcore::InterfaceId kPlugin = zcore::InterfaceId::FromLiteral("zcore.plugin.reader_impl.v1");
 * const zcore::InterfaceId interfaces[] = {kReader};
 * const zcore::PluginDescriptor descriptor(kPlugin, zcore::AbiVersion(1U, 0U), zcore::Slice<const zcore::InterfaceId>(interfaces));
 * @endcode
 */

#pragma once

#include <compare>
#include <cstddef>
#include <functional>
#include <zcore/abi/abi_version.hpp>
#include <zcore/abi/interface_id.hpp>
#include <zcore/foundation.hpp>
#include <zcore/hash/customization.hpp>
#include <zcore/slice.hpp>

namespace zcore {

/**
 * @brief Plugin ABI descriptor for compatibility and interface discovery.
 *
 * Valid descriptor requirements:
 * - plugin id is valid
 * - ABI version is non-zero
 * - each exposed interface id is valid
 * - exposed interface ids are unique
 */
class [[nodiscard("PluginDescriptor must be handled explicitly.")]] PluginDescriptor final {
public:
    /// @brief Constructs invalid empty descriptor.
    constexpr PluginDescriptor() noexcept
            : PluginId_(InterfaceId::Invalid())
            , AbiVersion_(AbiVersion::Zero())
            , ExposedInterfaces_(Slice<const InterfaceId>::Empty())
    {
    }

    /**
   * @brief Constructs descriptor from explicit parts.
   * @param pluginId Stable plugin id token.
   * @param abiVersion Plugin ABI version.
   * @param exposedInterfaces Non-owning view of exposed interfaces.
   */
    constexpr PluginDescriptor(InterfaceId pluginId, AbiVersion abiVersion, Slice<const InterfaceId> exposedInterfaces) noexcept
            : PluginId_(pluginId), AbiVersion_(abiVersion), ExposedInterfaces_(exposedInterfaces)
    {
    }

    constexpr PluginDescriptor(const PluginDescriptor&) noexcept = default;
    constexpr PluginDescriptor& operator=(const PluginDescriptor&) noexcept = default;
    constexpr PluginDescriptor(PluginDescriptor&&) noexcept = default;
    constexpr PluginDescriptor& operator=(PluginDescriptor&&) noexcept = default;
    ~PluginDescriptor() = default;

    /// @brief Returns canonical invalid descriptor.
    [[nodiscard]] static constexpr PluginDescriptor Invalid() noexcept
    {
        return PluginDescriptor();
    }

    /**
   * @brief Constructs descriptor from raw parts without validation.
   */
    [[nodiscard]] static constexpr PluginDescriptor
    FromRawUnchecked(InterfaceId pluginId, AbiVersion abiVersion, Slice<const InterfaceId> exposedInterfaces) noexcept
    {
        return PluginDescriptor(pluginId, abiVersion, exposedInterfaces);
    }

    /// @brief Returns plugin id token.
    [[nodiscard]] constexpr InterfaceId PluginId() const noexcept
    {
        return PluginId_;
    }

    /// @brief Returns plugin ABI version.
    [[nodiscard]] constexpr AbiVersion PluginAbiVersion() const noexcept
    {
        return AbiVersion_;
    }

    /// @brief Returns exposed interface list view.
    [[nodiscard]] constexpr Slice<const InterfaceId> ExposedInterfaces() const noexcept
    {
        return ExposedInterfaces_;
    }

    /// @brief Returns number of exposed interfaces.
    [[nodiscard]] constexpr usize InterfaceCount() const noexcept
    {
        return ExposedInterfaces_.Size();
    }

    /**
   * @brief Returns `true` when descriptor satisfies validity contract.
   */
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        if (!PluginId_.IsValid() || AbiVersion_.IsZero()) {
            return false;
        }

        for (usize i = 0; i < ExposedInterfaces_.Size(); ++i) {
            const InterfaceId current = ExposedInterfaces_[i];
            if (!current.IsValid()) {
                return false;
            }

            for (usize j = i + 1; j < ExposedInterfaces_.Size(); ++j) {
                if (current == ExposedInterfaces_[j]) {
                    return false;
                }
            }
        }

        return true;
    }

    /// @brief Returns `true` when descriptor does not satisfy validity contract.
    [[nodiscard]] constexpr bool IsInvalid() const noexcept
    {
        return !IsValid();
    }

    /**
   * @brief Returns `true` when descriptor exposes `interfaceId`.
   */
    [[nodiscard]] constexpr bool Exposes(InterfaceId interfaceId) const noexcept
    {
        if (!interfaceId.IsValid()) {
            return false;
        }
        for (const InterfaceId id : ExposedInterfaces_) {
            if (id == interfaceId) {
                return true;
            }
        }
        return false;
    }

    /**
   * @brief Returns `true` when plugin descriptor can satisfy runtime request.
   *
   * Request is satisfied when:
   * - descriptor is valid
   * - plugin ABI is compatible with requested ABI
   * - requested interface is exposed
   */
    [[nodiscard]] constexpr bool Supports(AbiVersion requiredAbi, InterfaceId requiredInterface) const noexcept
    {
        return IsValid() && AbiVersion_.IsCompatibleWith(requiredAbi) && Exposes(requiredInterface);
    }

    /// @brief Resets to canonical invalid descriptor.
    constexpr void Reset() noexcept
    {
        *this = Invalid();
    }

    [[nodiscard]] constexpr bool operator==(const PluginDescriptor& other) const noexcept
    {
        if (PluginId_ != other.PluginId_ || AbiVersion_ != other.AbiVersion_) {
            return false;
        }
        if (ExposedInterfaces_.Size() != other.ExposedInterfaces_.Size()) {
            return false;
        }
        for (usize i = 0; i < ExposedInterfaces_.Size(); ++i) {
            if (ExposedInterfaces_[i] != other.ExposedInterfaces_[i]) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] constexpr std::strong_ordering operator<=>(const PluginDescriptor& other) const noexcept
    {
        if (const auto byId = PluginId_ <=> other.PluginId_; byId != 0) {
            return byId;
        }
        if (const auto byAbi = AbiVersion_ <=> other.AbiVersion_; byAbi != 0) {
            return byAbi;
        }

        const usize lhsCount = ExposedInterfaces_.Size();
        const usize rhsCount = other.ExposedInterfaces_.Size();
        const usize minCount = lhsCount < rhsCount ? lhsCount : rhsCount;
        for (usize i = 0; i < minCount; ++i) {
            if (const auto byInterface = ExposedInterfaces_[i] <=> other.ExposedInterfaces_[i]; byInterface != 0) {
                return byInterface;
            }
        }

        if (lhsCount < rhsCount) {
            return std::strong_ordering::less;
        }
        if (lhsCount > rhsCount) {
            return std::strong_ordering::greater;
        }
        return std::strong_ordering::equal;
    }

private:
    InterfaceId PluginId_;
    AbiVersion AbiVersion_;
    Slice<const InterfaceId> ExposedInterfaces_;
};

} // namespace zcore

namespace zcore::hash {

/**
 * @brief `zcore::hash` specialization for `zcore::PluginDescriptor`.
 */
template <>
struct Hash<zcore::PluginDescriptor, void> final {
    [[nodiscard]] constexpr Hash64 operator()(const zcore::PluginDescriptor& value, u64 seed = 0ULL) const noexcept
    {
        Hash64 digest = Hash<zcore::InterfaceId>{}(value.PluginId(), seed);
        digest = Hash<zcore::AbiVersion>{}(value.PluginAbiVersion(), digest);
        for (const zcore::InterfaceId id : value.ExposedInterfaces()) {
            digest = Hash<zcore::InterfaceId>{}(id, digest);
        }
        return digest;
    }
};

} // namespace zcore::hash

namespace std {

/**
 * @brief `std::hash` adapter for `zcore::PluginDescriptor`.
 */
template <>
struct hash<zcore::PluginDescriptor> final {
    [[nodiscard]] std::size_t operator()(const zcore::PluginDescriptor& value) const noexcept
    {
        return zcore::hash::DigestToSizeT(zcore::hash::Hash<zcore::PluginDescriptor>{}(value));
    }
};

} // namespace std
