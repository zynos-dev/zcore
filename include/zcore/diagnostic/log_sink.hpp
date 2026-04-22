/**************************************************************************/
/*  diagnostic/log_sink.hpp                                               */
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
 * @file include/zcore/diagnostic/log_sink.hpp
 * @brief Log sink interface contracts.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/log_sink.hpp>
 * @endcode
 */

#pragma once

#include <zcore/diagnostic/log_level.hpp>
#include <zcore/status.hpp>

namespace zcore {

/**
 * @brief Structured log record payload.
 */
struct LogRecord final {
    /// @brief Severity level.
    LogLevel level;
    /// @brief Logical logging category/subsystem.
    const char* category;
    /// @brief Human-readable message text.
    const char* message;
    /// @brief Source file path where log record originated.
    const char* file;
    /// @brief Source line where log record originated.
    u32 line;
};

/**
 * @brief Constructs a `LogRecord` value.
 */
[[nodiscard]] constexpr LogRecord
MakeLogRecord(LogLevel level, const char* category, const char* message, const char* file = "", u32 line = 0U) noexcept
{
    return LogRecord{
            .level = level,
            .category = category,
            .message = message,
            .file = file,
            .line = line,
    };
}

/**
 * @brief Fallible log emission interface.
 */
class LogSink {
public:
    LogSink() = default;
    virtual ~LogSink() = default;
    LogSink(const LogSink&) = delete;
    LogSink& operator=(const LogSink&) = delete;
    LogSink(LogSink&&) = delete;
    LogSink& operator=(LogSink&&) = delete;

    /**
   * @brief Emits one log record.
   * @return Success or sink-specific error.
   */
    [[nodiscard]] virtual Status Write(const LogRecord& record) noexcept = 0;

    /**
   * @brief Flushes buffered records when supported.
   * @return Success or sink-specific error.
   */
    [[nodiscard]] virtual Status Flush() noexcept
    {
        return OkStatus();
    }
};

} // namespace zcore
