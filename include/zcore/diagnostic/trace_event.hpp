/**************************************************************************/
/*  diagnostic/trace_event.hpp                                            */
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
 * @file include/zcore/diagnostic/trace_event.hpp
 * @brief Trace event payload and type contracts.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/trace_event.hpp>
 * const zcore::TraceEvent event = zcore::MakeTraceEvent(
 *     zcore::TraceEventType::INSTANT, "runtime", "tick", 1234U);
 * @endcode
 */

#pragma once

#include <zcore/foundation.hpp>

namespace zcore {

/**
 * @brief Trace event classification.
 */
enum class TraceEventType : u8 {
    /// @brief Instant event at one timestamp.
    INSTANT = 0U,
    /// @brief Duration-begin marker.
    DURATION_BEGIN = 1U,
    /// @brief Duration-end marker.
    DURATION_END = 2U,
    /// @brief Counter sample event.
    COUNTER = 3U,
};

/**
 * @brief Canonical trace event payload.
 */
struct TraceEvent final {
    /// @brief Event classification.
    TraceEventType type;
    /// @brief Logical trace category/subsystem.
    const char* category;
    /// @brief Event name.
    const char* name;
    /// @brief Timestamp in nanoseconds under caller-selected clock.
    u64 timestampNs;
    /// @brief Thread identifier for correlation (`0` when unknown/not set).
    u64 threadId;
    /// @brief Numeric payload (`COUNTER` value or caller-defined metadata).
    i64 value;
};

/**
 * @brief Builds a `TraceEvent` value.
 */
[[nodiscard]] constexpr TraceEvent MakeTraceEvent(
        TraceEventType type, const char* category, const char* name, u64 timestampNs, u64 threadId = 0U, i64 value = 0) noexcept
{
    return TraceEvent{
            .type = type,
            .category = category,
            .name = name,
            .timestampNs = timestampNs,
            .threadId = threadId,
            .value = value,
    };
}

/**
 * @brief Returns canonical short trace-event-type name.
 * @return Stable uppercase type name or `"UNKNOWN"` for invalid enum values.
 */
[[nodiscard]] constexpr const char* TraceEventTypeName(TraceEventType type) noexcept
{
    switch (type) {
    case TraceEventType::INSTANT:
        return "INSTANT";
    case TraceEventType::DURATION_BEGIN:
        return "DURATION_BEGIN";
    case TraceEventType::DURATION_END:
        return "DURATION_END";
    case TraceEventType::COUNTER:
        return "COUNTER";
    }
    return "UNKNOWN";
}

/**
 * @brief Returns `true` when enum value is in the defined `TraceEventType` set.
 */
[[nodiscard]] constexpr bool IsValidTraceEventType(TraceEventType type) noexcept
{
    switch (type) {
    case TraceEventType::INSTANT:
    case TraceEventType::DURATION_BEGIN:
    case TraceEventType::DURATION_END:
    case TraceEventType::COUNTER:
        return true;
    }
    return false;
}

} // namespace zcore
