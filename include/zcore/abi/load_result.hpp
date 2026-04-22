/**************************************************************************/
/*  load_result.hpp                                                       */
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
 * @file include/zcore/abi/load_result.hpp
 * @brief Plugin load result and plugin-load domain error helpers.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/load_result.hpp>
 * zcore::LoadResult result = zcore::LoadFailed(
 *     zcore::MakeLoadError(zcore::LoadErrorCode::NOT_FOUND, "load", "plugin not found"));
 * @endcode
 */

#pragma once

#include <zcore/abi/plugin_descriptor.hpp>
#include <zcore/error.hpp>
#include <zcore/foundation.hpp>
#include <zcore/result.hpp>

namespace zcore {

/**
 * @brief Plugin-load error codes.
 */
enum class LoadErrorCode : i32 {
    /// @brief Plugin identity or metadata shape is invalid.
    INVALID_DESCRIPTOR = 1,
    /// @brief Plugin artifact could not be found.
    NOT_FOUND = 2,
    /// @brief Plugin ABI is incompatible with required ABI.
    ABI_MISMATCH = 3,
    /// @brief Requested interface is not exposed by plugin.
    INTERFACE_UNSUPPORTED = 4,
    /// @brief Loader backend failed for a platform/runtime-specific reason.
    BACKEND_FAILURE = 5,
};

/// @brief Built-in plugin-load error domain identifier.
inline constexpr ErrorDomain kLoadErrorDomain{
        .id = 5U,
        .name = "plugin_load",
};

/**
 * @brief Constructs plugin-load-domain error payload.
 * @param code Plugin-load error code.
 * @param operation Operation identifier.
 * @param message Human-readable message.
 * @param file Source file path.
 * @param line Source line.
 * @return Structured plugin-load error.
 */
[[nodiscard]] constexpr Error
MakeLoadError(LoadErrorCode code, const char* operation, const char* message, const char* file = "", u32 line = 0U) noexcept
{
    return MakeError(kLoadErrorDomain, static_cast<i32>(code), MakeErrorContext("plugin_load", operation, message, file, line));
}

/**
 * @brief Result type for plugin loading operations.
 *
 * Success payload is a validated `PluginDescriptor`; failure payload is `Error`.
 */
using LoadResult = Result<PluginDescriptor, Error>;

/**
 * @brief Constructs successful load result from descriptor.
 */
[[nodiscard]] constexpr LoadResult Loaded(const PluginDescriptor& descriptor)
{
    return LoadResult::Success(descriptor);
}

/**
 * @brief Constructs successful load result from moved descriptor.
 */
[[nodiscard]] constexpr LoadResult Loaded(PluginDescriptor&& descriptor)
{
    return LoadResult::Success(static_cast<PluginDescriptor&&>(descriptor));
}

/**
 * @brief Constructs failed load result from copied error.
 */
[[nodiscard]] constexpr LoadResult LoadFailed(const Error& error)
{
    return LoadResult::Failure(error);
}

/**
 * @brief Constructs failed load result from moved error.
 */
[[nodiscard]] constexpr LoadResult LoadFailed(Error&& error)
{
    return LoadResult::Failure(static_cast<Error&&>(error));
}

} // namespace zcore
