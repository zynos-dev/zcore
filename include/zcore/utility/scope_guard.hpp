/**************************************************************************/
/*  utility/scope_guard.hpp                                               */
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
 * @file include/zcore/utility/scope_guard.hpp
 * @brief Scope-exit cleanup guard for deterministic rollback paths.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/scope_guard.hpp>
 * auto guard = zcore::MakeScopeGuard([&]() noexcept { rollback(); });
 * @endcode
 */

#pragma once

#include <type_traits>
#include <utility>
#include <zcore/foundation.hpp>
#include <zcore/type_constraints.hpp>

namespace zcore {

/**
 * @brief Executes cleanup callable when leaving scope unless dismissed.
 *
 * @tparam CleanupFnT Callable type invocable as `cleanup() noexcept`.
 */
template <typename CleanupFnT>
class [[nodiscard("ScopeGuard must be bound to a variable.")]] ScopeGuard final {
public:
    ZCORE_STATIC_REQUIRE(constraints::ObjectType<CleanupFnT>&& constraints::NonReferenceType<CleanupFnT>,
                         "ScopeGuard<CleanupFnT> requires non-reference object callable type.");
    ZCORE_STATIC_REQUIRE(std::is_nothrow_invocable_v<CleanupFnT&>, "ScopeGuard<CleanupFnT> requires noexcept callable cleanup.");
    ZCORE_STATIC_REQUIRE(std::is_nothrow_move_constructible_v<CleanupFnT>,
                         "ScopeGuard<CleanupFnT> requires nothrow move-constructible callable type.");

    /// @brief Constructs active scope guard with moved callable.
    constexpr explicit ScopeGuard(CleanupFnT&& cleanup) noexcept : Cleanup_(std::move(cleanup))
    {
    }

    /// @brief Constructs active scope guard with copied callable.
    constexpr explicit ScopeGuard(const CleanupFnT& cleanup) noexcept(std::is_nothrow_copy_constructible_v<CleanupFnT>)
        requires(std::is_copy_constructible_v<CleanupFnT>)
            : Cleanup_(cleanup)
    {
    }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard& operator=(ScopeGuard&&) = delete;

    constexpr ScopeGuard(ScopeGuard&& other) noexcept : Cleanup_(std::move(other.Cleanup_)), Active_(other.Active_)
    {
        other.Active_ = false;
    }

    ~ScopeGuard() noexcept
    {
        if (Active_) {
            Cleanup_();
        }
    }

    /// @brief Returns `true` when cleanup will run on destruction.
    [[nodiscard]] constexpr bool Active() const noexcept
    {
        return Active_;
    }

    /// @brief Disables cleanup execution on destruction.
    constexpr void Dismiss() noexcept
    {
        Active_ = false;
    }

    /// @brief Re-enables cleanup execution on destruction.
    constexpr void Arm() noexcept
    {
        Active_ = true;
    }

    /// @brief Runs cleanup immediately once and disables further execution.
    constexpr void ExecuteNow() noexcept
    {
        if (!Active_) {
            return;
        }
        Active_ = false;
        Cleanup_();
    }

private:
    CleanupFnT Cleanup_;
    bool Active_{true};
};

/**
 * @brief Builds `ScopeGuard` from forwarded callable.
 */
template <typename CleanupFnT>
[[nodiscard]] constexpr auto MakeScopeGuard(CleanupFnT&& cleanup) noexcept -> ScopeGuard<std::decay_t<CleanupFnT>>
{
    return ScopeGuard<std::decay_t<CleanupFnT>>(std::forward<CleanupFnT>(cleanup));
}

} // namespace zcore
