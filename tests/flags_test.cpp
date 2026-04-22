/**************************************************************************/
/*  flags_test.cpp                                                        */
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
 * @file tests/flags_test.cpp
 * @brief Unit tests for enum-backed `zcore::Flags` behavior.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/flags.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <type_traits>
#include <unordered_set>

namespace {

enum class Permission : std::uint8_t {
  NONE = 0U,
  READ = 1U << 0U,
  WRITE = 1U << 1U,
  EXECUTE = 1U << 2U,
  ADMIN = 1U << 3U,
};

using Permissions = zcore::Flags<Permission>;

static_assert(std::is_trivially_copyable_v<Permissions>);
static_assert(sizeof(Permissions) == sizeof(std::uint8_t));

TEST(FlagsTest, DefaultConstructedIsEmpty) {
  const Permissions flags;
  EXPECT_TRUE(flags.IsEmpty());
  EXPECT_FALSE(flags.Any());
  EXPECT_FALSE(static_cast<bool>(flags));
  EXPECT_EQ(flags.Raw(), 0U);
}

TEST(FlagsTest, SingleFlagConstructionAndChecks) {
  const Permissions flags(Permission::READ);
  EXPECT_TRUE(flags.Has(Permission::READ));
  EXPECT_FALSE(flags.Has(Permission::WRITE));
  EXPECT_TRUE(flags.Any());
}

TEST(FlagsTest, SetClearToggleAndAssignWork) {
  Permissions flags = Permissions::None();
  flags.Set(Permission::READ);
  flags.Set(Permission::WRITE);

  EXPECT_TRUE(flags.Has(Permission::READ));
  EXPECT_TRUE(flags.Has(Permission::WRITE));

  flags.Clear(Permission::READ);
  EXPECT_FALSE(flags.Has(Permission::READ));
  EXPECT_TRUE(flags.Has(Permission::WRITE));

  flags.Toggle(Permission::EXECUTE);
  EXPECT_TRUE(flags.Has(Permission::EXECUTE));
  flags.Toggle(Permission::EXECUTE);
  EXPECT_FALSE(flags.Has(Permission::EXECUTE));

  flags.Assign(Permission::ADMIN, true);
  EXPECT_TRUE(flags.Has(Permission::ADMIN));
  flags.Assign(Permission::WRITE, false);
  EXPECT_FALSE(flags.Has(Permission::WRITE));
}

TEST(FlagsTest, HasAnyAndHasAllUseMaskSemantics) {
  const Permissions flags = Permissions(Permission::READ) | Permission::WRITE | Permission::EXECUTE;
  const Permissions readWrite = Permissions(Permission::READ) | Permission::WRITE;
  const Permissions writeAdmin = Permissions(Permission::WRITE) | Permission::ADMIN;

  EXPECT_TRUE(flags.HasAll(readWrite));
  EXPECT_TRUE(flags.HasAny(writeAdmin));
  EXPECT_FALSE(flags.HasAll(writeAdmin));
}

TEST(FlagsTest, BitwiseOperatorsComposeFlagsDeterministically) {
  Permissions flags = Permissions(Permission::READ) | Permission::WRITE;
  flags |= Permission::EXECUTE;
  EXPECT_TRUE(flags.Has(Permission::EXECUTE));

  const Permissions masked = flags & (Permissions(Permission::WRITE) | Permission::ADMIN);
  EXPECT_TRUE(masked.Has(Permission::WRITE));
  EXPECT_FALSE(masked.Has(Permission::READ));
  EXPECT_FALSE(masked.Has(Permission::EXECUTE));

  flags ^= Permission::WRITE;
  EXPECT_FALSE(flags.Has(Permission::WRITE));
}

TEST(FlagsTest, RawFactoryAndClearAllWork) {
  Permissions flags = Permissions::FromRawUnchecked(0b1010U);
  EXPECT_TRUE(flags.Has(Permission::WRITE));
  EXPECT_TRUE(flags.Has(Permission::ADMIN));

  flags.ClearAll();
  EXPECT_TRUE(flags.IsEmpty());
  EXPECT_EQ(flags.Raw(), 0U);
}

TEST(FlagsTest, HashAdaptersWorkForStandardContainers) {
  std::unordered_set<Permissions> set;
  set.insert(Permissions(Permission::READ) | Permission::WRITE);
  set.insert(Permissions(Permission::READ) | Permission::WRITE);
  set.insert(Permission::ADMIN);

  EXPECT_EQ(set.size(), 2U);
  EXPECT_EQ(set.count(Permission::ADMIN), 1U);
}

TEST(FlagsTest, ZcoreHashSpecializationMatchesStdHashAdapterDigest) {
  const Permissions flags = Permissions(Permission::READ) | Permission::EXECUTE;
  const auto digest = zcore::hash::Hash<Permissions>{}(flags, 99ULL);
  const auto expectedStd = zcore::hash::DigestToSizeT(zcore::hash::Hash<Permissions>{}(flags));
  const auto adaptedStd = std::hash<Permissions>{}(flags);

  EXPECT_NE(digest, 0ULL);
  EXPECT_EQ(adaptedStd, expectedStd);
}

}  // namespace
