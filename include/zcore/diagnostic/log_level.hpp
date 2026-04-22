/**************************************************************************/
/*  diagnostic/log_level.hpp                                              */
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
 * @file include/zcore/diagnostic/log_level.hpp
 * @brief Log severity level contracts.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/log_level.hpp>
 * const zcore::LogLevel level = zcore::LogLevel::INFO;
 * @endcode
 */

#pragma once

#include <zcore/foundation.hpp>

namespace zcore {

/**
 * @brief Ordered log severity levels.
 */
enum class LogLevel : u8 {
    /// @brief Verbose diagnostic detail.
    TRACE = 0U,
    /// @brief Debug detail for development.
    DEBUG = 1U,
    /// @brief Informational operational event.
    INFO = 2U,
    /// @brief Recoverable warning condition.
    WARN = 3U,
    /// @brief Error condition requiring attention.
    ERROR = 4U,
    /// @brief Fatal condition requiring immediate termination/escalation.
    FATAL = 5U,
};

/**
 * @brief Returns canonical short level name.
 * @param level Level value.
 * @return Stable uppercase level name or `"UNKNOWN"` for invalid enum values.
 */
[[nodiscard]] constexpr const char* LogLevelName(LogLevel level) noexcept
{
    switch (level) {
    case LogLevel::TRACE:
        return "TRACE";
    case LogLevel::DEBUG:
        return "DEBUG";
    case LogLevel::INFO:
        return "INFO";
    case LogLevel::WARN:
        return "WARN";
    case LogLevel::ERROR:
        return "ERROR";
    case LogLevel::FATAL:
        return "FATAL";
    }
    return "UNKNOWN";
}

/**
 * @brief Returns `true` when enum value is in the defined `LogLevel` set.
 */
[[nodiscard]] constexpr bool IsValidLogLevel(LogLevel level) noexcept
{
    switch (level) {
    case LogLevel::TRACE:
    case LogLevel::DEBUG:
    case LogLevel::INFO:
    case LogLevel::WARN:
    case LogLevel::ERROR:
    case LogLevel::FATAL:
        return true;
    }
    return false;
}

/**
 * @brief Returns `true` when `candidate` severity is at least `minimum`.
 */
[[nodiscard]] constexpr bool LogLevelAtLeast(LogLevel candidate, LogLevel minimum) noexcept
{
    return static_cast<u8>(candidate) >= static_cast<u8>(minimum);
}

} // namespace zcore
