/**************************************************************************/
/*  common.hpp                                                            */
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
 * @file include/zcore/hash/detail/common.hpp
 * @brief Internal shared utilities for zcore hash implementations.
 * @details Internal header; include <zcore/hash.hpp> for public APIs.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/hash.hpp>
 * auto digest = zcore::hash::HashBytes(bytes);
 * @endcode
 */

#pragma once

#include <bit>
#include <cstring>
#include <type_traits>
#include <zcore/foundation.hpp>
#include <zcore/slice.hpp>

#if defined(_MSC_VER)
  #include <intrin.h>
#endif

namespace zcore::hash::detail {

[[nodiscard]] constexpr u64 SplitMix64Step(u64& state) noexcept
{
    state += 0x9e3779b97f4a7c15ULL;
    u64 z = state;
    z = (z ^ (z >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27U)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31U);
}

[[nodiscard]] inline u64 RotateLeft64Runtime(u64 value, u32 bits) noexcept
{
    const u32 normalizedBits = bits & 63U;
    if (normalizedBits == 0U) {
        return value;
    }

#if defined(_MSC_VER) && defined(_M_X64)
    return static_cast<u64>(_rotl64(value, static_cast<int>(normalizedBits)));
#elif defined(__has_builtin)
  #if __has_builtin(__builtin_rotateleft64)
    return static_cast<u64>(__builtin_rotateleft64(value, normalizedBits));
  #else
    return std::rotl(value, static_cast<int>(normalizedBits));
  #endif
#else
    return std::rotl(value, static_cast<int>(normalizedBits));
#endif
}

[[nodiscard]] constexpr u64 RotateLeft64(u64 value, u32 bits) noexcept
{
    const u32 normalizedBits = bits & 63U;
    if (normalizedBits == 0U) {
        return value;
    }
    if (std::is_constant_evaluated()) {
        return (value << normalizedBits) | (value >> (64U - normalizedBits));
    }
    return RotateLeft64Runtime(value, normalizedBits);
}

[[nodiscard]] constexpr u8 ByteAt(ByteSpan data, usize index) noexcept
{
    return std::to_integer<u8>(data[index]);
}

[[nodiscard]] inline u64 ByteSwap64(u64 value) noexcept
{
#if defined(_MSC_VER)
    return static_cast<u64>(_byteswap_uint64(value));
#elif defined(__has_builtin)
  #if __has_builtin(__builtin_bswap64)
    return static_cast<u64>(__builtin_bswap64(value));
  #else
    return ((value & 0x00000000000000FFULL) << 56U) | ((value & 0x000000000000FF00ULL) << 40U)
           | ((value & 0x0000000000FF0000ULL) << 24U) | ((value & 0x00000000FF000000ULL) << 8U)
           | ((value & 0x000000FF00000000ULL) >> 8U) | ((value & 0x0000FF0000000000ULL) >> 24U)
           | ((value & 0x00FF000000000000ULL) >> 40U) | ((value & 0xFF00000000000000ULL) >> 56U);
  #endif
#else
    return ((value & 0x00000000000000FFULL) << 56U) | ((value & 0x000000000000FF00ULL) << 40U)
           | ((value & 0x0000000000FF0000ULL) << 24U) | ((value & 0x00000000FF000000ULL) << 8U)
           | ((value & 0x000000FF00000000ULL) >> 8U) | ((value & 0x0000FF0000000000ULL) >> 24U)
           | ((value & 0x00FF000000000000ULL) >> 40U) | ((value & 0xFF00000000000000ULL) >> 56U);
#endif
}

[[nodiscard]] inline u64 ReadLe64Runtime(const Byte* data) noexcept
{
    u64 value = 0ULL;
    std::memcpy(&value, data, sizeof(value));

    if constexpr (std::endian::native == std::endian::little) {
        return value;
    }
    return ByteSwap64(value);
}

[[nodiscard]] constexpr u64 ReadLe64(ByteSpan data, usize offset) noexcept
{
    if (!std::is_constant_evaluated()) {
        return ReadLe64Runtime(data.Data() + offset);
    }

    return static_cast<u64>(ByteAt(data, offset)) | (static_cast<u64>(ByteAt(data, offset + 1U)) << 8U)
           | (static_cast<u64>(ByteAt(data, offset + 2U)) << 16U) | (static_cast<u64>(ByteAt(data, offset + 3U)) << 24U)
           | (static_cast<u64>(ByteAt(data, offset + 4U)) << 32U) | (static_cast<u64>(ByteAt(data, offset + 5U)) << 40U)
           | (static_cast<u64>(ByteAt(data, offset + 6U)) << 48U) | (static_cast<u64>(ByteAt(data, offset + 7U)) << 56U);
}

} // namespace zcore::hash::detail
