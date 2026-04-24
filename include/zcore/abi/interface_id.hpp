/**************************************************************************/
/*  interface_id.hpp                                                      */
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
 * @file include/zcore/abi/interface_id.hpp
 * @brief Deterministic ABI interface identifier token.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/interface_id.hpp>
 * constexpr zcore::InterfaceId reader = zcore::InterfaceId::FromLiteral("zcore.io.reader.v1");
 * @endcode
 */

#pragma once

#include <cstddef>
#include <functional>
#include <string_view>
#include <zcore/foundation.hpp>
#include <zcore/hash/customization.hpp>

namespace zcore {

namespace detail {

[[nodiscard]] constexpr u64 Fnv1a64(std::string_view text) noexcept
{
    u64 hash = 0xcbf29ce484222325ULL;
    for (const char value : text) {
        hash ^= static_cast<u64>(static_cast<unsigned char>(value));
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

} // namespace detail

/**
 * @brief Stable interface identifier token for ABI boundary contracts.
 *
 * Interface ids are represented as 64-bit values. Use `FromLiteral` with a
 * canonical interface name for deterministic compile-time ids.
 */
class [[nodiscard("InterfaceId must be handled explicitly.")]] InterfaceId final {
public:
    using ValueType = u64;

    /// @brief Invalid sentinel for `InterfaceId`.
    static constexpr ValueType kInvalidValue = 0ULL;

    /// @brief Constructs invalid interface id.
    constexpr InterfaceId() noexcept : Value_(kInvalidValue)
    {
    }

    /**
   * @brief Constructs from raw value.
   * @param value Raw interface id token.
   */
    constexpr explicit InterfaceId(ValueType value) noexcept : Value_(value)
    {
    }

    constexpr InterfaceId(const InterfaceId&) noexcept = default;
    constexpr InterfaceId& operator=(const InterfaceId&) noexcept = default;
    constexpr InterfaceId(InterfaceId&&) noexcept = default;
    constexpr InterfaceId& operator=(InterfaceId&&) noexcept = default;
    ~InterfaceId() = default;

    /// @brief Returns canonical invalid id.
    [[nodiscard]] static constexpr InterfaceId Invalid() noexcept
    {
        return InterfaceId(kInvalidValue);
    }

    /**
   * @brief Constructs from raw value without validation.
   * @param value Raw interface id token.
   */
    [[nodiscard]] static constexpr InterfaceId FromRawUnchecked(ValueType value) noexcept
    {
        return InterfaceId(value);
    }

    /**
   * @brief Constructs from canonical interface name.
   *
   * Empty names return `Invalid()`.
   *
   * @param canonicalName Stable interface name token.
   */
    [[nodiscard]] static constexpr InterfaceId FromName(std::string_view canonicalName) noexcept
    {
        if (canonicalName.empty()) {
            return Invalid();
        }
        const u64 digest = detail::Fnv1a64(canonicalName);
        return InterfaceId(digest == kInvalidValue ? 1ULL : digest);
    }

    /**
   * @brief Constructs deterministic compile-time id from string literal.
   * @tparam NameLengthV Literal storage length including null terminator.
   * @param canonicalName Canonical interface name string literal.
   */
    template <usize NameLengthV>
    [[nodiscard]] static consteval InterfaceId FromLiteral(const char (&canonicalName)[NameLengthV]) noexcept
    {
        static_assert(NameLengthV > 1U, "InterfaceId::FromLiteral requires non-empty canonical name.");
        return FromName(std::string_view(canonicalName, NameLengthV - 1U));
    }

    /// @brief Returns raw interface id token.
    [[nodiscard]] constexpr ValueType Value() const noexcept
    {
        return Value_;
    }

    /// @brief Returns `true` when id is non-invalid.
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return Value_ != kInvalidValue;
    }

    /// @brief Returns `true` when id equals invalid sentinel.
    [[nodiscard]] constexpr bool IsInvalid() const noexcept
    {
        return !IsValid();
    }

    /// @brief Resets id to invalid sentinel.
    constexpr void Reset() noexcept
    {
        Value_ = kInvalidValue;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return IsValid();
    }

    [[nodiscard]] constexpr bool operator==(const InterfaceId&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const InterfaceId&) const noexcept = default;

private:
    ValueType Value_;
};

} // namespace zcore

namespace zcore::hash {

/**
 * @brief `zcore::hash` specialization for `zcore::InterfaceId`.
 */
template <>
struct Hash<zcore::InterfaceId, void> final {
    [[nodiscard]] constexpr Hash64 operator()(const zcore::InterfaceId& value, u64 seed = 0ULL) const noexcept
    {
        return Hash<zcore::InterfaceId::ValueType>{}(value.Value(), seed);
    }
};

} // namespace zcore::hash

namespace std {

/**
 * @brief `std::hash` adapter for `zcore::InterfaceId`.
 */
template <>
struct hash<zcore::InterfaceId> final {
    [[nodiscard]] std::size_t operator()(const zcore::InterfaceId& value) const noexcept
    {
        return zcore::hash::DigestToSizeT(zcore::hash::Hash<zcore::InterfaceId>{}(value));
    }
};

} // namespace std
