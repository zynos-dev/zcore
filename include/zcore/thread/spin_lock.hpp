/**************************************************************************/
/*  spin_lock.hpp                                                         */
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
 * @file include/zcore/thread/spin_lock.hpp
 * @brief Lightweight busy-wait lock for short critical sections.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/spin_lock.hpp>
 * @endcode
 */

#pragma once

#include <atomic>
#include <thread>

namespace zcore {

/**
 * @brief Non-recursive spin lock with explicit acquire/release operations.
 */
class SpinLock final {
public:
    SpinLock() noexcept = default;
    ~SpinLock() = default;
    SpinLock(const SpinLock&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;
    SpinLock(SpinLock&&) = delete;
    SpinLock& operator=(SpinLock&&) = delete;

    /// @brief Acquires lock with busy-wait loop until available.
    void Lock() noexcept
    {
        while (Flag_.test_and_set(std::memory_order_acquire)) {
            while (Flag_.test(std::memory_order_relaxed)) {
                std::this_thread::yield();
            }
        }
    }

    /// @brief Attempts lock acquisition without blocking.
    [[nodiscard]] bool TryLock() noexcept
    {
        return !Flag_.test_and_set(std::memory_order_acquire);
    }

    /// @brief Releases lock.
    void Unlock() noexcept
    {
        Flag_.clear(std::memory_order_release);
    }

    /// @brief BasicLockable adapter for standard lock utilities.
    void lock() noexcept
    { // NOLINT(readability-identifier-naming)
        Lock();
    }

    /// @brief BasicLockable adapter for standard lock utilities.
    [[nodiscard]] bool try_lock() noexcept
    { // NOLINT(readability-identifier-naming)
        return TryLock();
    }

    /// @brief BasicLockable adapter for standard lock utilities.
    void unlock() noexcept
    { // NOLINT(readability-identifier-naming)
        Unlock();
    }

    /// @brief Returns `true` when lock is currently held.
    [[nodiscard]] bool IsLocked() const noexcept
    {
        return Flag_.test(std::memory_order_relaxed);
    }

private:
    std::atomic_flag Flag_ = ATOMIC_FLAG_INIT;
};

} // namespace zcore
