/**************************************************************************/
/*  hash_customization_test.cpp                                           */
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
 * @file tests/hash_customization_test.cpp
 * @brief Unit tests for `zcore::hash` type-level customization contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/hash.hpp>
#include <zcore/id.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <functional>

namespace {

enum class StatusCode : std::uint8_t {
  OK = 0x11,
};

struct EntityTag final {};
using EntityId = zcore::Id<EntityTag>;

TEST(HashCustomizationTest, IntegralAndEnumHashesAreDeterministic) {
  const auto a = zcore::hash::Hash<std::uint32_t>{}(0xA55A13U, 42ULL);
  const auto b = zcore::hash::Hash<std::uint32_t>{}(0xA55A13U, 42ULL);
  const auto c = zcore::hash::Hash<std::uint32_t>{}(0xA55A13U, 43ULL);
  EXPECT_EQ(a, b);
  EXPECT_NE(a, c);

  const auto enumHash = zcore::hash::Hash<StatusCode>{}(StatusCode::OK, 7ULL);
  EXPECT_NE(enumHash, 0ULL);
}

TEST(HashCustomizationTest, IntegralAndEnumHashMatchFnvByteHashContract) {
  constexpr std::uint64_t kSeed = 0x1234ABCDULL;

  constexpr std::uint32_t kValue = 0xA55A13C7U;
  constexpr std::array<zcore::Byte, sizeof(kValue)> kValueLeBytes{
      zcore::Byte{0xC7U},
      zcore::Byte{0x13U},
      zcore::Byte{0x5AU},
      zcore::Byte{0xA5U},
  };

  const auto valueDigest = zcore::hash::Hash<std::uint32_t>{}(kValue, kSeed);
  const auto valueBytesDigest = zcore::hash::HashBytes(
      zcore::Slice<const zcore::Byte>(kValueLeBytes.data(), kValueLeBytes.size()),
      zcore::hash::HashAlgorithm::FNV1A_64,
      kSeed);
  EXPECT_EQ(valueDigest, valueBytesDigest);

  constexpr std::uint8_t kEnumRaw = static_cast<std::uint8_t>(StatusCode::OK);
  constexpr std::array<zcore::Byte, sizeof(kEnumRaw)> kEnumLeBytes{
      zcore::Byte{kEnumRaw},
  };

  const auto enumDigest = zcore::hash::Hash<StatusCode>{}(StatusCode::OK, kSeed);
  const auto enumBytesDigest = zcore::hash::HashBytes(
      zcore::Slice<const zcore::Byte>(kEnumLeBytes.data(), kEnumLeBytes.size()),
      zcore::hash::HashAlgorithm::FNV1A_64,
      kSeed);
  EXPECT_EQ(enumDigest, enumBytesDigest);
}

TEST(HashCustomizationTest, HashObjectUsesTypeSpecialization) {
  const EntityId id(77ULL);
  const auto viaObject = zcore::hash::HashObject(id, 9ULL);
  const auto viaSpecialization = zcore::hash::Hash<EntityId>{}(id, 9ULL);
  EXPECT_EQ(viaObject, viaSpecialization);
}

TEST(HashCustomizationTest, StdHashAdapterUsesZcoreHashDigest) {
  const EntityId id(1234ULL);
  const auto digest = zcore::hash::Hash<EntityId>{}(id);
  const auto expected = zcore::hash::DigestToSizeT(digest);
  const auto adapted = std::hash<EntityId>{}(id);
  EXPECT_EQ(adapted, expected);
}

}  // namespace
