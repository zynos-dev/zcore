/**************************************************************************/
/*  diagnostic/trace_sink.hpp                                             */
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
 * @file include/zcore/diagnostic/trace_sink.hpp
 * @brief Trace sink interface contracts.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/trace_sink.hpp>
 * @endcode
 */

#pragma once

#include <zcore/diagnostic/trace_event.hpp>
#include <zcore/status.hpp>

namespace zcore {

/**
 * @brief Fallible trace emission interface.
 */
class TraceSink {
public:
    TraceSink() = default;
    virtual ~TraceSink() = default;
    TraceSink(const TraceSink&) = delete;
    TraceSink& operator=(const TraceSink&) = delete;
    TraceSink(TraceSink&&) = delete;
    TraceSink& operator=(TraceSink&&) = delete;

    /**
   * @brief Emits one trace event.
   * @return Success or sink-specific error.
   */
    [[nodiscard]] virtual Status Write(const TraceEvent& event) noexcept = 0;

    /**
   * @brief Flushes buffered events when supported.
   * @return Success or sink-specific error.
   */
    [[nodiscard]] virtual Status Flush() noexcept
    {
        return OkStatus();
    }
};

} // namespace zcore
