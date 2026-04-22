/**************************************************************************/
/*  reader.hpp                                                            */
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
 * @file include/zcore/io/reader.hpp
 * @brief Byte-oriented read interface contract.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/reader.hpp>
 * @endcode
 */

#pragma once

#include <zcore/io/io_error.hpp>
#include <zcore/result.hpp>
#include <zcore/slice.hpp>

namespace zcore {

/**
 * @brief Fallible byte-oriented read interface.
 *
 * Implementations read up to `destination.Size()` bytes and return the number
 * of bytes read.
 */
class Reader {
public:
    Reader() = default;
    virtual ~Reader() = default;
    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;
    Reader(Reader&&) = delete;
    Reader& operator=(Reader&&) = delete;

    /**
   * @brief Reads bytes into destination.
   * @param destination Writable destination buffer.
   * @return Number of bytes read or I/O-domain error.
   */
    [[nodiscard]] virtual Result<usize, Error> Read(ByteSpanMut destination) noexcept = 0;
};

} // namespace zcore
