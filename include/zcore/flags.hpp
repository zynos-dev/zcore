/**************************************************************************/
/*  flags.hpp                                                             */
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
 * @file include/zcore/flags.hpp
 * @brief Type-safe enum-backed bit flag wrapper.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/flags.hpp>
 * enum class Permission : zcore::u8 { READ = 1U << 0U, WRITE = 1U << 1U };
 * zcore::Flags<Permission> permissions(Permission::READ);
 * @endcode
 */

#pragma once

#include <cstddef>
#include <functional>
#include <type_traits>
#include <zcore/hash/customization.hpp>

namespace zcore {

/**
 * @brief Enum-backed bit flag set with explicit typed operations.
 *
 * @tparam EnumT Enum type that provides bit mask values.
 */
template <typename EnumT>
class [[nodiscard("Flags must be handled explicitly.")]] Flags final {
public:
    static_assert(std::is_enum_v<EnumT>, "Flags<EnumT> requires an enum type.");

    using EnumType = EnumT;
    using UnderlyingType = std::underlying_type_t<EnumT>;
    using StorageType = std::make_unsigned_t<UnderlyingType>;

    /// @brief Empty mask value.
    static constexpr StorageType kEmptyValue = StorageType{0};

    /// @brief Constructs an empty flag set.
    constexpr Flags() noexcept : Bits_(kEmptyValue)
    {
    }

    /**
   * @brief Constructs a flag set containing one enum mask value.
   * @param flag Flag mask value to enable.
   */
    constexpr Flags(EnumT flag) noexcept : Bits_(ToMask(flag))
    {
    }

    /**
   * @brief Constructs from raw bits.
   * @param bits Raw bit mask.
   */
    explicit constexpr Flags(StorageType bits) noexcept : Bits_(bits)
    {
    }

    constexpr Flags(const Flags&) noexcept = default;
    constexpr Flags& operator=(const Flags&) noexcept = default;
    constexpr Flags(Flags&&) noexcept = default;
    constexpr Flags& operator=(Flags&&) noexcept = default;
    ~Flags() = default;

    /// @brief Returns an empty flag set.
    [[nodiscard]] static constexpr Flags None() noexcept
    {
        return Flags();
    }

    /**
   * @brief Constructs from raw bits without validation.
   * @param bits Raw bit mask.
   * @return Flags wrapping `bits`.
   */
    [[nodiscard]] static constexpr Flags FromRawUnchecked(StorageType bits) noexcept
    {
        return Flags(bits);
    }

    /// @brief Returns raw mask bits.
    [[nodiscard]] constexpr StorageType Raw() const noexcept
    {
        return Bits_;
    }

    /// @brief Returns `true` when no bits are enabled.
    [[nodiscard]] constexpr bool IsEmpty() const noexcept
    {
        return Bits_ == kEmptyValue;
    }

    /// @brief Returns `true` when any bit is enabled.
    [[nodiscard]] constexpr bool Any() const noexcept
    {
        return !IsEmpty();
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return Any();
    }

    /// @brief Returns `true` when a specific flag bit is enabled.
    [[nodiscard]] constexpr bool Has(EnumT flag) const noexcept
    {
        return (Bits_ & ToMask(flag)) != kEmptyValue;
    }

    /// @brief Returns `true` when any bit from `other` is enabled.
    [[nodiscard]] constexpr bool HasAny(Flags other) const noexcept
    {
        return (Bits_ & other.Bits_) != kEmptyValue;
    }

    /// @brief Returns `true` when all bits from `other` are enabled.
    [[nodiscard]] constexpr bool HasAll(Flags other) const noexcept
    {
        return (Bits_ & other.Bits_) == other.Bits_;
    }

    /// @brief Enables one flag bit.
    constexpr void Set(EnumT flag) noexcept
    {
        Bits_ |= ToMask(flag);
    }

    /// @brief Enables all bits from `other`.
    constexpr void Set(Flags other) noexcept
    {
        Bits_ |= other.Bits_;
    }

    /// @brief Clears one flag bit.
    constexpr void Clear(EnumT flag) noexcept
    {
        Bits_ &= static_cast<StorageType>(~ToMask(flag));
    }

    /// @brief Clears all bits from `other`.
    constexpr void Clear(Flags other) noexcept
    {
        Bits_ &= static_cast<StorageType>(~other.Bits_);
    }

    /// @brief Toggles one flag bit.
    constexpr void Toggle(EnumT flag) noexcept
    {
        Bits_ ^= ToMask(flag);
    }

    /// @brief Toggles all bits from `other`.
    constexpr void Toggle(Flags other) noexcept
    {
        Bits_ ^= other.Bits_;
    }

    /// @brief Conditionally assigns one flag bit.
    constexpr void Assign(EnumT flag, bool enabled) noexcept
    {
        if (enabled) {
            Set(flag);
            return;
        }
        Clear(flag);
    }

    /// @brief Conditionally assigns all bits from `other`.
    constexpr void Assign(Flags other, bool enabled) noexcept
    {
        if (enabled) {
            Set(other);
            return;
        }
        Clear(other);
    }

    /// @brief Clears all bits.
    constexpr void ClearAll() noexcept
    {
        Bits_ = kEmptyValue;
    }

    constexpr Flags& operator|=(Flags other) noexcept
    {
        Bits_ |= other.Bits_;
        return *this;
    }

    constexpr Flags& operator|=(EnumT flag) noexcept
    {
        Bits_ |= ToMask(flag);
        return *this;
    }

    constexpr Flags& operator&=(Flags other) noexcept
    {
        Bits_ &= other.Bits_;
        return *this;
    }

    constexpr Flags& operator&=(EnumT flag) noexcept
    {
        Bits_ &= ToMask(flag);
        return *this;
    }

    constexpr Flags& operator^=(Flags other) noexcept
    {
        Bits_ ^= other.Bits_;
        return *this;
    }

    constexpr Flags& operator^=(EnumT flag) noexcept
    {
        Bits_ ^= ToMask(flag);
        return *this;
    }

    [[nodiscard]] constexpr bool operator==(const Flags&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const Flags&) const noexcept = default;

    [[nodiscard]] friend constexpr Flags operator~(Flags value) noexcept
    {
        return Flags::FromRawUnchecked(static_cast<StorageType>(~value.Bits_));
    }

    [[nodiscard]] friend constexpr Flags operator|(Flags lhs, Flags rhs) noexcept
    {
        lhs |= rhs;
        return lhs;
    }

    [[nodiscard]] friend constexpr Flags operator&(Flags lhs, Flags rhs) noexcept
    {
        lhs &= rhs;
        return lhs;
    }

    [[nodiscard]] friend constexpr Flags operator^(Flags lhs, Flags rhs) noexcept
    {
        lhs ^= rhs;
        return lhs;
    }

    [[nodiscard]] friend constexpr Flags operator|(Flags lhs, EnumT rhs) noexcept
    {
        lhs |= rhs;
        return lhs;
    }

    [[nodiscard]] friend constexpr Flags operator&(Flags lhs, EnumT rhs) noexcept
    {
        lhs &= rhs;
        return lhs;
    }

    [[nodiscard]] friend constexpr Flags operator^(Flags lhs, EnumT rhs) noexcept
    {
        lhs ^= rhs;
        return lhs;
    }

    [[nodiscard]] friend constexpr Flags operator|(EnumT lhs, Flags rhs) noexcept
    {
        rhs |= lhs;
        return rhs;
    }

    [[nodiscard]] friend constexpr Flags operator&(EnumT lhs, Flags rhs) noexcept
    {
        rhs &= lhs;
        return rhs;
    }

    [[nodiscard]] friend constexpr Flags operator^(EnumT lhs, Flags rhs) noexcept
    {
        rhs ^= lhs;
        return rhs;
    }

    [[nodiscard]] friend constexpr Flags operator|(EnumT lhs, EnumT rhs) noexcept
    {
        return Flags(lhs) | rhs;
    }

    [[nodiscard]] friend constexpr Flags operator&(EnumT lhs, EnumT rhs) noexcept
    {
        return Flags(lhs) & rhs;
    }

    [[nodiscard]] friend constexpr Flags operator^(EnumT lhs, EnumT rhs) noexcept
    {
        return Flags(lhs) ^ rhs;
    }

private:
    [[nodiscard]] static constexpr StorageType ToMask(EnumT flag) noexcept
    {
        return static_cast<StorageType>(static_cast<UnderlyingType>(flag));
    }

    StorageType Bits_;
};

} // namespace zcore

namespace zcore::hash {

/**
 * @brief `zcore::hash` specialization for `zcore::Flags`.
 */
template <typename EnumT>
struct Hash<zcore::Flags<EnumT>, void> final {
    [[nodiscard]] constexpr Hash64 operator()(const zcore::Flags<EnumT>& value, u64 seed = 0ULL) const noexcept
    {
        using StorageT = typename zcore::Flags<EnumT>::StorageType;
        return Hash<StorageT>{}(value.Raw(), seed);
    }
};

} // namespace zcore::hash

namespace std {

/**
 * @brief `std::hash` adapter for `zcore::Flags`.
 */
template <typename EnumT>
struct hash<zcore::Flags<EnumT>> final {
    [[nodiscard]] std::size_t operator()(const zcore::Flags<EnumT>& value) const noexcept
    {
        return zcore::hash::DigestToSizeT(zcore::hash::Hash<zcore::Flags<EnumT>>{}(value));
    }
};

} // namespace std
