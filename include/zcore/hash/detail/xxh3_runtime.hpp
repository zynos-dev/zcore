/**************************************************************************/
/*  xxh3_runtime.hpp                                                      */
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
 * @file include/zcore/hash/detail/xxh3_runtime.hpp
 * @brief Runtime-dispatched XXH3 backend entry points.
 * @details Internal header; include <zcore/hash.hpp> for public APIs.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/hash.hpp>
 * auto digest = zcore::hash::HashBytes(bytes);
 * @endcode
 */

#pragma once

#include <zcore/foundation.hpp>
#include <zcore/hash/detail/xxhash_backend.hpp>
#include <zcore/slice.hpp>

namespace zcore::hash::detail {

[[nodiscard]] inline u64 Xxh364(ByteSpan data, u64 seed) noexcept
{
    return static_cast<u64>(XXH3_64bits_withSeed(data.Data(), data.Size(), seed));
}

} // namespace zcore::hash::detail
