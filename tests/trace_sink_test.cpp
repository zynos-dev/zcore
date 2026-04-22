/**************************************************************************/
/*  trace_sink_test.cpp                                                   */
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
 * @file tests/trace_sink_test.cpp
 * @brief Unit tests for `TraceSink` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/error.hpp>
#include <zcore/trace_sink.hpp>

#include <gtest/gtest.h>

#include <type_traits>

namespace {

static_assert(!std::is_copy_constructible_v<zcore::TraceSink>);
static_assert(!std::is_copy_assignable_v<zcore::TraceSink>);
static_assert(!std::is_move_constructible_v<zcore::TraceSink>);
static_assert(!std::is_move_assignable_v<zcore::TraceSink>);

class WriteOnlyTraceSink final : public zcore::TraceSink {
 public:
  [[nodiscard]] zcore::Status Write(const zcore::TraceEvent& event) noexcept override {
    ++WriteCalls;
    LastEvent = event;
    return zcore::OkStatus();
  }

  int WriteCalls = 0;
  zcore::TraceEvent LastEvent{
      .type = zcore::TraceEventType::INSTANT,
      .category = "",
      .name = "",
      .timestampNs = 0U,
      .threadId = 0U,
      .value = 0,
  };
};

class FailableTraceSink final : public zcore::TraceSink {
 public:
  [[nodiscard]] zcore::Status Write(const zcore::TraceEvent& event) noexcept override {
    LastEvent = event;
    if (FailWrite) {
      return zcore::ErrorStatus(zcore::MakeError(
          zcore::kZcoreErrorDomain,
          1200,
          zcore::MakeErrorContext("diagnostic", "trace_write", "forced write failure", __FILE__, __LINE__)));
    }
    return zcore::OkStatus();
  }

  [[nodiscard]] zcore::Status Flush() noexcept override {
    ++FlushCalls;
    if (FailFlush) {
      return zcore::ErrorStatus(zcore::MakeError(
          zcore::kZcoreErrorDomain,
          1201,
          zcore::MakeErrorContext("diagnostic", "trace_flush", "forced flush failure", __FILE__, __LINE__)));
    }
    return zcore::OkStatus();
  }

  bool FailWrite = false;
  bool FailFlush = false;
  int FlushCalls = 0;
  zcore::TraceEvent LastEvent{
      .type = zcore::TraceEventType::INSTANT,
      .category = "",
      .name = "",
      .timestampNs = 0U,
      .threadId = 0U,
      .value = 0,
  };
};

TEST(TraceSinkTest, WriteCapturesEventAndDefaultFlushIsOk) {
  WriteOnlyTraceSink sink;
  const zcore::TraceEvent event = zcore::MakeTraceEvent(
      zcore::TraceEventType::DURATION_BEGIN,
      "frame",
      "update",
      900U,
      7U,
      0);

  const zcore::Status writeStatus = sink.Write(event);
  EXPECT_TRUE(writeStatus.HasValue());
  EXPECT_EQ(sink.WriteCalls, 1);
  EXPECT_EQ(sink.LastEvent.type, zcore::TraceEventType::DURATION_BEGIN);
  EXPECT_STREQ(sink.LastEvent.category, "frame");
  EXPECT_STREQ(sink.LastEvent.name, "update");
  EXPECT_EQ(sink.LastEvent.timestampNs, 900U);
  EXPECT_EQ(sink.LastEvent.threadId, 7U);
  EXPECT_EQ(sink.LastEvent.value, 0);

  const zcore::Status flushStatus = sink.Flush();
  EXPECT_TRUE(flushStatus.HasValue());
}

TEST(TraceSinkTest, WriteAndFlushCanReportFailures) {
  FailableTraceSink sink;
  const zcore::TraceEvent event = zcore::MakeTraceEvent(
      zcore::TraceEventType::COUNTER,
      "runtime",
      "jobs",
      1100U,
      3U,
      99);

  sink.FailWrite = true;
  const zcore::Status writeStatus = sink.Write(event);
  ASSERT_TRUE(writeStatus.HasError());
  EXPECT_EQ(writeStatus.Error().code.domain.id, zcore::kZcoreErrorDomain.id);
  EXPECT_EQ(writeStatus.Error().code.value, 1200);
  EXPECT_STREQ(writeStatus.Error().context.operation, "trace_write");

  sink.FailWrite = false;
  sink.FailFlush = true;
  const zcore::Status flushStatus = sink.Flush();
  ASSERT_TRUE(flushStatus.HasError());
  EXPECT_EQ(flushStatus.Error().code.domain.id, zcore::kZcoreErrorDomain.id);
  EXPECT_EQ(flushStatus.Error().code.value, 1201);
  EXPECT_STREQ(flushStatus.Error().context.operation, "trace_flush");
}

}  // namespace
