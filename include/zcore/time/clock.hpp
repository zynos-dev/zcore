/**************************************************************************/
/*  clock.hpp                                                             */
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
 * @file include/zcore/time/clock.hpp
 * @brief Instant source interface for deterministic time queries.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/clock.hpp>
 * @endcode
 */

#pragma once

#include <zcore/time/instant.hpp>

namespace zcore {

/**
 * @brief Abstract instant source.
 *
 * Implementations provide current time points for monotonic or wall-clock
 * semantics.
 */
class Clock {
public:
    Clock() = default;
    virtual ~Clock() = default;
    Clock(const Clock&) = delete;
    Clock& operator=(const Clock&) = delete;
    Clock(Clock&&) = delete;
    Clock& operator=(Clock&&) = delete;

    /**
   * @brief Returns the current instant.
   */
    [[nodiscard]] virtual Instant Now() const noexcept = 0;

    /**
   * @brief Returns `true` when `Now()` is guaranteed monotonic.
   */
    [[nodiscard]] virtual bool IsMonotonic() const noexcept
    {
        return false;
    }

    /**
   * @brief Returns elapsed duration from `start` to `Now()`.
   */
    [[nodiscard]] Duration ElapsedSince(Instant start) const noexcept
    {
        return Now() - start;
    }
};

} // namespace zcore
