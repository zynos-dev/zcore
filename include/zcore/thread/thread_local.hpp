/**************************************************************************/
/*  thread_local.hpp                                                      */
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
 * @file include/zcore/thread/thread_local.hpp
 * @brief Per-thread typed storage slot keyed by value and optional tag type.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/thread_local.hpp>
 * @endcode
 */

#pragma once

#include <optional>
#include <type_traits>
#include <utility>
#include <zcore/contract_violation.hpp>
#include <zcore/type_constraints.hpp>

namespace zcore {

namespace detail {

template <typename ValueT, typename TagT>
struct ThreadLocalSlot final {
    static thread_local std::optional<ValueT> g_value;
};

template <typename ValueT, typename TagT>
thread_local std::optional<ValueT> ThreadLocalSlot<ValueT, TagT>::g_value = std::nullopt;

} // namespace detail

/**
 * @brief Per-thread storage slot keyed by `<ValueT, TagT>`.
 *
 * All `ThreadLocal<ValueT, TagT>` instances in a thread share one slot.
 * Different threads and different tag types isolate storage.
 */
template <typename ValueT, typename TagT = void>
class [[nodiscard("ThreadLocal must be handled explicitly.")]] ThreadLocal final {
public:
    ZCORE_STATIC_REQUIRE(constraints::ValueObjectType<ValueT>,
                         "ThreadLocal<ValueT, TagT> requires non-reference non-void object value type.");

    using ValueType = ValueT;
    using TagType = TagT;

    ThreadLocal() noexcept = default;
    ThreadLocal(const ThreadLocal&) noexcept = default;
    ThreadLocal& operator=(const ThreadLocal&) noexcept = default;
    ThreadLocal(ThreadLocal&&) noexcept = default;
    ThreadLocal& operator=(ThreadLocal&&) noexcept = default;
    ~ThreadLocal() = default;

    /// @brief Returns `true` when current thread slot contains a value.
    [[nodiscard]] bool HasValue() const noexcept
    {
        return Slot().has_value();
    }

    /// @brief Returns `true` when current thread slot is empty.
    [[nodiscard]] bool IsEmpty() const noexcept
    {
        return !HasValue();
    }

    /// @brief Clears current thread slot.
    void Reset() noexcept
    {
        Slot().reset();
    }

    /// @brief Sets current thread slot value by copy.
    void Set(const ValueT& value)
        requires(std::is_copy_constructible_v<ValueT>)
    {
        Slot().emplace(value);
    }

    /// @brief Sets current thread slot value by move.
    void Set(ValueT&& value)
        requires(std::is_move_constructible_v<ValueT>)
    {
        Slot().emplace(std::move(value));
    }

    /// @brief Constructs/replaces current thread slot value in-place.
    template <typename... ArgsT>
    ValueT& Emplace(ArgsT&&... args)
    {
        return Slot().emplace(std::forward<ArgsT>(args)...);
    }

    /**
   * @brief Returns current thread slot value.
   * @pre `HasValue()` must be true.
   */
    [[nodiscard]] ValueT& Value()
    {
        std::optional<ValueT>& slot = Slot();
        ZCORE_CONTRACT_REQUIRE(slot.has_value(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::ThreadLocal::Value() requires value in current thread slot");
        return slot.value();
    }

    /**
   * @brief Returns current thread slot value.
   * @pre `HasValue()` must be true.
   */
    [[nodiscard]] const ValueT& Value() const
    {
        const std::optional<ValueT>& slot = Slot();
        ZCORE_CONTRACT_REQUIRE(slot.has_value(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::ThreadLocal::Value() requires value in current thread slot");
        return slot.value();
    }

    /// @brief Returns pointer to current thread slot value or `nullptr`.
    [[nodiscard]] ValueT* TryValue() noexcept
    {
        std::optional<ValueT>& slot = Slot();
        if (!slot.has_value()) {
            return nullptr;
        }
        return &(*slot);
    }

    /// @brief Returns pointer to current thread slot value or `nullptr`.
    [[nodiscard]] const ValueT* TryValue() const noexcept
    {
        const std::optional<ValueT>& slot = Slot();
        if (!slot.has_value()) {
            return nullptr;
        }
        return &(*slot);
    }

    /// @brief Returns existing value or emplaces one in current thread slot.
    template <typename... ArgsT>
    ValueT& ValueOrEmplace(ArgsT&&... args)
    {
        std::optional<ValueT>& slot = Slot();
        if (slot.has_value()) {
            return slot.value();
        }
        return slot.emplace(std::forward<ArgsT>(args)...);
    }

private:
    [[nodiscard]] std::optional<ValueT>& Slot() noexcept
    {
        return detail::ThreadLocalSlot<ValueT, TagT>::g_value;
    }

    [[nodiscard]] const std::optional<ValueT>& Slot() const noexcept
    {
        return detail::ThreadLocalSlot<ValueT, TagT>::g_value;
    }
};

} // namespace zcore
