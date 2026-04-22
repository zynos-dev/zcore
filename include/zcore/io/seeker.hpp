/**************************************************************************/
/*  seeker.hpp                                                            */
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
 * @file include/zcore/io/seeker.hpp
 * @brief Cursor seek interface contract.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/seeker.hpp>
 * @endcode
 */

#pragma once

#include <limits>
#include <zcore/io/io_error.hpp>
#include <zcore/result.hpp>

namespace zcore {

/**
 * @brief Seek origin for cursor movement.
 */
enum class SeekOrigin : u8 {
    /// @brief Offset is relative to stream start.
    BEGIN = 0U,
    /// @brief Offset is relative to current cursor.
    CURRENT = 1U,
    /// @brief Offset is relative to stream end.
    END = 2U,
};

/**
 * @brief Fallible seek interface for byte-oriented cursors.
 */
class Seeker {
public:
    Seeker() = default;
    virtual ~Seeker() = default;
    Seeker(const Seeker&) = delete;
    Seeker& operator=(const Seeker&) = delete;
    Seeker(Seeker&&) = delete;
    Seeker& operator=(Seeker&&) = delete;

    /**
   * @brief Moves cursor by `offset` from `origin`.
   * @param offset Signed byte offset from origin.
   * @param origin Seek origin.
   * @return New absolute byte position or I/O-domain error.
   */
    [[nodiscard]] virtual Result<usize, Error> Seek(isize offset, SeekOrigin origin) noexcept = 0;

    /**
   * @brief Moves cursor to absolute `position` from stream start.
   * @return New absolute byte position or I/O-domain error.
   */
    [[nodiscard]] Result<usize, Error> SeekTo(usize position) noexcept
    {
        if (position > static_cast<usize>(std::numeric_limits<isize>::max())) {
            return Result<usize, Error>::Failure(
                    MakeIoError(IoErrorCode::OUT_OF_RANGE, "seek_to", "position exceeds signed offset range"));
        }
        return Seek(static_cast<isize>(position), SeekOrigin::BEGIN);
    }

    /**
   * @brief Rewinds cursor to stream start.
   * @return New absolute byte position or I/O-domain error.
   */
    [[nodiscard]] Result<usize, Error> Rewind() noexcept
    {
        return Seek(0, SeekOrigin::BEGIN);
    }
};

} // namespace zcore
