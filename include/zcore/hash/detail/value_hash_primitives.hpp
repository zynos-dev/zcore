/**************************************************************************/
/*  value_hash_primitives.hpp                                             */
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
 * @file include/zcore/hash/detail/value_hash_primitives.hpp
 * @brief Shared low-level value hashing primitives for zcore hash layers.
 * @details Internal header; include <zcore/hash.hpp> for public APIs.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/hash.hpp>
 * auto digest = zcore::hash::HashObject(42U);
 * @endcode
 */

#pragma once

#include <type_traits>
#include <zcore/foundation.hpp>
#include <zcore/hash/detail/fnv1a64.hpp>

namespace zcore::hash::detail {

template <typename ValueT, bool IsEnumV = std::is_enum_v<ValueT>>
struct RawValueType final {
    using Type = ValueT;
};

template <typename ValueT>
struct RawValueType<ValueT, true> final {
    using Type = std::underlying_type_t<ValueT>;
};

template <typename ValueT, bool IsBoolV = std::is_same_v<ValueT, bool>>
struct UnsignedValueType final {
    using Type = std::make_unsigned_t<ValueT>;
};

template <typename ValueT>
struct UnsignedValueType<ValueT, true> final {
    using Type = u8;
};

template <typename ValueT>
[[nodiscard]] constexpr auto NormalizeToUnsigned(ValueT value) noexcept
{
    using RawT = typename RawValueType<ValueT>::Type;
    using UnsignedT = typename UnsignedValueType<RawT>::Type;
    return static_cast<UnsignedT>(value);
}

template <typename UnsignedT>
[[nodiscard]] constexpr u64 HashUnsignedFnv1a(UnsignedT value, u64 seed) noexcept
{
    return Fnv1a64UnsignedLe(value, seed);
}

} // namespace zcore::hash::detail
