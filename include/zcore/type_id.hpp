/**************************************************************************/
/*  type_id.hpp                                                           */
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
 * @file include/zcore/type_id.hpp
 * @brief Deterministic type identity token for C++ types.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/type_id.hpp>
 * const zcore::TypeId id = zcore::TypeId::Of<int>();
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

[[nodiscard]] consteval u64 Fnv1a64(std::string_view text) noexcept
{
    u64 hash = 0xcbf29ce484222325ULL;
    for (const char value : text) {
        hash ^= static_cast<u64>(static_cast<unsigned char>(value));
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

template <typename ValueT>
[[nodiscard]] consteval std::string_view TypeSignature() noexcept
{
#if defined(__clang__) || defined(__GNUC__)
    return std::string_view(__PRETTY_FUNCTION__, sizeof(__PRETTY_FUNCTION__) - 1U);
#elif defined(_MSC_VER)
    return std::string_view(__FUNCSIG__, sizeof(__FUNCSIG__) - 1U);
#else
    return std::string_view("zcore::TypeId::unknown", sizeof("zcore::TypeId::unknown") - 1U);
#endif
}

} // namespace detail

/**
 * @brief Deterministic identity token derived from a C++ type.
 */
class [[nodiscard("TypeId must be handled explicitly.")]] TypeId final {
public:
    using ValueType = u64;

    /// @brief Invalid sentinel for `TypeId`.
    static constexpr ValueType kInvalidValue = 0ULL;

    /// @brief Constructs an invalid type id.
    constexpr TypeId() noexcept : Value_(kInvalidValue)
    {
    }

    /**
   * @brief Constructs from raw value.
   * @param value Raw type identifier value.
   */
    constexpr explicit TypeId(ValueType value) noexcept : Value_(value)
    {
    }

    constexpr TypeId(const TypeId&) noexcept = default;
    constexpr TypeId& operator=(const TypeId&) noexcept = default;
    constexpr TypeId(TypeId&&) noexcept = default;
    constexpr TypeId& operator=(TypeId&&) noexcept = default;
    ~TypeId() = default;

    /// @brief Returns canonical invalid type id.
    [[nodiscard]] static constexpr TypeId Invalid() noexcept
    {
        return TypeId(kInvalidValue);
    }

    /**
   * @brief Constructs from raw value without validation.
   * @param value Raw type identifier value.
   * @return TypeId containing raw value.
   */
    [[nodiscard]] static constexpr TypeId FromRawUnchecked(ValueType value) noexcept
    {
        return TypeId(value);
    }

    /**
   * @brief Returns deterministic id for exact C++ type `ValueT`.
   * @tparam ValueT Type to identify.
   * @return Compile-time deterministic type id token.
   */
    template <typename ValueT>
    [[nodiscard]] static consteval TypeId Of() noexcept
    {
        const u64 digest = detail::Fnv1a64(detail::TypeSignature<ValueT>());
        return TypeId(digest == kInvalidValue ? 1ULL : digest);
    }

    /// @brief Returns raw id value.
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

    /// @brief Resets to invalid value.
    constexpr void Reset() noexcept
    {
        Value_ = kInvalidValue;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return IsValid();
    }

    [[nodiscard]] constexpr bool operator==(const TypeId&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const TypeId&) const noexcept = default;

private:
    ValueType Value_;
};

} // namespace zcore

namespace zcore::hash {

/**
 * @brief `zcore::hash` specialization for `zcore::TypeId`.
 */
template <>
struct Hash<zcore::TypeId, void> final {
    [[nodiscard]] constexpr Hash64 operator()(const zcore::TypeId& value, u64 seed = 0ULL) const noexcept
    {
        return Hash<zcore::TypeId::ValueType>{}(value.Value(), seed);
    }
};

} // namespace zcore::hash

namespace std {

/**
 * @brief `std::hash` adapter for `zcore::TypeId`.
 */
template <>
struct hash<zcore::TypeId> final {
    [[nodiscard]] std::size_t operator()(const zcore::TypeId& value) const noexcept
    {
        return zcore::hash::DigestToSizeT(zcore::hash::Hash<zcore::TypeId>{}(value));
    }
};

} // namespace std
