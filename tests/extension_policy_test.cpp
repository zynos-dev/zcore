/**************************************************************************/
/*  extension_policy_test.cpp                                             */
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
 * @file tests/extension_policy_test.cpp
 * @brief Unit tests for extension-policy contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/extension_policy.hpp>

#include <gtest/gtest.h>

#include <type_traits>

namespace {

constexpr zcore::ErrorDomain kBaseDomain{
    .id = 77U,
    .name = "asset",
};

constexpr zcore::ErrorDomain kForeignDomain{
    .id = 91U,
    .name = "plugin",
};

static_assert(std::is_trivially_copyable_v<zcore::ExtensionPolicy>);

TEST(ExtensionPolicyTest, DefaultConstructedPolicyIsInvalid) {
  const zcore::ExtensionPolicy policy;
  EXPECT_TRUE(policy.IsInvalid());
  EXPECT_FALSE(policy.Allows(zcore::ErrorCode{
      .domain = kBaseDomain,
      .value = 1,
  }));
}

TEST(ExtensionPolicyTest, StrictPolicyAcceptsOnlyKnownBaseDomainCodes) {
  const zcore::ExtensionPolicy policy = zcore::ExtensionPolicy::Strict(kBaseDomain, 3);

  EXPECT_TRUE(policy.IsValid());
  EXPECT_TRUE(policy.IsKnownCode(zcore::ErrorCode{
      .domain = kBaseDomain,
      .value = 3,
  }));
  EXPECT_TRUE(policy.Allows(zcore::ErrorCode{
      .domain = kBaseDomain,
      .value = 1,
  }));
  EXPECT_FALSE(policy.Allows(zcore::ErrorCode{
      .domain = kBaseDomain,
      .value = 4,
  }));
  EXPECT_FALSE(policy.Allows(zcore::ErrorCode{
      .domain = kForeignDomain,
      .value = 1,
  }));
}

TEST(ExtensionPolicyTest, SameDomainForwardCompatibleAcceptsBaseDomainExtensions) {
  const zcore::ExtensionPolicy policy =
      zcore::ExtensionPolicy::SameDomainForwardCompatible(kBaseDomain, 2);

  const zcore::ErrorCode extensionCode{
      .domain = kBaseDomain,
      .value = 9,
  };

  EXPECT_TRUE(policy.IsExtensionCode(extensionCode));
  EXPECT_TRUE(policy.Allows(extensionCode));
  EXPECT_FALSE(policy.Allows(zcore::ErrorCode{
      .domain = kForeignDomain,
      .value = 9,
  }));
}

TEST(ExtensionPolicyTest, AnyDomainForwardCompatibleAcceptsForeignFailureDomains) {
  const zcore::ExtensionPolicy policy =
      zcore::ExtensionPolicy::AnyDomainForwardCompatible(kBaseDomain, 2);

  EXPECT_TRUE(policy.Allows(zcore::ErrorCode{
      .domain = kForeignDomain,
      .value = 4,
  }));
  EXPECT_FALSE(policy.Allows(zcore::ErrorCode{
      .domain = zcore::ErrorDomain::Success(),
      .value = 4,
  }));
}

TEST(ExtensionPolicyTest, AllowOkFlagControlsSuccessCodeAcceptance) {
  const zcore::ErrorCode okCode{
      .domain = zcore::ErrorDomain::Success(),
      .value = 0,
  };

  const zcore::ExtensionPolicy disallowOk = zcore::ExtensionPolicy::Strict(kBaseDomain, 2);
  const zcore::ExtensionPolicy allowOk = zcore::ExtensionPolicy::Strict(kBaseDomain, 2, true);

  EXPECT_FALSE(disallowOk.Allows(okCode));
  EXPECT_TRUE(allowOk.Allows(okCode));
}

TEST(ExtensionPolicyTest, AllowsErrorInfoDelegatesToErrorCodePolicy) {
  const zcore::ExtensionPolicy policy =
      zcore::ExtensionPolicy::SameDomainForwardCompatible(kBaseDomain, 2);
  const zcore::ErrorInfo info = zcore::MakeErrorInfo(
      kBaseDomain,
      6,
      zcore::MakeErrorContext("asset", "parse", "extension", __FILE__, 1U));

  EXPECT_TRUE(policy.Allows(info));
}

TEST(ExtensionPolicyTest, MakeExtensionPolicyErrorUsesExtensionPolicyDomainAndContext) {
  const zcore::Error error = zcore::MakeExtensionPolicyError(
      zcore::ExtensionPolicyErrorCode::DOMAIN_REJECTED,
      "check",
      "domain is not allowed");

  EXPECT_EQ(error.code.domain.id, zcore::kExtensionPolicyErrorDomain.id);
  EXPECT_STREQ(error.code.domain.name, "extension_policy");
  EXPECT_EQ(error.code.value, static_cast<zcore::i32>(zcore::ExtensionPolicyErrorCode::DOMAIN_REJECTED));
  EXPECT_STREQ(error.context.subsystem, "extension_policy");
  EXPECT_STREQ(error.context.operation, "check");
}

TEST(ExtensionPolicyTest, ResetReturnsPolicyToInvalidState) {
  zcore::ExtensionPolicy policy = zcore::ExtensionPolicy::AnyDomainForwardCompatible(kBaseDomain, 5, true);
  ASSERT_TRUE(policy.IsValid());

  policy.Reset();
  EXPECT_EQ(policy, zcore::ExtensionPolicy::Invalid());
  EXPECT_TRUE(policy.IsInvalid());
}

}  // namespace

