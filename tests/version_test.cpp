/**************************************************************************/
/*  version_test.cpp                                                      */
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
 * @file tests/version_test.cpp
 * @brief Unit tests for deterministic semantic version contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/version.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>
#include <unordered_set>

namespace {

static_assert(sizeof(zcore::Version) == sizeof(std::uint32_t) * 3U);
static_assert(std::is_trivially_copyable_v<zcore::Version>);

TEST(VersionTest, DefaultConstructedIsZero) {
  const zcore::Version version;
  EXPECT_TRUE(version.IsZero());
  EXPECT_TRUE(version.IsPreStable());
  EXPECT_EQ(version.Major(), 0U);
  EXPECT_EQ(version.Minor(), 0U);
  EXPECT_EQ(version.Patch(), 0U);
}

TEST(VersionTest, ExplicitConstructionAndRawFactoryPreserveParts) {
  const zcore::Version direct(1U, 2U, 3U);
  const zcore::Version factory = zcore::Version::FromRawUnchecked(4U, 5U, 6U);

  EXPECT_EQ(direct.Major(), 1U);
  EXPECT_EQ(direct.Minor(), 2U);
  EXPECT_EQ(direct.Patch(), 3U);
  EXPECT_EQ(factory.Major(), 4U);
  EXPECT_EQ(factory.Minor(), 5U);
  EXPECT_EQ(factory.Patch(), 6U);
}

TEST(VersionTest, LexicographicComparisonMatchesSemanticPartOrder) {
  const zcore::Version v100(1U, 0U, 0U);
  const zcore::Version v120(1U, 2U, 0U);
  const zcore::Version v121(1U, 2U, 1U);
  const zcore::Version v200(2U, 0U, 0U);

  EXPECT_LT(v100, v120);
  EXPECT_LT(v120, v121);
  EXPECT_LT(v121, v200);
}

TEST(VersionTest, BumpOperationsResetLowerPriorityParts) {
  zcore::Version version(3U, 4U, 5U);

  version.BumpPatch();
  EXPECT_EQ(version, zcore::Version(3U, 4U, 6U));

  version.BumpMinor();
  EXPECT_EQ(version, zcore::Version(3U, 5U, 0U));

  version.BumpMajor();
  EXPECT_EQ(version, zcore::Version(4U, 0U, 0U));
}

TEST(VersionTest, SetOperationsAndResetAreDeterministic) {
  zcore::Version version(2U, 6U, 9U);

  version.SetPatch(10U);
  EXPECT_EQ(version, zcore::Version(2U, 6U, 10U));

  version.SetMinor(7U);
  EXPECT_EQ(version, zcore::Version(2U, 7U, 0U));

  version.SetMajor(5U);
  EXPECT_EQ(version, zcore::Version(5U, 0U, 0U));
  EXPECT_FALSE(version.IsPreStable());

  version.Reset();
  EXPECT_EQ(version, zcore::Version::Zero());
}

TEST(VersionTest, HashSupportsUnorderedContainers) {
  std::unordered_set<zcore::Version> versions;
  versions.insert(zcore::Version(1U, 0U, 0U));
  versions.insert(zcore::Version(1U, 1U, 0U));
  versions.insert(zcore::Version(1U, 1U, 0U));

  EXPECT_EQ(versions.size(), 2U);
  EXPECT_EQ(versions.count(zcore::Version(1U, 1U, 0U)), 1U);
}

}  // namespace
