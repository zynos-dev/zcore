/**************************************************************************/
/*  hash_test.cpp                                                         */
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
 * @file tests/hash_test.cpp
 * @brief Unit tests for deterministic hash contracts and vectors.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/hash.hpp>

#include <gtest/gtest.h>

#include <array>

namespace {

[[nodiscard]] zcore::ByteSpan BytesOf(const char* text, zcore::usize size) {
  return zcore::AsBytes(zcore::Slice<const char>(text, size));
}

TEST(HashTest, Fnv1a64MatchesKnownVector) {
  constexpr const char kText[] = "hello";
  const auto bytes = BytesOf(kText, 5U);
  const zcore::hash::Hash64 value = zcore::hash::HashBytes(bytes, zcore::hash::HashAlgorithm::FNV1A_64);
  EXPECT_EQ(value, 0xa430d84680aabd0bULL);
}

TEST(HashTest, Xxh364MatchesKnownVectors) {
  constexpr const char kText[] = "hello";
  const auto bytes = BytesOf(kText, 5U);
  const auto noSeed = zcore::hash::HashBytes(bytes, zcore::hash::HashAlgorithm::XXH3_64);
  const auto seeded = zcore::hash::HashBytes(bytes, zcore::hash::HashAlgorithm::XXH3_64, 1234ULL);

  EXPECT_EQ(noSeed, 0x9555e8555c62dcfdULL);
  EXPECT_EQ(seeded, 0x1a8c5c755115631bULL);
}

TEST(HashTest, SipHash24MatchesKnownEmptyVector) {
  constexpr zcore::hash::SipHashKey kKey{
      .k0 = 0x0706050403020100ULL,
      .k1 = 0x0f0e0d0c0b0a0908ULL,
  };

  const zcore::ByteSpan empty;
  const zcore::hash::Hash64 value = zcore::hash::HashBytesKeyed(empty, kKey);
  EXPECT_EQ(value, 0x726fdb47dd0e0e31ULL);
}

TEST(HashTest, SeededHashesAreDeterministicAndConfigurable) {
  constexpr const char kText[] = "zcore";
  const auto bytes = BytesOf(kText, 5U);

  const auto a = zcore::hash::HashBytes(bytes, 1234ULL);
  const auto b = zcore::hash::HashBytes(bytes, 1234ULL);
  const auto c = zcore::hash::HashBytes(bytes, 5678ULL);

  EXPECT_EQ(a, b);
  EXPECT_NE(a, c);
}

TEST(HashTest, DefaultHashUsesXxh364) {
  constexpr const char kText[] = "zcore";
  const auto bytes = BytesOf(kText, 5U);
  EXPECT_EQ(zcore::hash::HashBytes(bytes),
            zcore::hash::HashBytes(bytes, zcore::hash::HashAlgorithm::XXH3_64));
}

TEST(HashTest, HashStringMatchesByteHash) {
  constexpr const char kText[] = "runtime";
  const zcore::Slice<const char> text(kText, 7U);

  const auto asString = zcore::hash::HashString(text, 9ULL);
  const auto asBytes = zcore::hash::HashBytes(zcore::AsBytes(text), 9ULL);
  EXPECT_EQ(asString, asBytes);
}

TEST(HashTest, HashValueSupportsIntegralTypes) {
  enum class Kind : std::uint8_t {
    A = 7,
  };

  const auto intHash = zcore::hash::HashValue<std::uint32_t>(123U);
  const auto enumHash = zcore::hash::HashValue<Kind>(Kind::A);
  EXPECT_NE(intHash, 0U);
  EXPECT_NE(enumHash, 0U);
}

TEST(HashTest, HashValueMatchesHashObjectForIntegralAndEnumTypes) {
  enum class Kind : std::uint16_t {
    A = 0x23A1U,
  };

  constexpr std::uint64_t kSeed = 0xC0FFEEULL;
  const auto valueDigest = zcore::hash::HashValue<std::uint32_t>(0xDEADBEEFU, kSeed);
  const auto objectDigest = zcore::hash::HashObject<std::uint32_t>(0xDEADBEEFU, kSeed);
  EXPECT_EQ(valueDigest, objectDigest);

  const auto enumValueDigest = zcore::hash::HashValue<Kind>(Kind::A, kSeed);
  const auto enumObjectDigest = zcore::hash::HashObject<Kind>(Kind::A, kSeed);
  EXPECT_EQ(enumValueDigest, enumObjectDigest);
}

TEST(HashTest, HashValueMatchesHashObjectAcrossSeedsAndIntegralKinds) {
  enum class Kind : std::uint32_t {
    A = 0xABCDEF12U,
  };

  constexpr std::array<std::uint64_t, 4> kSeeds{
      0ULL,
      1ULL,
      0xC0FFEEULL,
      0xDEADBEEF01234567ULL,
  };

  for (const std::uint64_t seed : kSeeds) {
    EXPECT_EQ(zcore::hash::HashValue<bool>(true, seed), zcore::hash::HashObject<bool>(true, seed));
    EXPECT_EQ(zcore::hash::HashValue<bool>(false, seed), zcore::hash::HashObject<bool>(false, seed));
    EXPECT_EQ(zcore::hash::HashValue<std::int64_t>(-42, seed), zcore::hash::HashObject<std::int64_t>(-42, seed));
    EXPECT_EQ(zcore::hash::HashValue<std::uint64_t>(0xF00DBAADULL, seed),
              zcore::hash::HashObject<std::uint64_t>(0xF00DBAADULL, seed));
    EXPECT_EQ(zcore::hash::HashValue<Kind>(Kind::A, seed), zcore::hash::HashObject<Kind>(Kind::A, seed));
  }
}

}  // namespace
