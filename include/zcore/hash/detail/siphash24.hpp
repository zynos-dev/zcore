/**************************************************************************/
/*  siphash24.hpp                                                         */
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
 * @file include/zcore/hash/detail/siphash24.hpp
 * @brief Internal SipHash-2-4 implementation.
 * @details Internal header; include <zcore/hash.hpp> for public APIs.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/hash.hpp>
 * auto digest = zcore::hash::HashBytesKeyed(bytes, key);
 * @endcode
 */

#pragma once

#include <zcore/hash/detail/common.hpp>

namespace zcore::hash::detail {

constexpr void SipRound(u64& v0, u64& v1, u64& v2, u64& v3) noexcept
{
    v0 += v1;
    v1 = RotateLeft64(v1, 13U);
    v1 ^= v0;
    v0 = RotateLeft64(v0, 32U);
    v2 += v3;
    v3 = RotateLeft64(v3, 16U);
    v3 ^= v2;
    v0 += v3;
    v3 = RotateLeft64(v3, 21U);
    v3 ^= v0;
    v2 += v1;
    v1 = RotateLeft64(v1, 17U);
    v1 ^= v2;
    v2 = RotateLeft64(v2, 32U);
}

[[nodiscard]] constexpr u64 SipHash24(ByteSpan data, u64 key0, u64 key1) noexcept
{
    u64 v0 = 0x736f6d6570736575ULL ^ key0;
    u64 v1 = 0x646f72616e646f6dULL ^ key1;
    u64 v2 = 0x6c7967656e657261ULL ^ key0;
    u64 v3 = 0x7465646279746573ULL ^ key1;

    usize offset = 0;
    const usize size = data.Size();
    while ((offset + 8U) <= size) {
        const u64 message = ReadLe64(data, offset);
        offset += 8U;

        v3 ^= message;
        SipRound(v0, v1, v2, v3);
        SipRound(v0, v1, v2, v3);
        v0 ^= message;
    }

    u64 tail = static_cast<u64>(size) << 56U;
    usize shift = 0;
    while (offset < size) {
        tail |= static_cast<u64>(ByteAt(data, offset)) << shift;
        ++offset;
        shift += 8U;
    }

    v3 ^= tail;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    v0 ^= tail;
    v2 ^= 0xffULL;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);

    return v0 ^ v1 ^ v2 ^ v3;
}

} // namespace zcore::hash::detail
