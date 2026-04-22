/**************************************************************************/
/*  atomic.hpp                                                            */
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
 * @file include/zcore/thread/atomic.hpp
 * @brief Explicit atomic value wrapper with typed memory-order operations.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/atomic.hpp>
 * zcore::Atomic<zcore::u32> counter(0U);
 * @endcode
 */

#pragma once

#include <atomic>
#include <concepts>
#include <type_traits>
#include <zcore/type_constraints.hpp>

namespace zcore {

namespace detail {

template <typename ValueT>
concept AtomicFetchAddable = requires(std::atomic<ValueT>& atom, ValueT value) {
    { atom.fetch_add(value, std::memory_order_seq_cst) } -> std::same_as<ValueT>;
};

template <typename ValueT>
concept AtomicFetchSubable = requires(std::atomic<ValueT>& atom, ValueT value) {
    { atom.fetch_sub(value, std::memory_order_seq_cst) } -> std::same_as<ValueT>;
};

template <typename ValueT>
concept AtomicFetchAndable = requires(std::atomic<ValueT>& atom, ValueT value) {
    { atom.fetch_and(value, std::memory_order_seq_cst) } -> std::same_as<ValueT>;
};

template <typename ValueT>
concept AtomicFetchOrable = requires(std::atomic<ValueT>& atom, ValueT value) {
    { atom.fetch_or(value, std::memory_order_seq_cst) } -> std::same_as<ValueT>;
};

template <typename ValueT>
concept AtomicFetchXorable = requires(std::atomic<ValueT>& atom, ValueT value) {
    { atom.fetch_xor(value, std::memory_order_seq_cst) } -> std::same_as<ValueT>;
};

} // namespace detail

/**
 * @brief Typed atomic value with explicit memory-order controls.
 *
 * @tparam ValueT Trivially copyable atomic value type.
 */
template <typename ValueT>
class Atomic final {
public:
    ZCORE_STATIC_REQUIRE(std::is_trivially_copyable_v<ValueT>, "Atomic<ValueT> requires a trivially copyable value type.");
    ZCORE_STATIC_REQUIRE(constraints::NonConstType<ValueT>, "Atomic<ValueT> does not allow const-qualified value types.");
    ZCORE_STATIC_REQUIRE(!std::is_volatile_v<ValueT>, "Atomic<ValueT> does not allow volatile-qualified value types.");
    ZCORE_STATIC_REQUIRE(constraints::NonReferenceType<ValueT>, "Atomic<ValueT> does not allow reference value types.");

    using ValueType = ValueT;
    using NativeHandleType = std::atomic<ValueT>;

    /// @brief `true` when the atomic is always lock-free for `ValueT`.
    static constexpr bool kIsAlwaysLockFree = NativeHandleType::is_always_lock_free;

    /// @brief Constructs with zero/value-initialized state.
    constexpr Atomic() noexcept : Native_(ValueT{})
    {
    }

    /// @brief Constructs with explicit initial value.
    constexpr explicit Atomic(ValueT value) noexcept : Native_(value)
    {
    }

    Atomic(const Atomic&) = delete;
    Atomic& operator=(const Atomic&) = delete;
    Atomic(Atomic&&) = delete;
    Atomic& operator=(Atomic&&) = delete;
    ~Atomic() = default;

    /// @brief Atomically stores `value`.
    void Store(ValueT value, std::memory_order order = std::memory_order_seq_cst) noexcept
    {
        Native_.store(value, order);
    }

    /// @brief Atomically loads current value.
    [[nodiscard]] ValueT Load(std::memory_order order = std::memory_order_seq_cst) const noexcept
    {
        return Native_.load(order);
    }

    /// @brief Atomically exchanges value and returns previous value.
    [[nodiscard]] ValueT Exchange(ValueT value, std::memory_order order = std::memory_order_seq_cst) noexcept
    {
        return Native_.exchange(value, order);
    }

    /**
   * @brief Weak compare-and-exchange.
   * @return `true` when replacement succeeded.
   */
    bool CompareExchangeWeak(ValueT& expected,
                             ValueT desired,
                             std::memory_order success = std::memory_order_seq_cst,
                             std::memory_order failure = std::memory_order_seq_cst) noexcept
    {
        return Native_.compare_exchange_weak(expected, desired, success, failure);
    }

    /**
   * @brief Strong compare-and-exchange.
   * @return `true` when replacement succeeded.
   */
    bool CompareExchangeStrong(ValueT& expected,
                               ValueT desired,
                               std::memory_order success = std::memory_order_seq_cst,
                               std::memory_order failure = std::memory_order_seq_cst) noexcept
    {
        return Native_.compare_exchange_strong(expected, desired, success, failure);
    }

    /// @brief Atomically fetches previous value and adds `value`.
    [[nodiscard]] ValueT FetchAdd(ValueT value, std::memory_order order = std::memory_order_seq_cst) noexcept
        requires(detail::AtomicFetchAddable<ValueT>)
    {
        return Native_.fetch_add(value, order);
    }

    /// @brief Atomically fetches previous value and subtracts `value`.
    [[nodiscard]] ValueT FetchSub(ValueT value, std::memory_order order = std::memory_order_seq_cst) noexcept
        requires(detail::AtomicFetchSubable<ValueT>)
    {
        return Native_.fetch_sub(value, order);
    }

    /// @brief Atomically fetches previous value and ANDs with `value`.
    [[nodiscard]] ValueT FetchAnd(ValueT value, std::memory_order order = std::memory_order_seq_cst) noexcept
        requires(detail::AtomicFetchAndable<ValueT>)
    {
        return Native_.fetch_and(value, order);
    }

    /// @brief Atomically fetches previous value and ORs with `value`.
    [[nodiscard]] ValueT FetchOr(ValueT value, std::memory_order order = std::memory_order_seq_cst) noexcept
        requires(detail::AtomicFetchOrable<ValueT>)
    {
        return Native_.fetch_or(value, order);
    }

    /// @brief Atomically fetches previous value and XORs with `value`.
    [[nodiscard]] ValueT FetchXor(ValueT value, std::memory_order order = std::memory_order_seq_cst) noexcept
        requires(detail::AtomicFetchXorable<ValueT>)
    {
        return Native_.fetch_xor(value, order);
    }

    /// @brief Returns `true` when runtime instance is lock-free.
    [[nodiscard]] bool IsLockFree() const noexcept
    {
        return Native_.is_lock_free();
    }

    /// @brief Returns native atomic handle.
    [[nodiscard]] NativeHandleType& NativeHandle() noexcept
    {
        return Native_;
    }

    /// @brief Returns native atomic handle.
    [[nodiscard]] const NativeHandleType& NativeHandle() const noexcept
    {
        return Native_;
    }

private:
    NativeHandleType Native_;
};

} // namespace zcore
