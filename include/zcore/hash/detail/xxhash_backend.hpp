/**************************************************************************/
/*  xxhash_backend.hpp                                                    */
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
 * @file include/zcore/hash/detail/xxhash_backend.hpp
 * @brief Internal bridge to vendored xxHash backend.
 * @details Internal header; include <zcore/hash.hpp> for public APIs.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/hash.hpp>
 * auto digest = zcore::hash::HashBytes(bytes);
 * @endcode
 */

#pragma once

#if defined(_MSC_VER)
  #pragma warning(push, 0)
#endif

#if defined(__clang__)
  #pragma clang system_header
#endif

#ifndef XXH_INLINE_ALL
  #define XXH_INLINE_ALL
  #define ZCORE_XXH_INLINE_ALL_DEFINED
#endif

#include <xxhash.h>

#ifdef ZCORE_XXH_INLINE_ALL_DEFINED
  #undef ZCORE_XXH_INLINE_ALL_DEFINED
  #undef XXH_INLINE_ALL
#endif

#if defined(_MSC_VER)
  #pragma warning(pop)
#endif
