/**************************************************************************/
/*  type_info_test.cpp                                                    */
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
 * @file tests/type_info_test.cpp
 * @brief Unit tests for deterministic type metadata behavior.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/type_info.hpp>

#include <gtest/gtest.h>

#include <string>
#include <type_traits>
#include <unordered_set>

namespace {

struct UserType final {
  int Value;
};

struct AssetType final {
  double Value;
};

struct Align16Type final {
  alignas(16) unsigned char Bytes[16];
};

struct NonTrivialType final {
  std::string Value;
};

static_assert(std::is_trivially_copyable_v<UserType>);
static_assert(!std::is_trivially_copyable_v<NonTrivialType>);
static_assert(!std::is_trivially_destructible_v<NonTrivialType>);

TEST(TypeInfoTest, DefaultConstructedIsInvalid) {
  const zcore::TypeInfo info;
  EXPECT_TRUE(info.IsInvalid());
  EXPECT_FALSE(info.IsValid());
  EXPECT_FALSE(static_cast<bool>(info));
  EXPECT_EQ(info.Id(), zcore::TypeId::Invalid());
  EXPECT_TRUE(info.Name().empty());
  EXPECT_EQ(info.Size(), 0U);
  EXPECT_EQ(info.Alignment(), 0U);
  EXPECT_EQ(info.TraitBits(), 0U);
}

TEST(TypeInfoTest, InvalidFactoryMatchesDefaultState) {
  const zcore::TypeInfo defaultInfo;
  const zcore::TypeInfo invalid = zcore::TypeInfo::Invalid();
  EXPECT_EQ(defaultInfo, invalid);
}

TEST(TypeInfoTest, FromRawUncheckedPreservesMetadata) {
  const zcore::TypeInfo info = zcore::TypeInfo::FromRawUnchecked(
      zcore::TypeId(77ULL), "manual-type", 64U, 16U, true, false, true);

  EXPECT_TRUE(info.IsValid());
  EXPECT_EQ(info.Id(), zcore::TypeId(77ULL));
  EXPECT_EQ(info.Name(), "manual-type");
  EXPECT_EQ(info.Size(), 64U);
  EXPECT_EQ(info.Alignment(), 16U);
  EXPECT_TRUE(info.IsTriviallyCopyable());
  EXPECT_FALSE(info.IsTriviallyDestructible());
  EXPECT_TRUE(info.IsTriviallyDefaultConstructible());
  EXPECT_EQ(info.TraitBits(), static_cast<zcore::u8>(0x5U));
}

TEST(TypeInfoTest, ResetReturnsToInvalidState) {
  zcore::TypeInfo info = zcore::TypeInfo::FromRawUnchecked(
      zcore::TypeId(9ULL), "reset-me", 8U, 8U, true, true, true);
  ASSERT_TRUE(info.IsValid());

  info.Reset();
  EXPECT_TRUE(info.IsInvalid());
  EXPECT_EQ(info, zcore::TypeInfo::Invalid());
}

TEST(TypeInfoTest, OfIsDeterministicForExactType) {
  constexpr zcore::TypeInfo kFirst = zcore::TypeInfo::Of<UserType>();
  constexpr zcore::TypeInfo kSecond = zcore::TypeInfo::Of<UserType>();
  EXPECT_EQ(kFirst, kSecond);
}

TEST(TypeInfoTest, OfDiffersForDistinctTypes) {
  constexpr zcore::TypeInfo kUserInfo = zcore::TypeInfo::Of<UserType>();
  constexpr zcore::TypeInfo kAssetInfo = zcore::TypeInfo::Of<AssetType>();
  EXPECT_NE(kUserInfo, kAssetInfo);
  EXPECT_NE(kUserInfo.Id(), kAssetInfo.Id());
}

TEST(TypeInfoTest, OfCarriesExpectedMetadataAndTraits) {
  constexpr zcore::TypeInfo kInfo = zcore::TypeInfo::Of<Align16Type>();
  EXPECT_TRUE(kInfo.IsValid());
  EXPECT_EQ(kInfo.Id(), zcore::TypeId::Of<Align16Type>());
  EXPECT_FALSE(kInfo.Name().empty());
  EXPECT_EQ(kInfo.Size(), sizeof(Align16Type));
  EXPECT_EQ(kInfo.Alignment(), alignof(Align16Type));
  EXPECT_TRUE(kInfo.IsTriviallyCopyable());
  EXPECT_TRUE(kInfo.IsTriviallyDestructible());
  EXPECT_TRUE(kInfo.IsTriviallyDefaultConstructible());
}

TEST(TypeInfoTest, OfReflectsNonTrivialTypeTraits) {
  constexpr zcore::TypeInfo kInfo = zcore::TypeInfo::Of<NonTrivialType>();
  EXPECT_EQ(kInfo.IsTriviallyCopyable(), std::is_trivially_copyable_v<NonTrivialType>);
  EXPECT_EQ(kInfo.IsTriviallyDestructible(), std::is_trivially_destructible_v<NonTrivialType>);
  EXPECT_EQ(
      kInfo.IsTriviallyDefaultConstructible(),
      std::is_trivially_default_constructible_v<NonTrivialType>);
}

TEST(TypeInfoTest, HashWorksForUnorderedContainers) {
  std::unordered_set<zcore::TypeInfo> infos;
  infos.insert(zcore::TypeInfo::Of<int>());
  infos.insert(zcore::TypeInfo::Of<float>());
  infos.insert(zcore::TypeInfo::Of<float>());

  EXPECT_EQ(infos.size(), 2U);
  EXPECT_EQ(infos.count(zcore::TypeInfo::Of<float>()), 1U);
}

}  // namespace
