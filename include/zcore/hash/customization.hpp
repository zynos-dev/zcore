/**************************************************************************/
/*  customization.hpp                                                     */
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
 * @file include/zcore/hash/customization.hpp
 * @brief First-class type-level hash customization contracts and defaults.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/hash/customization.hpp>
 * const zcore::hash::Hash64 digest = zcore::hash::Hash<int>{}(42);
 * @endcode
 */

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <zcore/foundation.hpp>
#include <zcore/hash/detail/value_hash_primitives.hpp>

namespace zcore::hash {

/// @brief 64-bit digest type returned by zcore hash APIs.
using Hash64 = u64;

/**
 * @brief Primary hash customization trait for zcore containers and utilities.
 *
 * Specialize this for project/domain types. Default integral/enum specializations
 * are provided below.
 *
 * @tparam ValueT Value type to hash.
 * @tparam EnableT SFINAE hook for partial specialization.
 */
template <typename ValueT, typename EnableT = void>
struct Hash final {
    [[nodiscard]] constexpr Hash64 operator()(const ValueT&, u64 = 0ULL) const noexcept = delete;
};

/**
 * @brief Default zcore hash specialization for integral and enum values.
 *
 * Uses deterministic seeded FNV-1a over canonical little-endian bytes.
 */
template <typename ValueT>
struct Hash<ValueT, std::enable_if_t<std::is_integral_v<ValueT> || std::is_enum_v<ValueT>>> final {
    [[nodiscard]] constexpr Hash64 operator()(ValueT value, u64 seed = 0ULL) const noexcept
    {
        const auto converted = detail::NormalizeToUnsigned(value);
        return detail::HashUnsignedFnv1a(converted, seed);
    }
};

/**
 * @brief Concept for values that provide `zcore::hash::Hash<ValueT>`.
 */
template <typename ValueT>
concept Hashable = requires(const ValueT& value, u64 seed) {
    { Hash<ValueT>{}(value, seed) } -> std::convertible_to<Hash64>;
};

/**
 * @brief Hashes a value through `zcore::hash::Hash<ValueT>`.
 * @param value Input value.
 * @param seed Optional seed.
 * @return 64-bit digest from `Hash<ValueT>`.
 */
template <typename ValueT>
    requires(Hashable<ValueT>)
[[nodiscard]] constexpr Hash64 HashObject(const ValueT& value, u64 seed = 0ULL) noexcept(noexcept(Hash<ValueT>{}(value, seed)))
{
    return static_cast<Hash64>(Hash<ValueT>{}(value, seed));
}

/**
 * @brief Adapts a 64-bit digest to `std::size_t` for standard containers.
 *
 * On 32-bit targets this folds high/low lanes to preserve entropy.
 */
[[nodiscard]] constexpr std::size_t DigestToSizeT(Hash64 digest) noexcept
{
#if SIZE_MAX >= UINT64_MAX
    return static_cast<std::size_t>(digest);
#else
    constexpr u64 kLowMask = 0xFFFFFFFFULL;
    return static_cast<std::size_t>((digest >> 32U) ^ (digest & kLowMask));
#endif
}

} // namespace zcore::hash
