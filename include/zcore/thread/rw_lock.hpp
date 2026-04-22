/**************************************************************************/
/*  rw_lock.hpp                                                           */
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
 * @file include/zcore/thread/rw_lock.hpp
 * @brief Reader/writer lock primitive with shared and exclusive modes.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/rw_lock.hpp>
 * @endcode
 */

#pragma once

#include <shared_mutex>

namespace zcore {

/**
 * @brief Reader/writer lock with explicit shared and exclusive operations.
 */
class RwLock final {
public:
    using NativeHandleType = std::shared_mutex;

    RwLock() = default;
    ~RwLock() = default;
    RwLock(const RwLock&) = delete;
    RwLock& operator=(const RwLock&) = delete;
    RwLock(RwLock&&) = delete;
    RwLock& operator=(RwLock&&) = delete;

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

    /// @brief Acquires shared lock (blocks until available).
    void LockShared()
    {
        Native_.lock_shared();
    }

    /// @brief Attempts shared lock acquisition without blocking.
    [[nodiscard]] bool TryLockShared()
    {
        return Native_.try_lock_shared();
    }

    /// @brief Releases shared lock.
    void UnlockShared()
    {
        Native_.unlock_shared();
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

    /// @brief SharedLockable adapter for standard lock utilities.
    void lock_shared()
    { // NOLINT(readability-identifier-naming)
        LockShared();
    }

    /// @brief SharedLockable adapter for standard lock utilities.
    [[nodiscard]] bool try_lock_shared()
    { // NOLINT(readability-identifier-naming)
        return TryLockShared();
    }

    /// @brief SharedLockable adapter for standard lock utilities.
    void unlock_shared()
    { // NOLINT(readability-identifier-naming)
        UnlockShared();
    }

    /// @brief Returns native lock handle.
    [[nodiscard]] NativeHandleType& NativeHandle() noexcept
    {
        return Native_;
    }

    /// @brief Returns native lock handle.
    [[nodiscard]] const NativeHandleType& NativeHandle() const noexcept
    {
        return Native_;
    }

private:
    std::shared_mutex Native_;
};

} // namespace zcore
