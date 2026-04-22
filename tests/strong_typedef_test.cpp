/**************************************************************************/
/*  strong_typedef_test.cpp                                               */
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
 * @file tests/strong_typedef_test.cpp
 * @brief Unit tests for strong typedef contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/strong_typedef.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>
#include <unordered_set>

namespace {

struct UserTag final {};
struct AssetTag final {};

using UserToken = zcore::StrongTypedef<UserTag>;
using AssetToken = zcore::StrongTypedef<AssetTag>;

static_assert(sizeof(UserToken) == sizeof(std::uint64_t));
static_assert(std::is_trivially_copyable_v<UserToken>);
static_assert(!std::is_convertible_v<UserToken, AssetToken>);
static_assert(!std::is_constructible_v<UserToken, AssetToken>);

TEST(StrongTypedefTest, DefaultConstructedIsInvalid) {
  const UserToken value;
  EXPECT_TRUE(value.IsInvalid());
  EXPECT_FALSE(value.IsValid());
  EXPECT_FALSE(static_cast<bool>(value));
  EXPECT_EQ(value.Raw(), UserToken::kInvalidValue);
}

TEST(StrongTypedefTest, RawConstructionAndAccessAreExplicit) {
  const UserToken value(42ULL);
  EXPECT_TRUE(value.IsValid());
  EXPECT_EQ(value.Raw(), 42ULL);
  EXPECT_EQ(static_cast<std::uint64_t>(value), 42ULL);
}

TEST(StrongTypedefTest, InvalidFactoryAndResetAreDeterministic) {
  UserToken value(7ULL);
  ASSERT_TRUE(value.IsValid());

  value.Reset();
  EXPECT_EQ(value, UserToken::Invalid());
  EXPECT_TRUE(value.IsInvalid());
}

TEST(StrongTypedefTest, FromRawUncheckedPreservesRawValue) {
  const UserToken value = UserToken::FromRawUnchecked(99ULL);
  EXPECT_EQ(value.Raw(), 99ULL);
}

TEST(StrongTypedefTest, ComparisonIsTagScopedAndDeterministic) {
  const UserToken a(3ULL);
  const UserToken b(5ULL);
  const UserToken c(5ULL);
  EXPECT_LT(a, b);
  EXPECT_EQ(b, c);
}

TEST(StrongTypedefTest, HashSupportsUnorderedContainers) {
  std::unordered_set<UserToken> values;
  values.insert(UserToken(1ULL));
  values.insert(UserToken(2ULL));
  values.insert(UserToken(2ULL));

  EXPECT_EQ(values.size(), 2U);
  EXPECT_EQ(values.count(UserToken(2ULL)), 1U);
}

TEST(StrongTypedefTest, CustomValueTypeAndInvalidSentinelAreSupported) {
  using SessionToken = zcore::StrongTypedef<struct SessionTag, std::uint32_t, 0xFFFFFFFFU>;

  const SessionToken invalid;
  EXPECT_FALSE(invalid.IsValid());
  EXPECT_EQ(invalid.Raw(), 0xFFFFFFFFU);

  const SessionToken valid(17U);
  EXPECT_TRUE(valid.IsValid());
  EXPECT_EQ(valid.Raw(), 17U);
}

}  // namespace
