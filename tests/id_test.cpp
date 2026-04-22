/**************************************************************************/
/*  id_test.cpp                                                           */
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
 * @file tests/id_test.cpp
 * @brief Unit tests for strong typed ID behavior.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/id.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>
#include <unordered_set>

namespace {

struct UserTag final {};
struct AssetTag final {};

using UserId = zcore::Id<UserTag>;
using AssetId = zcore::Id<AssetTag>;

static_assert(sizeof(UserId) == sizeof(std::uint64_t));
static_assert(std::is_trivially_copyable_v<UserId>);
static_assert(!std::is_convertible_v<UserId, AssetId>);
static_assert(!std::is_constructible_v<UserId, AssetId>);

TEST(IdTest, DefaultConstructedIsInvalid) {
  const UserId id;
  EXPECT_FALSE(id.IsValid());
  EXPECT_TRUE(id.IsInvalid());
  EXPECT_FALSE(static_cast<bool>(id));
  EXPECT_EQ(id.Raw(), UserId::kInvalidValue);
}

TEST(IdTest, RawConstructionAndAccessAreExplicit) {
  const UserId id(42);
  EXPECT_TRUE(id.IsValid());
  EXPECT_EQ(id.Raw(), 42U);
  EXPECT_EQ(static_cast<std::uint64_t>(id), 42U);
}

TEST(IdTest, InvalidFactoryMatchesDefaultState) {
  const UserId defaultId;
  const UserId invalid = UserId::Invalid();
  EXPECT_EQ(defaultId, invalid);
}

TEST(IdTest, ResetReturnsToInvalidState) {
  UserId id(9);
  ASSERT_TRUE(id.IsValid());

  id.Reset();
  EXPECT_TRUE(id.IsInvalid());
  EXPECT_EQ(id.Raw(), UserId::kInvalidValue);
}

TEST(IdTest, ComparisonIsTagScopedAndDeterministic) {
  const UserId a(7);
  const UserId b(9);
  const UserId c(9);

  EXPECT_LT(a, b);
  EXPECT_EQ(b, c);
}

TEST(IdTest, HashWorksForUnorderedContainers) {
  std::unordered_set<UserId> ids;
  ids.insert(UserId(1));
  ids.insert(UserId(2));
  ids.insert(UserId(2));

  EXPECT_EQ(ids.size(), 2U);
  EXPECT_EQ(ids.count(UserId(2)), 1U);
}

TEST(IdTest, CustomValueTypeAndInvalidSentinelAreSupported) {
  using SessionId = zcore::Id<struct SessionTag, std::uint32_t, 0xFFFFFFFFu>;

  const SessionId invalid;
  EXPECT_FALSE(invalid.IsValid());
  EXPECT_EQ(invalid.Raw(), 0xFFFFFFFFu);

  const SessionId valid(17u);
  EXPECT_TRUE(valid.IsValid());
  EXPECT_EQ(valid.Raw(), 17u);
}

}  // namespace
