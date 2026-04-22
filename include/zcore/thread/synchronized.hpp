/**************************************************************************/
/*  synchronized.hpp                                                      */
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
 * @file include/zcore/thread/synchronized.hpp
 * @brief Mutex-guarded value wrapper with explicit lock-scoped access.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/synchronized.hpp>
 * @endcode
 */

#pragma once

#include <mutex>
#include <type_traits>
#include <utility>
#include <zcore/mutex.hpp>
#include <zcore/option.hpp>
#include <zcore/type_constraints.hpp>

namespace zcore {

/**
 * @brief Mutex-guarded value wrapper.
 *
 * `Synchronized<T>` owns one value and one mutex. Access is only available
 * through lock guard objects returned by `Lock()`/`TryLock()`.
 */
template <typename ValueT>
class [[nodiscard("Synchronized must be handled explicitly.")]] Synchronized final {
public:
    ZCORE_STATIC_REQUIRE(constraints::ValueObjectType<ValueT>,
                         "Synchronized<ValueT> requires non-reference non-void object value type.");

    using ValueType = ValueT;
    using MutexType = Mutex;

    class Guard final {
    public:
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
        Guard(Guard&&) noexcept = default;
        Guard& operator=(Guard&&) noexcept = default;
        ~Guard() = default;

        [[nodiscard]] ValueT& Value() noexcept
        {
            return *Ptr_;
        }

        [[nodiscard]] ValueT* operator->() noexcept
        {
            return Ptr_;
        }

        [[nodiscard]] ValueT& operator*() noexcept
        {
            return *Ptr_;
        }

    private:
        friend class Synchronized;

        Guard(Mutex& mutex, ValueT& value) : Lock_(mutex), Ptr_(&value)
        {
        }
        Guard(std::unique_lock<Mutex>&& lock, ValueT& value) : Lock_(std::move(lock)), Ptr_(&value)
        {
        }

        std::unique_lock<Mutex> Lock_;
        ValueT* Ptr_;
    };

    class ConstGuard final {
    public:
        ConstGuard(const ConstGuard&) = delete;
        ConstGuard& operator=(const ConstGuard&) = delete;
        ConstGuard(ConstGuard&&) noexcept = default;
        ConstGuard& operator=(ConstGuard&&) noexcept = default;
        ~ConstGuard() = default;

        [[nodiscard]] const ValueT& Value() const noexcept
        {
            return *Ptr_;
        }

        [[nodiscard]] const ValueT* operator->() const noexcept
        {
            return Ptr_;
        }

        [[nodiscard]] const ValueT& operator*() const noexcept
        {
            return *Ptr_;
        }

    private:
        friend class Synchronized;

        ConstGuard(Mutex& mutex, const ValueT& value) : Lock_(mutex), Ptr_(&value)
        {
        }
        ConstGuard(std::unique_lock<Mutex>&& lock, const ValueT& value) : Lock_(std::move(lock)), Ptr_(&value)
        {
        }

        std::unique_lock<Mutex> Lock_;
        const ValueT* Ptr_;
    };

    /// @brief Constructs with default-initialized value.
    Synchronized()
        requires(std::is_default_constructible_v<ValueT>)
            : Value_()
    {
    }

    /// @brief Constructs with copied value.
    explicit Synchronized(const ValueT& value)
        requires(std::is_copy_constructible_v<ValueT>)
            : Value_(value)
    {
    }

    /// @brief Constructs with moved value.
    explicit Synchronized(ValueT&& value)
        requires(std::is_move_constructible_v<ValueT>)
            : Value_(std::move(value))
    {
    }

    Synchronized(const Synchronized&) = delete;
    Synchronized& operator=(const Synchronized&) = delete;
    Synchronized(Synchronized&&) = delete;
    Synchronized& operator=(Synchronized&&) = delete;
    ~Synchronized() = default;

    /// @brief Acquires lock and returns mutable guard.
    [[nodiscard]] Guard Lock()
    {
        return Guard(Mutex_, Value_);
    }

    /// @brief Attempts to acquire lock and returns mutable guard when successful.
    [[nodiscard]] Option<Guard> TryLock()
    {
        std::unique_lock<Mutex> lock(Mutex_, std::try_to_lock);
        if (!lock.owns_lock()) {
            return None;
        }
        return Option<Guard>(Guard(std::move(lock), Value_));
    }

    /// @brief Acquires lock and returns immutable guard.
    [[nodiscard]] ConstGuard Lock() const
    {
        return ConstGuard(Mutex_, Value_);
    }

    /// @brief Attempts to acquire lock and returns immutable guard when successful.
    [[nodiscard]] Option<ConstGuard> TryLock() const
    {
        std::unique_lock<Mutex> lock(Mutex_, std::try_to_lock);
        if (!lock.owns_lock()) {
            return None;
        }
        return Option<ConstGuard>(ConstGuard(std::move(lock), Value_));
    }

    /// @brief Returns underlying mutex (interop only).
    [[nodiscard]] Mutex& NativeMutex() noexcept
    {
        return Mutex_;
    }

    /// @brief Returns underlying mutex (interop only).
    [[nodiscard]] const Mutex& NativeMutex() const noexcept
    {
        return Mutex_;
    }

private:
    mutable Mutex Mutex_{};
    ValueT Value_;
};

} // namespace zcore
