/**************************************************************************/
/*  error_info_test.cpp                                                   */
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
 * @file tests/error_info_test.cpp
 * @brief Unit tests for ErrorInfo contracts and Error compatibility alias.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/error_info.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

namespace {

static_assert(std::is_same_v<zcore::ErrorInfo, zcore::Error>);

TEST(ErrorInfoTest, OkFactoryUsesCanonicalSuccessShape) {
  constexpr zcore::ErrorInfo kOk = zcore::ErrorInfo::Ok();
  static_assert(kOk.IsOk());
  EXPECT_TRUE(kOk.IsOk());
  EXPECT_EQ(kOk.code.value, 0);
  EXPECT_TRUE(kOk.code.domain.IsSuccessDomain());
}

TEST(ErrorInfoTest, MakeErrorInfoBuildsFailurePayload) {
  const zcore::ErrorInfo info = zcore::MakeErrorInfo(
      zcore::kZcoreErrorDomain,
      17,
      zcore::MakeErrorContext("io", "read", "short read", __FILE__, static_cast<std::uint32_t>(__LINE__)));

  EXPECT_FALSE(info.IsOk());
  EXPECT_EQ(info.code.domain.id, zcore::kZcoreErrorDomain.id);
  EXPECT_EQ(info.code.value, 17);
  EXPECT_STREQ(info.context.operation, "read");
}

TEST(ErrorInfoTest, ErrorAliasAndLegacyFactoryRemainCompatible) {
  const zcore::Error legacy = zcore::MakeError(
      zcore::ErrorDomain{.id = 7U, .name = "net"},
      3,
      zcore::MakeErrorContext("net", "connect", "timeout", __FILE__, static_cast<std::uint32_t>(__LINE__)));
  const zcore::ErrorInfo info = legacy;
  const zcore::Error rebound = info;

  EXPECT_EQ(info.code.domain.id, 7U);
  EXPECT_EQ(rebound.code.value, 3);
  EXPECT_STREQ(rebound.context.subsystem, "net");
}

}  // namespace
