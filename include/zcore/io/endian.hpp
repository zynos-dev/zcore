/**************************************************************************/
/*  endian.hpp                                                            */
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
 * @file include/zcore/io/endian.hpp
 * @brief Endian conversion helpers for serialization and I/O boundaries.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/endian.hpp>
 * const zcore::u32 network = zcore::endian::to_be(local);
 * @endcode
 */

#pragma once

#include <bit>
#include <type_traits>

namespace zcore::endian {

/**
 * @brief Value category supported by endian conversion helpers.
 */
template <typename ValueT>
concept EndianValue = (std::is_integral_v<ValueT> || std::is_enum_v<ValueT>) && !std::is_same_v<std::remove_cv_t<ValueT>, bool>;

/// @brief Returns `true` when native endianness is little-endian.
[[nodiscard]] constexpr bool IsNativeLittleEndian() noexcept
{
    return std::endian::native == std::endian::little;
}

/// @brief Returns `true` when native endianness is big-endian.
[[nodiscard]] constexpr bool IsNativeBigEndian() noexcept
{
    return std::endian::native == std::endian::big;
}

/**
 * @brief Byte-swaps integral or enum value.
 * @param value Source value.
 * @return Byte-swapped value (`sizeof(ValueT)==1` is unchanged).
 */
template <EndianValue ValueT>
[[nodiscard]] constexpr ValueT ByteSwap(ValueT value) noexcept
{
    if constexpr (std::is_enum_v<ValueT>) {
        using RawT = std::underlying_type_t<ValueT>;
        return static_cast<ValueT>(ByteSwap(static_cast<RawT>(value)));
    }
    else if constexpr (sizeof(ValueT) == 1U) {
        return value;
    }
    else {
        return std::byteswap(value);
    }
}

/**
 * @brief Converts native value to little-endian representation.
 */
template <EndianValue ValueT>
[[nodiscard]] constexpr ValueT ToLittleEndian(ValueT value) noexcept
{
    if constexpr (std::endian::native == std::endian::little) {
        return value;
    }
    else {
        return ByteSwap(value);
    }
}

/**
 * @brief Converts little-endian value to native representation.
 */
template <EndianValue ValueT>
[[nodiscard]] constexpr ValueT FromLittleEndian(ValueT value) noexcept
{
    return ToLittleEndian(value);
}

/**
 * @brief Converts native value to big-endian representation.
 */
template <EndianValue ValueT>
[[nodiscard]] constexpr ValueT ToBigEndian(ValueT value) noexcept
{
    if constexpr (std::endian::native == std::endian::big) {
        return value;
    }
    else {
        return ByteSwap(value);
    }
}

/**
 * @brief Converts big-endian value to native representation.
 */
template <EndianValue ValueT>
[[nodiscard]] constexpr ValueT FromBigEndian(ValueT value) noexcept
{
    return ToBigEndian(value);
}

/**
 * @brief Alias for `ToLittleEndian`.
 */
template <EndianValue ValueT>
// NOLINTNEXTLINE(readability-identifier-naming): snake_case aliases intentionally mirror conventional endian shorthand.
[[nodiscard]] constexpr ValueT to_le(ValueT value) noexcept
{
    return ToLittleEndian(value);
}

/**
 * @brief Alias for `FromLittleEndian`.
 */
template <EndianValue ValueT>
// NOLINTNEXTLINE(readability-identifier-naming): snake_case aliases intentionally mirror conventional endian shorthand.
[[nodiscard]] constexpr ValueT from_le(ValueT value) noexcept
{
    return FromLittleEndian(value);
}

/**
 * @brief Alias for `ToBigEndian`.
 */
template <EndianValue ValueT>
// NOLINTNEXTLINE(readability-identifier-naming): snake_case aliases intentionally mirror conventional endian shorthand.
[[nodiscard]] constexpr ValueT to_be(ValueT value) noexcept
{
    return ToBigEndian(value);
}

/**
 * @brief Alias for `FromBigEndian`.
 */
template <EndianValue ValueT>
// NOLINTNEXTLINE(readability-identifier-naming): snake_case aliases intentionally mirror conventional endian shorthand.
[[nodiscard]] constexpr ValueT from_be(ValueT value) noexcept
{
    return FromBigEndian(value);
}

} // namespace zcore::endian
