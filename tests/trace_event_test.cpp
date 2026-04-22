/**************************************************************************/
/*  trace_event_test.cpp                                                  */
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
 * @file tests/trace_event_test.cpp
 * @brief Unit tests for `TraceEvent` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/trace_event.hpp>

#include <gtest/gtest.h>

#include <type_traits>

namespace {

static_assert(std::is_same_v<std::underlying_type_t<zcore::TraceEventType>, zcore::u8>);
static_assert(std::is_trivially_copyable_v<zcore::TraceEvent>);

TEST(TraceEventTest, NumericOrderingIsStable) {
  EXPECT_EQ(static_cast<zcore::u8>(zcore::TraceEventType::INSTANT), 0U);
  EXPECT_EQ(static_cast<zcore::u8>(zcore::TraceEventType::DURATION_BEGIN), 1U);
  EXPECT_EQ(static_cast<zcore::u8>(zcore::TraceEventType::DURATION_END), 2U);
  EXPECT_EQ(static_cast<zcore::u8>(zcore::TraceEventType::COUNTER), 3U);
}

TEST(TraceEventTest, NameMappingIsStable) {
  EXPECT_STREQ(zcore::TraceEventTypeName(zcore::TraceEventType::INSTANT), "INSTANT");
  EXPECT_STREQ(zcore::TraceEventTypeName(zcore::TraceEventType::DURATION_BEGIN), "DURATION_BEGIN");
  EXPECT_STREQ(zcore::TraceEventTypeName(zcore::TraceEventType::DURATION_END), "DURATION_END");
  EXPECT_STREQ(zcore::TraceEventTypeName(zcore::TraceEventType::COUNTER), "COUNTER");
}

TEST(TraceEventTest, ValidationRejectsOutOfRangeEnumValues) {
  const auto invalid = static_cast<zcore::TraceEventType>(255U);  // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
  EXPECT_FALSE(zcore::IsValidTraceEventType(invalid));
  EXPECT_STREQ(zcore::TraceEventTypeName(invalid), "UNKNOWN");
}

TEST(TraceEventTest, MakeTraceEventBuildsStructuredPayload) {
  const zcore::TraceEvent event = zcore::MakeTraceEvent(
      zcore::TraceEventType::COUNTER,
      "runtime",
      "allocations",
      1001U,
      9U,
      42);

  EXPECT_EQ(event.type, zcore::TraceEventType::COUNTER);
  EXPECT_STREQ(event.category, "runtime");
  EXPECT_STREQ(event.name, "allocations");
  EXPECT_EQ(event.timestampNs, 1001U);
  EXPECT_EQ(event.threadId, 9U);
  EXPECT_EQ(event.value, 42);
}

}  // namespace
