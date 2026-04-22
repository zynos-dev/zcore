/**************************************************************************/
/*  log_sink_test.cpp                                                     */
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
 * @file tests/log_sink_test.cpp
 * @brief Unit tests for `LogSink` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/error.hpp>
#include <zcore/log_sink.hpp>

#include <gtest/gtest.h>

#include <type_traits>

namespace {

static_assert(!std::is_copy_constructible_v<zcore::LogSink>);
static_assert(!std::is_copy_assignable_v<zcore::LogSink>);
static_assert(!std::is_move_constructible_v<zcore::LogSink>);
static_assert(!std::is_move_assignable_v<zcore::LogSink>);

class WriteOnlySink final : public zcore::LogSink {
 public:
  [[nodiscard]] zcore::Status Write(const zcore::LogRecord& record) noexcept override {
    ++WriteCalls;
    LastRecord = record;
    return zcore::OkStatus();
  }

  int WriteCalls = 0;
  zcore::LogRecord LastRecord{
      .level = zcore::LogLevel::INFO,
      .category = "",
      .message = "",
      .file = "",
      .line = 0U,
  };
};

class FailableSink final : public zcore::LogSink {
 public:
  [[nodiscard]] zcore::Status Write(const zcore::LogRecord& record) noexcept override {
    LastRecord = record;
    if (FailWrite) {
      return zcore::ErrorStatus(zcore::MakeError(
          zcore::kZcoreErrorDomain,
          999,
          zcore::MakeErrorContext("diagnostic", "log_write", "forced write failure", __FILE__, __LINE__)));
    }
    return zcore::OkStatus();
  }

  [[nodiscard]] zcore::Status Flush() noexcept override {
    ++FlushCalls;
    if (FailFlush) {
      return zcore::ErrorStatus(zcore::MakeError(
          zcore::kZcoreErrorDomain,
          1000,
          zcore::MakeErrorContext("diagnostic", "log_flush", "forced flush failure", __FILE__, __LINE__)));
    }
    return zcore::OkStatus();
  }

  bool FailWrite = false;
  bool FailFlush = false;
  int FlushCalls = 0;
  zcore::LogRecord LastRecord{
      .level = zcore::LogLevel::INFO,
      .category = "",
      .message = "",
      .file = "",
      .line = 0U,
  };
};

TEST(LogSinkTest, MakeLogRecordBuildsStructuredPayload) {
  const zcore::LogRecord record = zcore::MakeLogRecord(
      zcore::LogLevel::WARN,
      "net",
      "packet dropped",
      "file.cpp",
      44U);

  EXPECT_EQ(record.level, zcore::LogLevel::WARN);
  EXPECT_STREQ(record.category, "net");
  EXPECT_STREQ(record.message, "packet dropped");
  EXPECT_STREQ(record.file, "file.cpp");
  EXPECT_EQ(record.line, 44U);
}

TEST(LogSinkTest, WriteCapturesRecordAndDefaultFlushIsOk) {
  WriteOnlySink sink;
  const zcore::LogRecord record = zcore::MakeLogRecord(
      zcore::LogLevel::INFO,
      "core",
      "initialized",
      "main.cpp",
      12U);

  const zcore::Status writeStatus = sink.Write(record);
  EXPECT_TRUE(writeStatus.HasValue());
  EXPECT_EQ(sink.WriteCalls, 1);
  EXPECT_EQ(sink.LastRecord.level, zcore::LogLevel::INFO);
  EXPECT_STREQ(sink.LastRecord.category, "core");
  EXPECT_STREQ(sink.LastRecord.message, "initialized");
  EXPECT_STREQ(sink.LastRecord.file, "main.cpp");
  EXPECT_EQ(sink.LastRecord.line, 12U);

  const zcore::Status flushStatus = sink.Flush();
  EXPECT_TRUE(flushStatus.HasValue());
}

TEST(LogSinkTest, WriteAndFlushCanReportFailures) {
  FailableSink sink;
  const zcore::LogRecord record = zcore::MakeLogRecord(
      zcore::LogLevel::ERROR,
      "asset",
      "load failed");

  sink.FailWrite = true;
  const zcore::Status writeStatus = sink.Write(record);
  ASSERT_TRUE(writeStatus.HasError());
  EXPECT_EQ(writeStatus.Error().code.domain.id, zcore::kZcoreErrorDomain.id);
  EXPECT_EQ(writeStatus.Error().code.value, 999);
  EXPECT_STREQ(writeStatus.Error().context.operation, "log_write");

  sink.FailWrite = false;
  sink.FailFlush = true;
  const zcore::Status flushStatus = sink.Flush();
  ASSERT_TRUE(flushStatus.HasError());
  EXPECT_EQ(flushStatus.Error().code.domain.id, zcore::kZcoreErrorDomain.id);
  EXPECT_EQ(flushStatus.Error().code.value, 1000);
  EXPECT_STREQ(flushStatus.Error().context.operation, "log_flush");
}

}  // namespace
