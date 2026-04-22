/**************************************************************************/
/*  plugin_descriptor_test.cpp                                            */
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
 * @file tests/plugin_descriptor_test.cpp
 * @brief Unit tests for deterministic plugin descriptor contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/plugin_descriptor.hpp>

#include <gtest/gtest.h>

#include <type_traits>
#include <unordered_set>

namespace {

constexpr zcore::InterfaceId kPluginId = zcore::InterfaceId::FromLiteral("zcore.plugin.reader_impl.v1");
constexpr zcore::InterfaceId kReaderId = zcore::InterfaceId::FromLiteral("zcore.io.reader.v1");
constexpr zcore::InterfaceId kWriterId = zcore::InterfaceId::FromLiteral("zcore.io.writer.v1");
constexpr zcore::InterfaceId kSeekerId = zcore::InterfaceId::FromLiteral("zcore.io.seeker.v1");

static_assert(std::is_trivially_copyable_v<zcore::PluginDescriptor>);

TEST(PluginDescriptorTest, DefaultConstructedIsInvalid) {
  const zcore::PluginDescriptor descriptor;
  EXPECT_TRUE(descriptor.IsInvalid());
  EXPECT_EQ(descriptor.PluginId(), zcore::InterfaceId::Invalid());
  EXPECT_EQ(descriptor.PluginAbiVersion(), zcore::AbiVersion::Zero());
  EXPECT_EQ(descriptor.InterfaceCount(), 0U);
}

TEST(PluginDescriptorTest, ValidDescriptorExposesInterfacesAndSupportsRequests) {
  const zcore::InterfaceId interfaces[] = {kReaderId, kWriterId};
  const zcore::PluginDescriptor descriptor(
      kPluginId,
      zcore::AbiVersion(2U, 3U),
      zcore::Slice<const zcore::InterfaceId>(interfaces));

  EXPECT_TRUE(descriptor.IsValid());
  EXPECT_TRUE(descriptor.Exposes(kReaderId));
  EXPECT_TRUE(descriptor.Exposes(kWriterId));
  EXPECT_FALSE(descriptor.Exposes(kSeekerId));
  EXPECT_TRUE(descriptor.Supports(zcore::AbiVersion(2U, 1U), kReaderId));
  EXPECT_FALSE(descriptor.Supports(zcore::AbiVersion(3U, 0U), kReaderId));
  EXPECT_FALSE(descriptor.Supports(zcore::AbiVersion(2U, 1U), kSeekerId));
}

TEST(PluginDescriptorTest, ValidationRejectsInvalidOrDuplicateInterfaces) {
  const zcore::InterfaceId withInvalid[] = {kReaderId, zcore::InterfaceId::Invalid()};
  const zcore::PluginDescriptor invalidInterfaceDescriptor(
      kPluginId,
      zcore::AbiVersion(1U, 0U),
      zcore::Slice<const zcore::InterfaceId>(withInvalid));
  EXPECT_TRUE(invalidInterfaceDescriptor.IsInvalid());

  const zcore::InterfaceId withDuplicates[] = {kReaderId, kReaderId};
  const zcore::PluginDescriptor duplicateInterfaceDescriptor(
      kPluginId,
      zcore::AbiVersion(1U, 0U),
      zcore::Slice<const zcore::InterfaceId>(withDuplicates));
  EXPECT_TRUE(duplicateInterfaceDescriptor.IsInvalid());
}

TEST(PluginDescriptorTest, EqualityAndOrderingUseValueSemantics) {
  const zcore::InterfaceId listA[] = {kReaderId, kWriterId};
  const zcore::InterfaceId listB[] = {kReaderId, kWriterId};
  const zcore::InterfaceId listC[] = {kReaderId, kSeekerId};

  const zcore::PluginDescriptor a(
      kPluginId,
      zcore::AbiVersion(1U, 0U),
      zcore::Slice<const zcore::InterfaceId>(listA));
  const zcore::PluginDescriptor b(
      kPluginId,
      zcore::AbiVersion(1U, 0U),
      zcore::Slice<const zcore::InterfaceId>(listB));
  const zcore::PluginDescriptor c(
      kPluginId,
      zcore::AbiVersion(1U, 0U),
      zcore::Slice<const zcore::InterfaceId>(listC));
  const zcore::PluginDescriptor d(
      kPluginId,
      zcore::AbiVersion(1U, 1U),
      zcore::Slice<const zcore::InterfaceId>(listA));

  EXPECT_EQ(a, b);
  EXPECT_NE(a, c);
  EXPECT_LT(a, d);
  EXPECT_TRUE((a < c) || (c < a));
}

TEST(PluginDescriptorTest, ResetReturnsDescriptorToInvalidState) {
  const zcore::InterfaceId interfaces[] = {kReaderId};
  zcore::PluginDescriptor descriptor(
      kPluginId,
      zcore::AbiVersion(1U, 2U),
      zcore::Slice<const zcore::InterfaceId>(interfaces));
  ASSERT_TRUE(descriptor.IsValid());

  descriptor.Reset();
  EXPECT_EQ(descriptor, zcore::PluginDescriptor::Invalid());
  EXPECT_TRUE(descriptor.IsInvalid());
}

TEST(PluginDescriptorTest, HashSupportsUnorderedContainers) {
  const zcore::InterfaceId listA[] = {kReaderId};
  const zcore::InterfaceId listB[] = {kWriterId};
  const zcore::PluginDescriptor a(
      kPluginId,
      zcore::AbiVersion(1U, 0U),
      zcore::Slice<const zcore::InterfaceId>(listA));
  const zcore::PluginDescriptor b(
      kPluginId,
      zcore::AbiVersion(1U, 0U),
      zcore::Slice<const zcore::InterfaceId>(listB));
  const zcore::PluginDescriptor aDuplicate(
      kPluginId,
      zcore::AbiVersion(1U, 0U),
      zcore::Slice<const zcore::InterfaceId>(listA));

  std::unordered_set<zcore::PluginDescriptor> values;
  values.insert(a);
  values.insert(b);
  values.insert(aDuplicate);

  EXPECT_EQ(values.size(), 2U);
  EXPECT_EQ(values.count(a), 1U);
}

}  // namespace
