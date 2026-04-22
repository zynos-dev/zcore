/**************************************************************************/
/*  utility/defer.hpp                                                     */
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
 * @file include/zcore/utility/defer.hpp
 * @brief Scope-exit cleanup helper alias for concise defer-style cleanup.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/defer.hpp>
 * auto defer = zcore::MakeDefer([&]() noexcept { rollback(); });
 * @endcode
 */

#pragma once

#include <type_traits>
#include <utility>
#include <zcore/utility/scope_guard.hpp>

namespace zcore {

/**
 * @brief Alias for `ScopeGuard` used with defer-style naming at call sites.
 *
 * @tparam CleanupFnT Callable type invocable as `cleanup() noexcept`.
 */
template <typename CleanupFnT>
using Defer = ScopeGuard<CleanupFnT>;

/**
 * @brief Builds `Defer` from forwarded callable.
 */
template <typename CleanupFnT>
[[nodiscard]] constexpr auto MakeDefer(CleanupFnT&& cleanup) noexcept -> Defer<std::decay_t<CleanupFnT>>
{
    return Defer<std::decay_t<CleanupFnT>>(std::forward<CleanupFnT>(cleanup));
}

} // namespace zcore
