/**************************************************************************/
/*  fnv1a64.hpp                                                           */
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
 * @file include/zcore/hash/detail/fnv1a64.hpp
 * @brief Internal FNV-1a 64-bit hash implementation.
 * @details Internal header; include <zcore/hash.hpp> for public APIs.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/hash.hpp>
 * auto digest = zcore::hash::HashBytes(bytes, zcore::hash::HashAlgorithm::FNV1A_64);
 * @endcode
 */

#pragma once

#include <type_traits>
#include <zcore/hash/detail/common.hpp>

namespace zcore::hash::detail {

inline constexpr u64 kFnv1a64OffsetBasis = 14695981039346656037ULL;
inline constexpr u64 kFnv1a64Prime = 1099511628211ULL;

[[nodiscard]] constexpr u64 Fnv1a64Init(u64 seed) noexcept
{
    return kFnv1a64OffsetBasis ^ seed;
}

[[nodiscard]] constexpr u64 Fnv1a64MixByte(u64 hash, u8 byte) noexcept
{
    return (hash ^ static_cast<u64>(byte)) * kFnv1a64Prime;
}

template <typename UnsignedT>
[[nodiscard]] constexpr u64 Fnv1a64UnsignedLe(UnsignedT value, u64 seed) noexcept
{
    static_assert(std::is_unsigned_v<UnsignedT>, "Fnv1a64UnsignedLe requires an unsigned value type.");

    u64 hash = Fnv1a64Init(seed);
    UnsignedT remaining = value;
    for (usize index = 0; index < sizeof(UnsignedT); ++index) {
        hash = Fnv1a64MixByte(hash, static_cast<u8>(remaining & static_cast<UnsignedT>(0xFFU)));
        remaining = static_cast<UnsignedT>(static_cast<u64>(remaining) >> 8U);
    }
    return hash;
}

[[nodiscard]] constexpr u64 Fnv1a64(ByteSpan data, u64 seed) noexcept
{
    u64 hash = Fnv1a64Init(seed);
    usize index = 0;
    const usize size = data.Size();

    while ((index + 8U) <= size) {
        const u64 lane = ReadLe64(data, index);
        hash = Fnv1a64MixByte(hash, static_cast<u8>((lane >> 0U) & 0xFFULL));
        hash = Fnv1a64MixByte(hash, static_cast<u8>((lane >> 8U) & 0xFFULL));
        hash = Fnv1a64MixByte(hash, static_cast<u8>((lane >> 16U) & 0xFFULL));
        hash = Fnv1a64MixByte(hash, static_cast<u8>((lane >> 24U) & 0xFFULL));
        hash = Fnv1a64MixByte(hash, static_cast<u8>((lane >> 32U) & 0xFFULL));
        hash = Fnv1a64MixByte(hash, static_cast<u8>((lane >> 40U) & 0xFFULL));
        hash = Fnv1a64MixByte(hash, static_cast<u8>((lane >> 48U) & 0xFFULL));
        hash = Fnv1a64MixByte(hash, static_cast<u8>((lane >> 56U) & 0xFFULL));
        index += 8U;
    }

    while (index < size) {
        hash = Fnv1a64MixByte(hash, ByteAt(data, index));
        ++index;
    }
    return hash;
}

} // namespace zcore::hash::detail
