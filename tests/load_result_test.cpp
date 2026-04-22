/**************************************************************************/
/*  load_result_test.cpp                                                  */
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
 * @file tests/load_result_test.cpp
 * @brief Unit tests for plugin load result contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/load_result.hpp>

#include <gtest/gtest.h>

#include <type_traits>

namespace {

constexpr zcore::InterfaceId kPluginId = zcore::InterfaceId::FromLiteral("zcore.plugin.reader_impl.v1");
constexpr zcore::InterfaceId kReaderId = zcore::InterfaceId::FromLiteral("zcore.io.reader.v1");

static_assert(std::is_same_v<zcore::LoadResult, zcore::Result<zcore::PluginDescriptor, zcore::Error>>);

TEST(LoadResultTest, LoadedHelperConstructsSuccessResult) {
  const zcore::InterfaceId interfaces[] = {kReaderId};
  const zcore::PluginDescriptor descriptor(
      kPluginId,
      zcore::AbiVersion(1U, 0U),
      zcore::Slice<const zcore::InterfaceId>(interfaces));

  const zcore::LoadResult result = zcore::Loaded(descriptor);
  ASSERT_TRUE(result.HasValue());
  EXPECT_EQ(result.Value().PluginId(), kPluginId);
  EXPECT_EQ(result.Value().PluginAbiVersion(), zcore::AbiVersion(1U, 0U));
  EXPECT_TRUE(result.Value().Exposes(kReaderId));
}

TEST(LoadResultTest, LoadFailedHelperConstructsFailureResult) {
  const zcore::Error error = zcore::MakeLoadError(
      zcore::LoadErrorCode::NOT_FOUND,
      "load",
      "plugin not found");

  const zcore::LoadResult result = zcore::LoadFailed(error);
  ASSERT_TRUE(result.HasError());
  EXPECT_EQ(result.Error().code.domain.id, zcore::kLoadErrorDomain.id);
  EXPECT_EQ(result.Error().code.value, static_cast<zcore::i32>(zcore::LoadErrorCode::NOT_FOUND));
}

TEST(LoadResultTest, MakeLoadErrorUsesLoadDomainAndContext) {
  const zcore::Error error = zcore::MakeLoadError(
      zcore::LoadErrorCode::ABI_MISMATCH,
      "validate_abi",
      "required ABI is incompatible");

  EXPECT_EQ(error.code.domain.id, zcore::kLoadErrorDomain.id);
  EXPECT_STREQ(error.code.domain.name, "plugin_load");
  EXPECT_EQ(error.code.value, static_cast<zcore::i32>(zcore::LoadErrorCode::ABI_MISMATCH));
  EXPECT_STREQ(error.context.subsystem, "plugin_load");
  EXPECT_STREQ(error.context.operation, "validate_abi");
}

TEST(LoadResultTest, ResultCombinatorsWorkForLoadResult) {
  const zcore::InterfaceId interfaces[] = {kReaderId};
  const zcore::PluginDescriptor descriptor(
      kPluginId,
      zcore::AbiVersion(2U, 1U),
      zcore::Slice<const zcore::InterfaceId>(interfaces));

  const zcore::LoadResult loaded = zcore::Loaded(descriptor);
  const auto mapped = loaded.Map([](const zcore::PluginDescriptor& value) {
    return value.PluginAbiVersion();
  });
  ASSERT_TRUE(mapped.HasValue());
  EXPECT_EQ(mapped.Value(), zcore::AbiVersion(2U, 1U));

  const zcore::LoadResult failed = zcore::LoadFailed(
      zcore::MakeLoadError(zcore::LoadErrorCode::INTERFACE_UNSUPPORTED, "bind", "missing interface"));
  const auto recovered = failed.OrElse([](const zcore::Error&) {
    return zcore::Loaded(zcore::PluginDescriptor::Invalid());
  });
  ASSERT_TRUE(recovered.HasValue());
  EXPECT_TRUE(recovered.Value().IsInvalid());
}

}  // namespace
