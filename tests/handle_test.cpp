/**************************************************************************/
/*  handle_test.cpp                                                       */
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
 * @file tests/handle_test.cpp
 * @brief Unit tests for typed handle behavior.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/handle.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>
#include <unordered_set>

namespace {

struct EntityTag final {};
struct ResourceTag final {};

using EntityHandle = zcore::Handle<EntityTag>;
using ResourceHandle = zcore::Handle<ResourceTag>;

static_assert(sizeof(EntityHandle) == sizeof(std::uint32_t) * 2U);
static_assert(std::is_trivially_copyable_v<EntityHandle>);
static_assert(!std::is_convertible_v<EntityHandle, ResourceHandle>);
static_assert(!std::is_constructible_v<EntityHandle, ResourceHandle>);

TEST(HandleTest, DefaultConstructedIsInvalid) {
  const EntityHandle handle;
  EXPECT_TRUE(handle.IsInvalid());
  EXPECT_FALSE(handle.IsValid());
  EXPECT_FALSE(static_cast<bool>(handle));
  EXPECT_EQ(handle.Index(), EntityHandle::kInvalidIndex);
  EXPECT_EQ(handle.Generation(), EntityHandle::kInvalidGeneration);
}

TEST(HandleTest, ExplicitConstructionAndAccessorsWork) {
  const EntityHandle handle(42U, 7U);
  EXPECT_TRUE(handle.IsValid());
  EXPECT_EQ(handle.Index(), 42U);
  EXPECT_EQ(handle.Generation(), 7U);
  EXPECT_TRUE(static_cast<bool>(handle));
}

TEST(HandleTest, InvalidFactoryAndResetAreDeterministic) {
  EntityHandle handle(3U, 1U);
  ASSERT_TRUE(handle.IsValid());

  handle.Reset();
  EXPECT_EQ(handle, EntityHandle::Invalid());
  EXPECT_TRUE(handle.IsInvalid());
}

TEST(HandleTest, ComparisonAndSameSlotAreStable) {
  const EntityHandle a(7U, 1U);
  const EntityHandle b(7U, 2U);
  const EntityHandle c(9U, 1U);

  EXPECT_NE(a, b);
  EXPECT_LT(a, b);
  EXPECT_TRUE(a.SameSlot(b));
  EXPECT_FALSE(a.SameSlot(c));
}

TEST(HandleTest, HashSupportsUnorderedContainers) {
  std::unordered_set<EntityHandle> handles;
  handles.insert(EntityHandle(10U, 1U));
  handles.insert(EntityHandle(10U, 2U));
  handles.insert(EntityHandle(10U, 2U));

  EXPECT_EQ(handles.size(), 2U);
  EXPECT_EQ(handles.count(EntityHandle(10U, 2U)), 1U);
}

TEST(HandleTest, CustomSentinelsAreSupported) {
  using SlotHandle = zcore::Handle<struct SlotTag, std::uint16_t, std::uint8_t, 0xFFFFU, 0xFFU>;

  const SlotHandle invalid;
  EXPECT_TRUE(invalid.IsInvalid());
  EXPECT_EQ(invalid.Index(), 0xFFFFU);
  EXPECT_EQ(invalid.Generation(), 0xFFU);

  const SlotHandle valid(17U, 3U);
  EXPECT_TRUE(valid.IsValid());
  EXPECT_EQ(valid.Index(), 17U);
  EXPECT_EQ(valid.Generation(), 3U);
}

TEST(HandleTest, FromRawUncheckedPreservesRawParts) {
  const EntityHandle handle = EntityHandle::FromRawUnchecked(11U, 4U);
  EXPECT_EQ(handle.Index(), 11U);
  EXPECT_EQ(handle.Generation(), 4U);
}

}  // namespace

