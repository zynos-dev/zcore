/**************************************************************************/
/*  io_error.hpp                                                          */
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
 * @file include/zcore/io/io_error.hpp
 * @brief I/O-domain error contracts for reader/writer/seeker interfaces.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/io_error.hpp>
 * @endcode
 */

#pragma once

#include <zcore/error.hpp>
#include <zcore/foundation.hpp>

namespace zcore {

/**
 * @brief I/O-domain error codes.
 */
enum class IoErrorCode : i32 {
    /// @brief Input argument is invalid.
    INVALID_ARGUMENT = 1,
    /// @brief Requested operation is not supported.
    UNSUPPORTED_OPERATION = 2,
    /// @brief Requested operation exceeds valid range.
    OUT_OF_RANGE = 3,
    /// @brief End-of-stream reached before full operation.
    END_OF_STREAM = 4,
};


inline const char* ToString(IoErrorCode e)
{
    switch (e) {
    case IoErrorCode::INVALID_ARGUMENT:
        return "INVALID_ARGUMENT";
    case IoErrorCode::UNSUPPORTED_OPERATION:
        return "UNSUPPORTED_OPERATION";
    case IoErrorCode::OUT_OF_RANGE:
        return "OUT_OF_RANGE";
    case IoErrorCode::END_OF_STREAM:
        return "END_OF_STREAM";
    default:
        return "unknown";
    }
}

/// @brief Built-in I/O error domain identifier.
inline constexpr ErrorDomain kIoErrorDomain{
        .id = 4U,
        .name = "io",
};

/**
 * @brief Constructs I/O-domain error payload.
 * @param code I/O error code.
 * @param operation Operation identifier.
 * @param message Human-readable message.
 * @param file Source file path.
 * @param line Source line.
 * @return Structured I/O error.
 */
[[nodiscard]] constexpr Error
MakeIoError(IoErrorCode code, const char* operation, const char* message, const char* file = "", u32 line = 0U) noexcept
{
    return MakeError(kIoErrorDomain, static_cast<i32>(code), MakeErrorContext("io", operation, message, file, line));
}

} // namespace zcore
