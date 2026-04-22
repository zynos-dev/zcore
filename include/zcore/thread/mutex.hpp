/**************************************************************************/
/*  mutex.hpp                                                             */
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
 * @file include/zcore/thread/mutex.hpp
 * @brief Non-recursive mutual exclusion primitive.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/mutex.hpp>
 * @endcode
 */

#pragma once

#include <mutex>

namespace zcore {

/**
 * @brief Non-recursive mutual exclusion primitive.
 */
class Mutex final {
public:
    using NativeHandleType = std::mutex;

    Mutex() noexcept = default;
    ~Mutex() = default;
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;
    Mutex(Mutex&&) = delete;
    Mutex& operator=(Mutex&&) = delete;

    /// @brief Acquires exclusive lock (blocks until available).
    void Lock()
    {
        Native_.lock();
    }

    /// @brief Attempts exclusive lock acquisition without blocking.
    [[nodiscard]] bool TryLock()
    {
        return Native_.try_lock();
    }

    /// @brief Releases exclusive lock.
    void Unlock()
    {
        Native_.unlock();
    }

    /// @brief BasicLockable adapter for standard lock utilities.
    void lock()
    { // NOLINT(readability-identifier-naming)
        Lock();
    }

    /// @brief BasicLockable adapter for standard lock utilities.
    [[nodiscard]] bool try_lock()
    { // NOLINT(readability-identifier-naming)
        return TryLock();
    }

    /// @brief BasicLockable adapter for standard lock utilities.
    void unlock()
    { // NOLINT(readability-identifier-naming)
        Unlock();
    }

    /// @brief Returns native mutex handle.
    [[nodiscard]] NativeHandleType& NativeHandle() noexcept
    {
        return Native_;
    }

    /// @brief Returns native mutex handle.
    [[nodiscard]] const NativeHandleType& NativeHandle() const noexcept
    {
        return Native_;
    }

private:
    std::mutex Native_;
};

} // namespace zcore
