/**************************************************************************/
/*  abi_version_test.cpp                                                  */
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
 * @file tests/abi_version_test.cpp
 * @brief Unit tests for deterministic ABI version contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/abi_version.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>
#include <unordered_set>

namespace {

static_assert(sizeof(zcore::AbiVersion) == sizeof(std::uint32_t) * 2U);
static_assert(std::is_trivially_copyable_v<zcore::AbiVersion>);

TEST(AbiVersionTest, DefaultConstructedIsZero) {
  const zcore::AbiVersion version;
  EXPECT_TRUE(version.IsZero());
  EXPECT_EQ(version.Epoch(), 0U);
  EXPECT_EQ(version.Revision(), 0U);
}

TEST(AbiVersionTest, ExplicitConstructionAndRawFactoryPreserveParts) {
  const zcore::AbiVersion direct(2U, 7U);
  const zcore::AbiVersion factory = zcore::AbiVersion::FromRawUnchecked(4U, 3U);

  EXPECT_EQ(direct.Epoch(), 2U);
  EXPECT_EQ(direct.Revision(), 7U);
  EXPECT_EQ(factory.Epoch(), 4U);
  EXPECT_EQ(factory.Revision(), 3U);
}

TEST(AbiVersionTest, LexicographicComparisonUsesEpochThenRevision) {
  const zcore::AbiVersion v10(1U, 0U);
  const zcore::AbiVersion v11(1U, 1U);
  const zcore::AbiVersion v20(2U, 0U);

  EXPECT_LT(v10, v11);
  EXPECT_LT(v11, v20);
}

TEST(AbiVersionTest, CompatibilityRequiresSameEpochAndSufficientRevision) {
  const zcore::AbiVersion provider(3U, 5U);

  EXPECT_TRUE(provider.IsCompatibleWith(zcore::AbiVersion(3U, 0U)));
  EXPECT_TRUE(provider.IsCompatibleWith(zcore::AbiVersion(3U, 5U)));
  EXPECT_FALSE(provider.IsCompatibleWith(zcore::AbiVersion(3U, 6U)));
  EXPECT_FALSE(provider.IsCompatibleWith(zcore::AbiVersion(2U, 9U)));
  EXPECT_FALSE(provider.IsCompatibleWith(zcore::AbiVersion(4U, 0U)));
}

TEST(AbiVersionTest, BumpAndSetOperationsAreDeterministic) {
  zcore::AbiVersion version(1U, 2U);

  version.BumpRevision();
  EXPECT_EQ(version, zcore::AbiVersion(1U, 3U));

  version.SetRevision(9U);
  EXPECT_EQ(version, zcore::AbiVersion(1U, 9U));

  version.BumpEpoch();
  EXPECT_EQ(version, zcore::AbiVersion(2U, 0U));

  version.SetEpoch(7U);
  EXPECT_EQ(version, zcore::AbiVersion(7U, 0U));

  version.Reset();
  EXPECT_EQ(version, zcore::AbiVersion::Zero());
}

TEST(AbiVersionTest, HashSupportsUnorderedContainers) {
  std::unordered_set<zcore::AbiVersion> versions;
  versions.insert(zcore::AbiVersion(1U, 0U));
  versions.insert(zcore::AbiVersion(1U, 1U));
  versions.insert(zcore::AbiVersion(1U, 1U));

  EXPECT_EQ(versions.size(), 2U);
  EXPECT_EQ(versions.count(zcore::AbiVersion(1U, 1U)), 1U);
}

}  // namespace
