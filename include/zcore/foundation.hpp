/**************************************************************************/
/*  foundation.hpp                                                        */
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
 * @file include/zcore/foundation.hpp
 * @brief Foundational aliases and shared primitive types.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/foundation.hpp>
 * zcore::u64 value = 42ULL;
 * @endcode
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace zcore {

/// @name Unsigned Integer Aliases
/// @{
/// @brief 8-bit unsigned integer alias.
using u8 = std::uint8_t; ///< 8-bit unsigned integer.
/// @brief 16-bit unsigned integer alias.
using u16 = std::uint16_t; ///< 16-bit unsigned integer.
/// @brief 32-bit unsigned integer alias.
using u32 = std::uint32_t; ///< 32-bit unsigned integer.
/// @brief 64-bit unsigned integer alias.
using u64 = std::uint64_t; ///< 64-bit unsigned integer.
/// @}

/// @name Signed Integer Aliases
/// @{
/// @brief 8-bit signed integer alias.
using i8 = std::int8_t; ///< 8-bit signed integer.
/// @brief 16-bit signed integer alias.
using i16 = std::int16_t; ///< 16-bit signed integer.
/// @brief 32-bit signed integer alias.
using i32 = std::int32_t; ///< 32-bit signed integer.
/// @brief 64-bit signed integer alias.
using i64 = std::int64_t; ///< 64-bit signed integer.
/// @}

/// @name Pointer-Sized Aliases
/// @{
/// @brief Unsigned integer type large enough to hold object sizes.
using usize = std::size_t; ///< Unsigned size type.
/// @brief Signed integer type large enough to hold pointer differences.
using isize = std::ptrdiff_t; ///< Signed size/difference type.
/// @}

/// @brief Byte-sized storage unit alias used for raw memory views.
using Byte = std::byte; ///< Raw byte type.

} // namespace zcore
