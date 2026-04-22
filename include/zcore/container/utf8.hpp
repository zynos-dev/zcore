/**************************************************************************/
/*  utf8.hpp                                                              */
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
 * @file include/zcore/container/utf8.hpp
 * @brief UTF-8 validation and decoding utilities for byte-oriented text APIs.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/utf8.hpp>
 * bool ok = zcore::utf8::IsValid("zcore", 5U);
 * @endcode
 */

#pragma once

#include <zcore/foundation.hpp>
#include <zcore/option.hpp>

namespace zcore::utf8 {

/**
 * @brief Decoded UTF-8 code point payload.
 */
struct DecodedCodePoint final {
    /// @brief Unicode scalar value.
    u32 value;
    /// @brief Number of bytes consumed from source.
    usize width;
};

namespace detail {

[[nodiscard]] constexpr u8 ToByte(char value) noexcept
{
    return static_cast<u8>(static_cast<unsigned char>(value));
}

[[nodiscard]] constexpr bool IsContinuation(u8 value) noexcept
{
    return (value & 0xC0U) == 0x80U;
}

[[nodiscard]] constexpr bool IsValidScalarValue(u32 value) noexcept
{
    if (value > 0x10FFFFU) {
        return false;
    }
    return value < 0xD800U || value > 0xDFFFU;
}

[[nodiscard]] constexpr Option<DecodedCodePoint> DecodeAtUnchecked(const char* data, usize size, usize offset) noexcept
{
    if (offset >= size || data == nullptr) {
        return None;
    }

    const u8 lead = ToByte(data[offset]);

    if (lead <= 0x7FU) {
        return Option<DecodedCodePoint>(DecodedCodePoint{
                .value = static_cast<u32>(lead),
                .width = 1U,
        });
    }

    if (lead >= 0xC2U && lead <= 0xDFU) {
        if (offset + 1U >= size) {
            return None;
        }
        const u8 b1 = ToByte(data[offset + 1U]);
        if (!IsContinuation(b1)) {
            return None;
        }
        const u32 cp = (static_cast<u32>(lead & 0x1FU) << 6U) | static_cast<u32>(b1 & 0x3FU);
        return Option<DecodedCodePoint>(DecodedCodePoint{
                .value = cp,
                .width = 2U,
        });
    }

    if (lead >= 0xE0U && lead <= 0xEFU) {
        if (offset + 2U >= size) {
            return None;
        }
        const u8 b1 = ToByte(data[offset + 1U]);
        const u8 b2 = ToByte(data[offset + 2U]);
        if (!IsContinuation(b1) || !IsContinuation(b2)) {
            return None;
        }
        if (lead == 0xE0U && b1 < 0xA0U) {
            return None;
        }
        if (lead == 0xEDU && b1 > 0x9FU) {
            return None;
        }
        const u32 cp =
                (static_cast<u32>(lead & 0x0FU) << 12U) | (static_cast<u32>(b1 & 0x3FU) << 6U) | static_cast<u32>(b2 & 0x3FU);
        if (!IsValidScalarValue(cp)) {
            return None;
        }
        return Option<DecodedCodePoint>(DecodedCodePoint{
                .value = cp,
                .width = 3U,
        });
    }

    if (lead >= 0xF0U && lead <= 0xF4U) {
        if (offset + 3U >= size) {
            return None;
        }
        const u8 b1 = ToByte(data[offset + 1U]);
        const u8 b2 = ToByte(data[offset + 2U]);
        const u8 b3 = ToByte(data[offset + 3U]);
        if (!IsContinuation(b1) || !IsContinuation(b2) || !IsContinuation(b3)) {
            return None;
        }
        if (lead == 0xF0U && b1 < 0x90U) {
            return None;
        }
        if (lead == 0xF4U && b1 > 0x8FU) {
            return None;
        }
        const u32 cp = (static_cast<u32>(lead & 0x07U) << 18U) | (static_cast<u32>(b1 & 0x3FU) << 12U)
                       | (static_cast<u32>(b2 & 0x3FU) << 6U) | static_cast<u32>(b3 & 0x3FU);
        if (!IsValidScalarValue(cp)) {
            return None;
        }
        return Option<DecodedCodePoint>(DecodedCodePoint{
                .value = cp,
                .width = 4U,
        });
    }

    return None;
}

} // namespace detail

/**
 * @brief Decodes one code point at `offset`.
 * @param data UTF-8 bytes.
 * @param size Byte length.
 * @param offset Start offset.
 * @return Decoded code point and width, or `None` when invalid/out-of-range.
 */
[[nodiscard]] constexpr Option<DecodedCodePoint> DecodeAt(const char* data, usize size, usize offset) noexcept
{
    return detail::DecodeAtUnchecked(data, size, offset);
}

/**
 * @brief Returns whether `offset` is a UTF-8 code point boundary.
 * @param data UTF-8 bytes.
 * @param size Byte length.
 * @param offset Candidate byte offset.
 * @return `true` when offset is at start/end or at a leading byte position.
 */
[[nodiscard]] constexpr bool IsCodePointBoundary(const char* data, usize size, usize offset) noexcept
{
    if (offset > size) {
        return false;
    }
    if (data == nullptr) {
        return size == 0U && offset == 0U;
    }
    if (offset == 0U || offset == size) {
        return true;
    }
    const u8 value = detail::ToByte(data[offset]);
    return !detail::IsContinuation(value);
}

/**
 * @brief Advances by `count` UTF-8 code points from `offset`.
 * @param data UTF-8 bytes.
 * @param size Byte length.
 * @param offset Starting byte offset (must be a code point boundary).
 * @param count Number of code points to advance.
 * @return Advanced byte offset, or `None` on invalid UTF-8/boundary/range.
 */
[[nodiscard]] constexpr Option<usize> AdvanceCodePoints(const char* data, usize size, usize offset, usize count) noexcept
{
    if (offset > size) {
        return None;
    }
    if (count == 0U) {
        return IsCodePointBoundary(data, size, offset) ? Option<usize>(offset) : None;
    }
    if (data == nullptr || !IsCodePointBoundary(data, size, offset)) {
        return None;
    }

    usize cursor = offset;
    for (usize index = 0U; index < count; ++index) {
        Option<DecodedCodePoint> decoded = detail::DecodeAtUnchecked(data, size, cursor);
        if (!decoded.HasValue()) {
            return None;
        }
        cursor += decoded.Value().width;
    }
    return Option<usize>(cursor);
}

/**
 * @brief Encodes one Unicode scalar value to UTF-8 bytes.
 * @param value Unicode scalar value.
 * @param out Destination byte buffer.
 * @param capacity Destination buffer capacity.
 * @return Encoded byte width, or `None` when value is invalid or capacity is insufficient.
 */
[[nodiscard]] constexpr Option<usize> EncodeCodePoint(u32 value, char* out, usize capacity) noexcept
{
    if (!detail::IsValidScalarValue(value) || out == nullptr) {
        return None;
    }

    if (value <= 0x7FU) {
        if (capacity < 1U) {
            return None;
        }
        out[0] = static_cast<char>(value);
        return Option<usize>(1U);
    }

    if (value <= 0x7FFU) {
        if (capacity < 2U) {
            return None;
        }
        out[0] = static_cast<char>(0xC0U | ((value >> 6U) & 0x1FU));
        out[1] = static_cast<char>(0x80U | (value & 0x3FU));
        return Option<usize>(2U);
    }

    if (value <= 0xFFFFU) {
        if (capacity < 3U) {
            return None;
        }
        out[0] = static_cast<char>(0xE0U | ((value >> 12U) & 0x0FU));
        out[1] = static_cast<char>(0x80U | ((value >> 6U) & 0x3FU));
        out[2] = static_cast<char>(0x80U | (value & 0x3FU));
        return Option<usize>(3U);
    }

    if (value <= 0x10FFFFU) {
        if (capacity < 4U) {
            return None;
        }
        out[0] = static_cast<char>(0xF0U | ((value >> 18U) & 0x07U));
        out[1] = static_cast<char>(0x80U | ((value >> 12U) & 0x3FU));
        out[2] = static_cast<char>(0x80U | ((value >> 6U) & 0x3FU));
        out[3] = static_cast<char>(0x80U | (value & 0x3FU));
        return Option<usize>(4U);
    }

    return None;
}

/**
 * @brief Validates UTF-8 byte sequence.
 * @param data UTF-8 bytes.
 * @param size Byte length.
 * @return `true` when valid UTF-8.
 */
[[nodiscard]] constexpr bool IsValid(const char* data, usize size) noexcept
{
    if (size == 0U) {
        return true;
    }
    if (data == nullptr) {
        return false;
    }

    usize offset = 0U;
    while (offset < size) {
        Option<DecodedCodePoint> decoded = detail::DecodeAtUnchecked(data, size, offset);
        if (!decoded.HasValue()) {
            return false;
        }
        offset += decoded.Value().width;
    }
    return true;
}

/**
 * @brief Counts Unicode code points in UTF-8 bytes.
 * @param data UTF-8 bytes.
 * @param size Byte length.
 * @return Number of code points, or `None` when invalid UTF-8.
 */
[[nodiscard]] constexpr Option<usize> CountCodePoints(const char* data, usize size) noexcept
{
    if (!IsValid(data, size)) {
        return None;
    }

    usize offset = 0U;
    usize count = 0U;
    while (offset < size) {
        Option<DecodedCodePoint> decoded = detail::DecodeAtUnchecked(data, size, offset);
        if (!decoded.HasValue()) {
            return None;
        }
        offset += decoded.Value().width;
        ++count;
    }
    return Option<usize>(count);
}

} // namespace zcore::utf8
