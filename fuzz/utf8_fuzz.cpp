/**************************************************************************/
/*  utf8_fuzz.cpp                                                         */
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
 * @file fuzz/utf8_fuzz.cpp
 * @brief Local libFuzzer harness for UTF-8 invariants.
 */

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <zcore/utf8.hpp>

namespace {

[[noreturn]] void FuzzAbort() noexcept
{
    std::abort();
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    const char* bytes = reinterpret_cast<const char*>(data);
    const zcore::usize byteCount = static_cast<zcore::usize>(size);

    const bool isValid = zcore::utf8::IsValid(bytes, byteCount);
    const zcore::Option<zcore::usize> counted = zcore::utf8::CountCodePoints(bytes, byteCount);
    if (isValid != counted.HasValue()) {
        FuzzAbort();
    }

    for (zcore::usize offset = 0U; offset <= byteCount; ++offset) {
        const bool isBoundary = zcore::utf8::IsCodePointBoundary(bytes, byteCount, offset);
        const zcore::Option<zcore::usize> stay = zcore::utf8::AdvanceCodePoints(bytes, byteCount, offset, 0U);
        if (isBoundary != stay.HasValue()) {
            FuzzAbort();
        }
        if (stay.HasValue() && stay.Value() != offset) {
            FuzzAbort();
        }
    }

    if (!isValid) {
        const zcore::usize probe = byteCount == 0U ? 0U : static_cast<zcore::usize>(data[0]) % byteCount;
        (void) zcore::utf8::DecodeAt(bytes, byteCount, probe);
        return 0;
    }

    zcore::usize offset = 0U;
    zcore::usize decodedCount = 0U;
    while (offset < byteCount) {
        const zcore::Option<zcore::utf8::DecodedCodePoint> decoded = zcore::utf8::DecodeAt(bytes, byteCount, offset);
        if (!decoded.HasValue()) {
            FuzzAbort();
        }
        if (!zcore::utf8::IsCodePointBoundary(bytes, byteCount, offset)) {
            FuzzAbort();
        }

        const zcore::Option<zcore::usize> next = zcore::utf8::AdvanceCodePoints(bytes, byteCount, offset, 1U);
        if (!next.HasValue()) {
            FuzzAbort();
        }
        if (next.Value() != offset + decoded.Value().width) {
            FuzzAbort();
        }

        char encoded[4] = {};
        const zcore::Option<zcore::usize> encodedWidth = zcore::utf8::EncodeCodePoint(decoded.Value().value, encoded, 4U);
        if (!encodedWidth.HasValue()) {
            FuzzAbort();
        }
        if (encodedWidth.Value() != decoded.Value().width) {
            FuzzAbort();
        }

        const zcore::Option<zcore::utf8::DecodedCodePoint> roundtrip = zcore::utf8::DecodeAt(encoded, encodedWidth.Value(), 0U);
        if (!roundtrip.HasValue()) {
            FuzzAbort();
        }
        if (roundtrip.Value().value != decoded.Value().value) {
            FuzzAbort();
        }
        if (roundtrip.Value().width != encodedWidth.Value()) {
            FuzzAbort();
        }

        offset = next.Value();
        ++decodedCount;
    }

    if (!zcore::utf8::IsCodePointBoundary(bytes, byteCount, byteCount)) {
        FuzzAbort();
    }
    if (!counted.HasValue()) {
        FuzzAbort();
    }
    if (counted.Value() != decodedCount) {
        FuzzAbort();
    }

    return 0;
}
