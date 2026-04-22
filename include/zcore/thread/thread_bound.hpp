/**************************************************************************/
/*  thread_bound.hpp                                                      */
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
 * @file include/zcore/thread/thread_bound.hpp
 * @brief Runtime thread-affine value wrapper with checked access.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/thread_bound.hpp>
 * @endcode
 */

#pragma once

#include <thread>
#include <type_traits>
#include <utility>
#include <zcore/contract_violation.hpp>
#include <zcore/thread_id.hpp>
#include <zcore/type_constraints.hpp>

namespace zcore {

/**
 * @brief Value wrapper bound to the constructing thread.
 *
 * `ThreadBound<T>` stores one value and captures the constructing thread id.
 * Value access is allowed only from that owner thread.
 */
template <typename ValueT>
class [[nodiscard("ThreadBound must be handled explicitly.")]] ThreadBound final {
public:
    ZCORE_STATIC_REQUIRE(constraints::ValueObjectType<ValueT>,
                         "ThreadBound<ValueT> requires non-reference non-void object value type.");

    using ValueType = ValueT;

    /// @brief Constructs with default-initialized value on current thread.
    ThreadBound()
        requires(std::is_default_constructible_v<ValueT>)
            : OwnerNativeThreadId_(std::this_thread::get_id()), OwnerThreadId_(ThreadId::Current()), Value_()
    {
    }

    /// @brief Constructs with copied value on current thread.
    explicit ThreadBound(const ValueT& value)
        requires(std::is_copy_constructible_v<ValueT>)
            : OwnerNativeThreadId_(std::this_thread::get_id()), OwnerThreadId_(ThreadId::Current()), Value_(value)
    {
    }

    /// @brief Constructs with moved value on current thread.
    explicit ThreadBound(ValueT&& value)
        requires(std::is_move_constructible_v<ValueT>)
            : OwnerNativeThreadId_(std::this_thread::get_id()), OwnerThreadId_(ThreadId::Current()), Value_(std::move(value))
    {
    }

    ThreadBound(const ThreadBound&) = delete;
    ThreadBound& operator=(const ThreadBound&) = delete;
    ThreadBound(ThreadBound&&) = delete;
    ThreadBound& operator=(ThreadBound&&) = delete;
    ~ThreadBound() = default;

    /// @brief Returns owner thread id captured at construction.
    [[nodiscard]] ThreadId OwnerThreadId() const noexcept
    {
        return OwnerThreadId_;
    }

    /// @brief Returns `true` when called from owner thread.
    [[nodiscard]] bool IsOwnerThread() const noexcept
    {
        return std::this_thread::get_id() == OwnerNativeThreadId_;
    }

    /**
   * @brief Returns mutable value reference.
   * @pre Must be called from owner thread.
   */
    [[nodiscard]] ValueT& Value()
    {
        RequireOwnerThread("zcore::ThreadBound::Value() requires owner thread access");
        return Value_;
    }

    /**
   * @brief Returns immutable value reference.
   * @pre Must be called from owner thread.
   */
    [[nodiscard]] const ValueT& Value() const
    {
        RequireOwnerThread("zcore::ThreadBound::Value() requires owner thread access");
        return Value_;
    }

    /// @brief Returns mutable pointer when called from owner thread, otherwise `nullptr`.
    [[nodiscard]] ValueT* TryValue() noexcept
    {
        if (!IsOwnerThread()) {
            return nullptr;
        }
        return &Value_;
    }

    /// @brief Returns immutable pointer when called from owner thread, otherwise `nullptr`.
    [[nodiscard]] const ValueT* TryValue() const noexcept
    {
        if (!IsOwnerThread()) {
            return nullptr;
        }
        return &Value_;
    }

private:
    void RequireOwnerThread(const char* message) const
    {
        ZCORE_CONTRACT_REQUIRE(IsOwnerThread(), detail::ContractViolationCode::PRECONDITION, message);
    }

    std::thread::id OwnerNativeThreadId_;
    ThreadId OwnerThreadId_;
    ValueT Value_;
};

} // namespace zcore
