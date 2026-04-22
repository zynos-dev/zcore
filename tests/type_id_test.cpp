/**************************************************************************/
/*  type_id_test.cpp                                                      */
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
 * @file tests/type_id_test.cpp
 * @brief Unit tests for deterministic type identity behavior.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/type_id.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>
#include <unordered_set>

namespace {

struct UserType final {};
struct AssetType final {};

static_assert(sizeof(zcore::TypeId) == sizeof(std::uint64_t));
static_assert(std::is_trivially_copyable_v<zcore::TypeId>);
static_assert(zcore::TypeId::Of<int>().IsValid());
static_assert(zcore::TypeId::Of<int>() == zcore::TypeId::Of<int>());
static_assert(zcore::TypeId::Of<int>() != zcore::TypeId::Invalid());

TEST(TypeIdTest, DefaultConstructedIsInvalid) {
  const zcore::TypeId id;
  EXPECT_TRUE(id.IsInvalid());
  EXPECT_FALSE(id.IsValid());
  EXPECT_FALSE(static_cast<bool>(id));
  EXPECT_EQ(id.Value(), zcore::TypeId::kInvalidValue);
}

TEST(TypeIdTest, RawConstructionAndAccessAreExplicit) {
  const zcore::TypeId id(42ULL);
  EXPECT_TRUE(id.IsValid());
  EXPECT_EQ(id.Value(), 42ULL);
}

TEST(TypeIdTest, InvalidFactoryMatchesDefaultState) {
  const zcore::TypeId defaultId;
  const zcore::TypeId invalid = zcore::TypeId::Invalid();
  EXPECT_EQ(defaultId, invalid);
}

TEST(TypeIdTest, ResetReturnsToInvalidState) {
  zcore::TypeId id(9ULL);
  ASSERT_TRUE(id.IsValid());

  id.Reset();
  EXPECT_TRUE(id.IsInvalid());
  EXPECT_EQ(id.Value(), zcore::TypeId::kInvalidValue);
}

TEST(TypeIdTest, OfIsDeterministicForExactType) {
  constexpr zcore::TypeId kFirst = zcore::TypeId::Of<UserType>();
  constexpr zcore::TypeId kSecond = zcore::TypeId::Of<UserType>();
  EXPECT_EQ(kFirst, kSecond);
}

TEST(TypeIdTest, OfDiffersForDistinctTypes) {
  constexpr zcore::TypeId kUserId = zcore::TypeId::Of<UserType>();
  constexpr zcore::TypeId kAssetId = zcore::TypeId::Of<AssetType>();
  EXPECT_NE(kUserId, kAssetId);
}

TEST(TypeIdTest, FromRawUncheckedPreservesRawValue) {
  const zcore::TypeId id = zcore::TypeId::FromRawUnchecked(77ULL);
  EXPECT_EQ(id.Value(), 77ULL);
}

TEST(TypeIdTest, HashWorksForUnorderedContainers) {
  std::unordered_set<zcore::TypeId> ids;
  ids.insert(zcore::TypeId::Of<int>());
  ids.insert(zcore::TypeId::Of<float>());
  ids.insert(zcore::TypeId::Of<float>());

  EXPECT_EQ(ids.size(), 2U);
  EXPECT_EQ(ids.count(zcore::TypeId::Of<float>()), 1U);
}

}  // namespace
