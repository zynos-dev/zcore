/**************************************************************************/
/*  error/error_handler.hpp                                               */
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
 * @file include/zcore/error/error_handler.hpp
 * @brief Explicit callback helpers for reporting recoverable `Error` values.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/error_handler.hpp>
 * @endcode
 */

#pragma once

#include <cstdio>
#include <zcore/error/error.hpp>
#include <zcore/error/result.hpp>
#include <zcore/error/status.hpp>

namespace zcore {

/**
 * @brief Callback signature for recoverable error reporting.
 */
using ErrorHandler = void (*)(const Error& error, void* userData) noexcept;

/**
 * @brief Non-owning handler reference passed through reporting helpers.
 */
struct ErrorHandlerRef final {
    /// @brief Callback target, null when disabled.
    ErrorHandler handler;
    /// @brief Opaque caller-owned context passed to handler.
    void* userData;

    /// @brief Returns `true` when handler callback is set.
    [[nodiscard]] constexpr bool IsSet() const noexcept
    {
        return handler != nullptr;
    }

    /// @brief Returns disabled handler reference.
    [[nodiscard]] static constexpr ErrorHandlerRef None() noexcept
    {
        return ErrorHandlerRef{
                .handler = nullptr,
                .userData = nullptr,
        };
    }
};

/**
 * @brief Builds an `ErrorHandlerRef` from callback and context.
 */
[[nodiscard]] constexpr ErrorHandlerRef MakeErrorHandler(ErrorHandler handler, void* userData = nullptr) noexcept
{
    return ErrorHandlerRef{
            .handler = handler,
            .userData = userData,
    };
}

/**
 * @brief Reports error to handler when configured.
 * @return `true` when callback was invoked.
 */
[[nodiscard]] inline bool ReportError(const Error& error, ErrorHandlerRef errorHandler) noexcept
{
    if (!errorHandler.IsSet()) {
        return false;
    }
    errorHandler.handler(error, errorHandler.userData);
    return true;
}

/**
 * @brief Reports failure branch from `Result<ValueT, Error>`.
 * @return `true` when result had error and callback was invoked.
 */
template <typename ValueT>
[[nodiscard]] inline bool HandleIfError(const Result<ValueT, Error>& result, ErrorHandlerRef errorHandler) noexcept
{
    if (!result.HasError()) {
        return false;
    }
    return ReportError(result.Error(), errorHandler);
}

/**
 * @brief Reports failure branch from `Status`.
 * @return `true` when status had error and callback was invoked.
 */
[[nodiscard]] inline bool HandleIfError(const Status& status, ErrorHandlerRef errorHandler) noexcept
{
    if (!status.HasError()) {
        return false;
    }
    return ReportError(status.Error(), errorHandler);
}

/**
 * @brief Convenience `ReportError` overload from callback and context.
 */
[[nodiscard]] inline bool ReportError(const Error& error, ErrorHandler handler, void* userData = nullptr) noexcept
{
    return ReportError(error, MakeErrorHandler(handler, userData));
}

/**
 * @brief Convenience `HandleIfError` overload from callback and context.
 */
template <typename ValueT>
[[nodiscard]] inline bool
HandleIfError(const Result<ValueT, Error>& result, ErrorHandler handler, void* userData = nullptr) noexcept
{
    return HandleIfError(result, MakeErrorHandler(handler, userData));
}

/**
 * @brief Convenience `HandleIfError` overload for `Status`.
 */
[[nodiscard]] inline bool HandleIfError(const Status& status, ErrorHandler handler, void* userData = nullptr) noexcept
{
    return HandleIfError(status, MakeErrorHandler(handler, userData));
}

/**
 * @brief Explicit opt-in sink that writes errors to `stderr`.
 */
inline void StderrErrorSink(const Error& error, void* userData) noexcept
{
    (void) userData;
    const char* const domainName = error.code.domain.name == nullptr ? "<unknown-domain>" : error.code.domain.name;
    const char* const subsystem = error.context.subsystem == nullptr ? "<unknown-subsystem>" : error.context.subsystem;
    const char* const operation = error.context.operation == nullptr ? "<unknown-operation>" : error.context.operation;
    const char* const message = error.context.message == nullptr ? "<no-message>" : error.context.message;
    const char* const file = error.context.file == nullptr ? "<unknown-file>" : error.context.file;

    std::fprintf(stderr,
                 "zcore error [%s:%d] %s::%s: %s (%s:%u)\n",
                 domainName,
                 error.code.value,
                 subsystem,
                 operation,
                 message,
                 file,
                 static_cast<unsigned int>(error.context.line));
}

} // namespace zcore
