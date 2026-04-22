/**************************************************************************/
/*  interface_id_test.cpp                                                 */
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
 * @file tests/interface_id_test.cpp
 * @brief Unit tests for deterministic interface id contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/interface_id.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>
#include <unordered_set>

namespace {

static_assert(sizeof(zcore::InterfaceId) == sizeof(std::uint64_t));
static_assert(std::is_trivially_copyable_v<zcore::InterfaceId>);
static_assert(zcore::InterfaceId::FromLiteral("zcore.io.reader.v1").IsValid());
static_assert(zcore::InterfaceId::FromLiteral("zcore.io.reader.v1") == zcore::InterfaceId::FromLiteral("zcore.io.reader.v1"));

TEST(InterfaceIdTest, DefaultConstructedIsInvalid) {
  const zcore::InterfaceId id;
  EXPECT_TRUE(id.IsInvalid());
  EXPECT_FALSE(id.IsValid());
  EXPECT_FALSE(static_cast<bool>(id));
  EXPECT_EQ(id.Value(), zcore::InterfaceId::kInvalidValue);
}

TEST(InterfaceIdTest, RawConstructionAndAccessAreExplicit) {
  const zcore::InterfaceId id(42ULL);
  EXPECT_TRUE(id.IsValid());
  EXPECT_EQ(id.Value(), 42ULL);
}

TEST(InterfaceIdTest, NameFactoriesAreDeterministic) {
  constexpr zcore::InterfaceId kLiteralA = zcore::InterfaceId::FromLiteral("zcore.io.reader.v1");
  constexpr zcore::InterfaceId kLiteralB = zcore::InterfaceId::FromLiteral("zcore.io.reader.v1");
  const zcore::InterfaceId runtime = zcore::InterfaceId::FromName("zcore.io.reader.v1");
  const zcore::InterfaceId other = zcore::InterfaceId::FromName("zcore.io.writer.v1");

  EXPECT_EQ(kLiteralA, kLiteralB);
  EXPECT_EQ(kLiteralA, runtime);
  EXPECT_NE(runtime, other);
}

TEST(InterfaceIdTest, EmptyNameProducesInvalidId) {
  const zcore::InterfaceId id = zcore::InterfaceId::FromName("");
  EXPECT_TRUE(id.IsInvalid());
}

TEST(InterfaceIdTest, ComparisonAndResetAreDeterministic) {
  const zcore::InterfaceId a = zcore::InterfaceId::FromName("zcore.plugin.loader.v1");
  const zcore::InterfaceId b = zcore::InterfaceId::FromName("zcore.plugin.loader.v2");
  EXPECT_NE(a, b);

  zcore::InterfaceId mutableId = a;
  mutableId.Reset();
  EXPECT_EQ(mutableId, zcore::InterfaceId::Invalid());
}

TEST(InterfaceIdTest, HashSupportsUnorderedContainers) {
  std::unordered_set<zcore::InterfaceId> ids;
  ids.insert(zcore::InterfaceId::FromName("zcore.io.reader.v1"));
  ids.insert(zcore::InterfaceId::FromName("zcore.io.writer.v1"));
  ids.insert(zcore::InterfaceId::FromName("zcore.io.writer.v1"));

  EXPECT_EQ(ids.size(), 2U);
  EXPECT_EQ(ids.count(zcore::InterfaceId::FromName("zcore.io.writer.v1")), 1U);
}

}  // namespace
