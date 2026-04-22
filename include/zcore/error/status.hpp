/**************************************************************************/
/*  status.hpp                                                            */
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
 * @file include/zcore/error/status.hpp
 * @brief Lightweight status and status/result helpers.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/status.hpp>
 * const zcore::Status status = zcore::OkStatus();
 * @endcode
 */

#pragma once

#include <zcore/error.hpp>
#include <zcore/result.hpp>

namespace zcore {

/**
 * @brief Convenience alias for `Result<void, Error>`.
 *
 * Use when an operation either succeeds with no payload or fails with `Error`.
 */
using Status = Result<void, Error>;

/**
 * @brief Constructs a successful status.
 * @return `Status::Success()`.
 */
[[nodiscard]] constexpr Status OkStatus() noexcept
{
    return Status::Success();
}

/**
 * @brief Constructs a failure status from an lvalue error.
 * @param error Error payload to copy.
 * @return `Status::Failure(error)`.
 */
[[nodiscard]] constexpr Status ErrorStatus(const Error& error)
{
    return Status::Failure(error);
}

/**
 * @brief Constructs a failure status from an rvalue error.
 * @param error Error payload to move.
 * @return `Status::Failure(std::move(error))`.
 */
[[nodiscard]] constexpr Status ErrorStatus(Error&& error)
{
    return Status::Failure(static_cast<Error&&>(error));
}

} // namespace zcore
