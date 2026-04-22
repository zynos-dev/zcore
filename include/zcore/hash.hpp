/**************************************************************************/
/*  hash.hpp                                                              */
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
 * @file include/zcore/hash.hpp
 * @brief Deterministic non-cryptographic and keyed hashing API.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/hash.hpp>
 * const zcore::hash::Hash64 digest = zcore::hash::HashBytes(bytes);
 * @endcode
 */

#pragma once

#include <type_traits>
#include <zcore/foundation.hpp>
#include <zcore/hash/customization.hpp>
#include <zcore/hash/detail/common.hpp>
#include <zcore/hash/detail/fnv1a64.hpp>
#include <zcore/hash/detail/siphash24.hpp>
#include <zcore/hash/detail/xxh3_runtime.hpp>
#include <zcore/slice.hpp>

namespace zcore::hash {

/**
 * @brief Hash algorithms exposed by `HashBytes`.
 */
enum class HashAlgorithm : u8 {
    /// @brief xxHash3 64-bit (fast non-cryptographic default).
    XXH3_64 = 0,
    /// @brief FNV-1a 64-bit (simple deterministic baseline).
    FNV1A_64,
    /// @brief SipHash-2-4 keyed variant.
    SIPHASH_2_4,
};

/**
 * @brief 128-bit key material used by SipHash-2-4.
 */
struct SipHashKey final {
    /// @brief First 64-bit key lane.
    u64 k0;
    /// @brief Second 64-bit key lane.
    u64 k1;

    /// @brief Value equality comparison for keys.
    [[nodiscard]] constexpr bool operator==(const SipHashKey&) const noexcept = default;

    /**
   * @brief Returns deterministic process-default SipHash key.
   */
    [[nodiscard]] static constexpr SipHashKey Default() noexcept
    {
        return SipHashKey{
                .k0 = 0x6f635f687361685fULL,
                .k1 = 0x7a636f72655f6b79ULL,
        };
    }

    /**
   * @brief Derives SipHash key material from a 64-bit seed.
   * @param seed Seed value.
   * @return Derived key pair.
   */
    [[nodiscard]] static constexpr SipHashKey FromSeed(u64 seed) noexcept;
};

namespace detail {

[[nodiscard]] inline Hash64 Fnv1a64Scalar(ByteSpan data, u64 seed) noexcept
{
    return Fnv1a64(data, seed);
}

[[nodiscard]] inline Hash64 SipHash24Seeded(ByteSpan data, u64 seed) noexcept
{
    const SipHashKey key = SipHashKey::FromSeed(seed);
    return SipHash24(data, key.k0, key.k1);
}

[[nodiscard]] inline Hash64 HashBytesDispatched(ByteSpan data, HashAlgorithm algorithm, u64 seed) noexcept
{
    switch (algorithm) {
    case HashAlgorithm::XXH3_64:
        return Xxh364(data, seed);
    case HashAlgorithm::FNV1A_64:
        return Fnv1a64Scalar(data, seed);
    case HashAlgorithm::SIPHASH_2_4:
        return SipHash24Seeded(data, seed);
    }
    return Xxh364(data, seed);
}

} // namespace detail

[[nodiscard]] constexpr SipHashKey SipHashKey::FromSeed(u64 seed) noexcept
{
    u64 state = seed;
    return SipHashKey{
            .k0 = detail::SplitMix64Step(state),
            .k1 = detail::SplitMix64Step(state),
    };
}

/**
 * @brief Hashes arbitrary bytes with algorithm and seed selection.
 * @param data Input byte view.
 * @param algorithm Algorithm selector.
 * @param seed Seed value (algorithm-specific semantics).
 * @return 64-bit digest.
 */
[[nodiscard]] inline Hash64 HashBytes(ByteSpan data, HashAlgorithm algorithm, u64 seed) noexcept
{
    return detail::HashBytesDispatched(data, algorithm, seed);
}

/**
 * @brief Hashes arbitrary bytes with algorithm selector and zero seed.
 */
[[nodiscard]] inline Hash64 HashBytes(ByteSpan data, HashAlgorithm algorithm) noexcept
{
    return HashBytes(data, algorithm, 0ULL);
}

/**
 * @brief Hashes bytes with default algorithm (`XXH3_64`) and caller seed.
 */
[[nodiscard]] inline Hash64 HashBytes(ByteSpan data, u64 seed) noexcept
{
    return HashBytes(data, HashAlgorithm::XXH3_64, seed);
}

/**
 * @brief Hashes bytes with default algorithm (`XXH3_64`) and zero seed.
 */
[[nodiscard]] inline Hash64 HashBytes(ByteSpan data) noexcept
{
    return HashBytes(data, HashAlgorithm::XXH3_64, 0ULL);
}

/**
 * @brief Computes keyed SipHash-2-4 digest.
 * @param data Input bytes.
 * @param key Explicit SipHash key.
 * @return 64-bit digest.
 */
[[nodiscard]] constexpr Hash64 HashBytesKeyed(ByteSpan data, SipHashKey key) noexcept
{
    return detail::SipHash24(data, key.k0, key.k1);
}

/**
 * @brief Hashes text using default algorithm (`XXH3_64`).
 * @param text Input text slice.
 * @param seed Optional seed.
 */
[[nodiscard]] inline Hash64 HashString(Slice<const char> text, u64 seed = 0ULL) noexcept
{
    return HashBytes(AsBytes(text), seed);
}

/**
 * @brief Hashes text using an explicit algorithm.
 * @param text Input text slice.
 * @param algorithm Algorithm selector.
 * @param seed Optional seed.
 */
[[nodiscard]] inline Hash64 HashString(Slice<const char> text, HashAlgorithm algorithm, u64 seed = 0ULL) noexcept
{
    return HashBytes(AsBytes(text), algorithm, seed);
}

/**
 * @brief Hashes integral or enum value through the canonical `Hash<ValueT>` customization path.
 * @tparam ValueT Integral or enum type.
 * @param value Input value.
 * @param seed Optional seed.
 * @return 64-bit digest.
 */
template <typename ValueT>
    requires(std::is_integral_v<ValueT> || std::is_enum_v<ValueT>)
[[nodiscard]] inline Hash64 HashValue(ValueT value, u64 seed = 0ULL) noexcept
{
    return HashObject(value, seed);
}

} // namespace zcore::hash
