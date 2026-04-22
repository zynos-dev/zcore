/**************************************************************************/
/*  error/error.hpp                                                       */
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
 * @file include/zcore/error/error.hpp
 * @brief Structured error payload and metadata for fallible APIs.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/error.hpp>
 * const zcore::ErrorInfo error = zcore::MakeErrorInfo(
 *     zcore::ErrorDomain{.id = 1U, .name = "zcore"},
 *     7,
 *     zcore::MakeErrorContext("core", "init", "failed", __FILE__, __LINE__));
 * @endcode
 */

#pragma once

#include <cstdint>

namespace zcore {

namespace detail {

[[nodiscard]] constexpr bool CStrEquals(const char* lhs, const char* rhs) noexcept
{
    if (lhs == nullptr || rhs == nullptr) {
        return false;
    }

    uint32_t index = 0U;
    while (lhs[index] != '\0' && rhs[index] != '\0') {
        if (lhs[index] != rhs[index]) {
            return false;
        }
        ++index;
    }

    return lhs[index] == rhs[index];
}

} // namespace detail

/**
 * @brief Error domain metadata used to categorize error codes.
 */
struct ErrorDomain final {
    /// @brief Reserved identifier for the canonical success domain.
    static constexpr uint32_t kSuccessId = 0U;
    /// @brief Canonical success domain name.
    static constexpr const char* kSuccessName = "ok";

    /// @brief Stable domain identifier. `0` is reserved for the success domain.
    uint32_t id;
    /// @brief Human-readable domain name.
    const char* name;

    /// @brief Returns canonical success domain metadata.
    [[nodiscard]] static constexpr ErrorDomain Success() noexcept
    {
        return ErrorDomain{
                .id = kSuccessId,
                .name = kSuccessName,
        };
    }

    /// @brief Returns `true` when this is the canonical success domain.
    [[nodiscard]] constexpr bool IsSuccessDomain() const noexcept
    {
        return id == kSuccessId && detail::CStrEquals(name, kSuccessName);
    }

    /// @brief Returns `true` when this is a non-success domain with non-null name.
    [[nodiscard]] constexpr bool IsFailureDomain() const noexcept
    {
        return id != kSuccessId && name != nullptr;
    }

    /**
   * @brief Returns `true` when domain metadata is well-formed.
   *
   * Domain id `0` is valid only for canonical success domain `{0, "ok"}`.
   * Non-success domains require non-zero id and non-null name.
   */
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        if (id == kSuccessId) {
            return IsSuccessDomain();
        }
        return name != nullptr;
    }
};

/**
 * @brief Domain-qualified numeric error code.
 */
struct ErrorCode final {
    /// @brief Domain that owns this numeric code.
    ErrorDomain domain;
    /// @brief Domain-specific numeric value; `0` means success.
    int32_t value;

    /**
   * @brief Returns `true` when this code represents success.
   *
   * Success requires both zero numeric value and canonical success domain.
   */
    [[nodiscard]] constexpr bool IsOk() const noexcept
    {
        return value == 0 && domain.IsSuccessDomain();
    }
};

/**
 * @brief Lightweight source and operation context for diagnostics.
 */
struct ErrorContext final {
    /// @brief Logical subsystem name (for example, "io" or "net").
    const char* subsystem;
    /// @brief Operation name that failed.
    const char* operation;
    /// @brief Human-readable error message.
    const char* message;
    /// @brief Source file path where error was created.
    const char* file;
    /// @brief Source line where error was created.
    uint32_t line;
};

/**
 * @brief Structured error payload used by `Result` and `Status`.
 */
struct ErrorInfo final {
    /// @brief Domain and numeric code.
    ErrorCode code;
    /// @brief Diagnostic context attached at creation time.
    ErrorContext context;

    /// @brief Returns `true` when `code` is success (`value == 0`).
    [[nodiscard]] constexpr bool IsOk() const noexcept
    {
        return code.IsOk();
    }

    /**
   * @brief Constructs a canonical success error value.
   * @return Success-shaped `Error` payload.
   */
    [[nodiscard]] static constexpr ErrorInfo Ok() noexcept
    {
        return ErrorInfo{
                .code =
                        ErrorCode{
                                .domain = ErrorDomain::Success(),
                                .value = 0,
                        },
                .context =
                        ErrorContext{
                                .subsystem = "zcore",
                                .operation = "none",
                                .message = "ok",
                                .file = "",
                                .line = 0U,
                        },
        };
    }
};

/// @brief Backward-compatible alias for historical `Error` naming.
using Error = ErrorInfo;

/**
 * @brief Helper to construct an `ErrorContext`.
 * @param subsystem Logical subsystem name.
 * @param operation Operation identifier.
 * @param message Human-readable message.
 * @param file Source file path.
 * @param line Source line.
 * @return Populated context structure.
 */
[[nodiscard]] constexpr ErrorContext
MakeErrorContext(const char* subsystem, const char* operation, const char* message, const char* file, uint32_t line) noexcept
{
    return ErrorContext{
            .subsystem = subsystem,
            .operation = operation,
            .message = message,
            .file = file,
            .line = line,
    };
}

/**
 * @brief Helper to construct an `ErrorInfo` from code and context parts.
 * @param domain Error domain metadata.
 * @param value Domain-specific numeric value.
 * @param context Diagnostic context.
 * @return Structured error payload.
 */
[[nodiscard]] constexpr ErrorInfo MakeErrorInfo(ErrorDomain domain, int32_t value, ErrorContext context) noexcept
{
    return ErrorInfo{
            .code =
                    ErrorCode{
                            .domain = domain,
                            .value = value,
                    },
            .context = context,
    };
}

/**
 * @brief Backward-compatible helper to construct an `Error`.
 * @param domain Error domain metadata.
 * @param value Domain-specific numeric value.
 * @param context Diagnostic context.
 * @return Structured error payload.
 */
[[nodiscard]] constexpr Error MakeError(ErrorDomain domain, int32_t value, ErrorContext context) noexcept
{
    return MakeErrorInfo(domain, value, context);
}

/// @brief Built-in zcore error domain identifier.
inline constexpr ErrorDomain kZcoreErrorDomain{
        .id = 1U,
        .name = "zcore",
};

} // namespace zcore
