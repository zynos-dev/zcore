/**************************************************************************/
/*  writer.hpp                                                            */
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
 * @file include/zcore/io/writer.hpp
 * @brief Byte-oriented write interface contract.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/writer.hpp>
 * @endcode
 */

#pragma once

#include <zcore/io/io_error.hpp>
#include <zcore/result.hpp>
#include <zcore/slice.hpp>
#include <zcore/status.hpp>

namespace zcore {

/**
 * @brief Fallible byte-oriented write interface.
 *
 * Implementations write up to `source.Size()` bytes and return the number
 * of bytes written.
 */
class Writer {
public:
    Writer() = default;
    virtual ~Writer() = default;
    Writer(const Writer&) = delete;
    Writer& operator=(const Writer&) = delete;
    Writer(Writer&&) = delete;
    Writer& operator=(Writer&&) = delete;

    /**
   * @brief Writes bytes from source.
   * @param source Byte source buffer.
   * @return Number of bytes written or I/O-domain error.
   */
    [[nodiscard]] virtual Result<usize, Error> Write(ByteSpan source) noexcept = 0;

    /**
   * @brief Flushes buffered data when supported.
   * @return Success or I/O-domain error.
   */
    [[nodiscard]] virtual Status Flush() noexcept
    {
        return OkStatus();
    }
};

} // namespace zcore
