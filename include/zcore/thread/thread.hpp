/**************************************************************************/
/*  thread.hpp                                                            */
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
 * @file include/zcore/thread/thread.hpp
 * @brief Move-only owning runtime thread handle with explicit join/detach.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/thread.hpp>
 * @endcode
 */

#pragma once

#include <functional>
#include <thread>
#include <utility>
#include <zcore/contract_violation.hpp>
#include <zcore/thread/thread_id.hpp>

namespace zcore {

/**
 * @brief Move-only owning runtime thread handle.
 *
 * Destruction requires non-joinable state; callers must explicitly `Join()` or
 * `Detach()` before destruction.
 */
class Thread final {
public:
    using NativeHandleType = std::thread;

    /// @brief Constructs empty (non-joinable) handle.
    Thread() noexcept = default;

    /// @brief Adopts existing native thread handle.
    explicit Thread(NativeHandleType&& native) noexcept : Native_(std::move(native))
    {
    }

    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    Thread(Thread&&) noexcept = default;

    Thread& operator=(Thread&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        ZCORE_CONTRACT_REQUIRE(!Native_.joinable(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Thread move-assignment requires target to be non-joinable");
        Native_ = std::move(other.Native_);
        return *this;
    }

    ~Thread()
    {
        ZCORE_CONTRACT_REQUIRE(!Native_.joinable(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Thread destruction requires Join() or Detach() before destruction");
    }

    /// @brief Returns `true` when handle owns an active joinable thread.
    [[nodiscard]] bool Joinable() const noexcept
    {
        return Native_.joinable();
    }

    /// @brief Returns owned thread id token, or invalid when non-joinable.
    [[nodiscard]] ThreadId GetId() const noexcept
    {
        return ToThreadId(Native_.get_id());
    }

    /// @brief Returns current calling thread id token.
    [[nodiscard]] static ThreadId CurrentId() noexcept
    {
        return ThreadId::Current();
    }

    /**
   * @brief Joins owned thread.
   * @pre `Joinable()`.
   * @pre Must not be called from the same thread.
   */
    void Join()
    {
        ZCORE_CONTRACT_REQUIRE(Joinable(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Thread::Join() requires joinable thread");
        ZCORE_CONTRACT_REQUIRE(Native_.get_id() != std::this_thread::get_id(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Thread::Join() cannot join the current thread");
        Native_.join();
    }

    /**
   * @brief Detaches owned thread.
   * @pre `Joinable()`.
   */
    void Detach()
    {
        ZCORE_CONTRACT_REQUIRE(Joinable(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Thread::Detach() requires joinable thread");
        Native_.detach();
    }

    /// @brief Swaps owned native handles.
    void Swap(Thread& other) noexcept
    {
        Native_.swap(other.Native_);
    }

    /// @brief Returns native thread handle.
    [[nodiscard]] NativeHandleType& NativeHandle() noexcept
    {
        return Native_;
    }

    /// @brief Returns native thread handle.
    [[nodiscard]] const NativeHandleType& NativeHandle() const noexcept
    {
        return Native_;
    }

private:
    [[nodiscard]] static ThreadId ToThreadId(const std::thread::id id) noexcept
    {
        if (id == std::thread::id{}) {
            return ThreadId::Invalid();
        }

        const std::size_t hashed = std::hash<std::thread::id>{}(id);
        const u64 value = static_cast<u64>(hashed);
        return ThreadId::FromRawUnchecked(value == ThreadId::kInvalidValue ? 1ULL : value);
    }

    NativeHandleType Native_;
};

} // namespace zcore
