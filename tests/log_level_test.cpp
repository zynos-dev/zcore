/**************************************************************************/
/*  log_level_test.cpp                                                    */
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
 * @file tests/log_level_test.cpp
 * @brief Unit tests for `LogLevel` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/log_level.hpp>

#include <gtest/gtest.h>

#include <type_traits>

namespace {

static_assert(std::is_same_v<std::underlying_type_t<zcore::LogLevel>, zcore::u8>);

TEST(LogLevelTest, NumericOrderingIsStable) {
  EXPECT_EQ(static_cast<zcore::u8>(zcore::LogLevel::TRACE), 0U);
  EXPECT_EQ(static_cast<zcore::u8>(zcore::LogLevel::DEBUG), 1U);
  EXPECT_EQ(static_cast<zcore::u8>(zcore::LogLevel::INFO), 2U);
  EXPECT_EQ(static_cast<zcore::u8>(zcore::LogLevel::WARN), 3U);
  EXPECT_EQ(static_cast<zcore::u8>(zcore::LogLevel::ERROR), 4U);
  EXPECT_EQ(static_cast<zcore::u8>(zcore::LogLevel::FATAL), 5U);
}

TEST(LogLevelTest, NameMappingIsStable) {
  EXPECT_STREQ(zcore::LogLevelName(zcore::LogLevel::TRACE), "TRACE");
  EXPECT_STREQ(zcore::LogLevelName(zcore::LogLevel::DEBUG), "DEBUG");
  EXPECT_STREQ(zcore::LogLevelName(zcore::LogLevel::INFO), "INFO");
  EXPECT_STREQ(zcore::LogLevelName(zcore::LogLevel::WARN), "WARN");
  EXPECT_STREQ(zcore::LogLevelName(zcore::LogLevel::ERROR), "ERROR");
  EXPECT_STREQ(zcore::LogLevelName(zcore::LogLevel::FATAL), "FATAL");
}

TEST(LogLevelTest, ValidationRejectsOutOfRangeEnumValues) {
  const auto invalid = static_cast<zcore::LogLevel>(255U);  // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
  EXPECT_FALSE(zcore::IsValidLogLevel(invalid));
  EXPECT_STREQ(zcore::LogLevelName(invalid), "UNKNOWN");
}

TEST(LogLevelTest, SeverityComparisonUsesOrdinalOrdering) {
  EXPECT_TRUE(zcore::LogLevelAtLeast(zcore::LogLevel::ERROR, zcore::LogLevel::WARN));
  EXPECT_TRUE(zcore::LogLevelAtLeast(zcore::LogLevel::INFO, zcore::LogLevel::INFO));
  EXPECT_FALSE(zcore::LogLevelAtLeast(zcore::LogLevel::DEBUG, zcore::LogLevel::INFO));
}

}  // namespace
