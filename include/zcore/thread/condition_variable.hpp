/**************************************************************************/
/*  condition_variable.hpp                                                */
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
 * @file include/zcore/thread/condition_variable.hpp
 * @brief Blocking condition notification primitive for `Mutex`-guarded state.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/condition_variable.hpp>
 * @endcode
 */

#pragma once

#include <chrono>
#include <condition_variable>
#include <utility>
#include <zcore/duration.hpp>
#include <zcore/thread/mutex.hpp>

namespace zcore {

/**
 * @brief Condition variable that interoperates with `std::unique_lock<zcore::Mutex>`.
 */
class ConditionVariable final {
public:
    using NativeHandleType = std::condition_variable_any;

    ConditionVariable() = default;
    ~ConditionVariable() = default;
    ConditionVariable(const ConditionVariable&) = delete;
    ConditionVariable& operator=(const ConditionVariable&) = delete;
    ConditionVariable(ConditionVariable&&) = delete;
    ConditionVariable& operator=(ConditionVariable&&) = delete;

    /// @brief Wakes one waiting thread.
    void NotifyOne() noexcept
    {
        Native_.notify_one();
    }

    /// @brief Wakes all waiting threads.
    void NotifyAll() noexcept
    {
        Native_.notify_all();
    }

    /// @brief Blocks until notification (spurious wakeups are permitted).
    void Wait(std::unique_lock<Mutex>& lock)
    {
        Native_.wait(lock);
    }

    /// @brief Blocks until `predicate()` becomes `true`.
    template <typename PredicateT>
    void Wait(std::unique_lock<Mutex>& lock, PredicateT&& predicate)
    {
        Native_.wait(lock, std::forward<PredicateT>(predicate));
    }

    /**
   * @brief Blocks until `predicate()` is `true` or timeout elapses.
   * @return `true` when predicate became true before timeout.
   */
    template <typename PredicateT>
    [[nodiscard]] bool WaitFor(std::unique_lock<Mutex>& lock, Duration timeout, PredicateT&& predicate)
    {
        const Duration::Rep rawNanoseconds = timeout.AsNanoseconds();
        const std::chrono::nanoseconds clampedTimeout =
                rawNanoseconds > 0LL ? std::chrono::nanoseconds(rawNanoseconds) : std::chrono::nanoseconds::zero();
        return Native_.wait_for(lock, clampedTimeout, std::forward<PredicateT>(predicate));
    }

    /// @brief Returns native condition variable handle.
    [[nodiscard]] NativeHandleType& NativeHandle() noexcept
    {
        return Native_;
    }

    /// @brief Returns native condition variable handle.
    [[nodiscard]] const NativeHandleType& NativeHandle() const noexcept
    {
        return Native_;
    }

private:
    std::condition_variable_any Native_;
};

} // namespace zcore
