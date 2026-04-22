/**************************************************************************/
/*  non_null.hpp                                                          */
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
 * @file include/zcore/container/non_null.hpp
 * @brief Non-null pointer wrapper enforcing explicit pointer invariants.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/non_null.hpp>
 * int value = 7;
 * zcore::NonNull<int> ptr(&value);
 * @endcode
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <zcore/contract_violation.hpp>
#include <zcore/type_constraints.hpp>

namespace zcore {

/**
 * @brief Non-null pointer wrapper with explicit casts.
 *
 * @tparam ValueT Pointed-to type (`void` supported).
 *
 * This type enforces a non-null invariant at construction time. It does not
 * own the pointee and does not model lifetime.
 */
template <typename ValueT>
class [[nodiscard("NonNull must be handled explicitly.")]] NonNull final {
public:
    ZCORE_STATIC_REQUIRE(!std::is_null_pointer_v<ValueT>, "NonNull<std::nullptr_t> is invalid.");
    ZCORE_STATIC_REQUIRE(constraints::BorrowablePointeeType<ValueT>,
                         "NonNull<ValueT> requires non-reference object-or-void pointee type.");

    using ElementType = ValueT;

    NonNull() = delete;
    NonNull(std::nullptr_t) = delete;
    NonNull& operator=(std::nullptr_t) = delete;

    constexpr NonNull(const NonNull&) noexcept = default;
    constexpr NonNull& operator=(const NonNull&) noexcept = default;
    constexpr NonNull(NonNull&&) noexcept = default;
    constexpr NonNull& operator=(NonNull&&) noexcept = default;
    ~NonNull() = default;

    /**
   * @brief Constructs from a raw pointer and validates non-null.
   * @param pointer Pointer that must not be null.
   */
    constexpr explicit NonNull(ValueT* pointer) noexcept : Ptr_(pointer)
    {
        EnsureNonNull(pointer);
    }

    /**
   * @brief Constructs from a reference.
   * @param reference Referenced object.
   */
    template <typename ReferencedT = ValueT>
        requires(!std::is_void_v<ReferencedT>)
    constexpr explicit NonNull(ReferencedT& reference) noexcept : Ptr_(std::addressof(reference))
    {
    }

    /**
   * @brief Converts from compatible `NonNull<OtherT>`.
   * @param other Source non-null pointer wrapper.
   */
    template <typename OtherT>
        requires(std::is_convertible_v<OtherT*, ValueT*>)
    constexpr NonNull(NonNull<OtherT> other) noexcept : Ptr_(other.Get())
    {
    }

    /**
   * @brief Constructs without validating non-null.
   * @param pointer Pointer expected to already satisfy invariant.
   * @return `NonNull` wrapping `pointer`.
   */
    [[nodiscard]] static constexpr NonNull UnsafeFromPointerUnchecked(ValueT* pointer) noexcept
    {
        return NonNull(pointer, UncheckedTag{});
    }

    /// @brief Returns `true` when `pointer` is non-null.
    [[nodiscard]] static constexpr bool IsValidPointer(const ValueT* pointer) noexcept
    {
        return pointer != nullptr;
    }

    /// @brief Returns the wrapped raw pointer.
    [[nodiscard]] constexpr ValueT* Get() const noexcept
    {
        return Ptr_;
    }

    [[nodiscard]] constexpr explicit operator ValueT*() const noexcept
    {
        return Ptr_;
    }

    /// @brief Returns pointer address as integer.
    [[nodiscard]] constexpr std::uintptr_t Address() const noexcept
    {
        return reinterpret_cast<std::uintptr_t>(Ptr_);
    }

    template <typename ReferencedT = ValueT>
        requires(!std::is_void_v<ReferencedT>)
    [[nodiscard]] constexpr ReferencedT& operator*() const noexcept
    {
        return *Ptr_;
    }

    template <typename ReferencedT = ValueT>
        requires(!std::is_void_v<ReferencedT>)
    [[nodiscard]] constexpr ReferencedT* operator->() const noexcept
    {
        return Ptr_;
    }

    /**
   * @brief Performs `static_cast` on wrapped pointer type.
   * @tparam OtherT Target pointee type.
   * @return Converted non-null pointer.
   */
    template <typename OtherT>
        requires(std::is_convertible_v<ValueT*, OtherT*>)
    [[nodiscard]] constexpr NonNull<OtherT> StaticCast() const noexcept
    {
        return NonNull<OtherT>::UnsafeFromPointerUnchecked(static_cast<OtherT*>(Ptr_));
    }

    /**
   * @brief Performs `reinterpret_cast` on wrapped pointer type.
   * @tparam OtherT Target pointee type.
   * @return Reinterpreted non-null pointer.
   */
    template <typename OtherT>
    [[nodiscard]] constexpr NonNull<OtherT> ReinterpretCast() const noexcept
    {
        return NonNull<OtherT>::UnsafeFromPointerUnchecked(reinterpret_cast<OtherT*>(Ptr_));
    }

    /**
   * @brief Removes const qualification from pointee type.
   * @return Mutable non-null pointer.
   */
    template <typename ReferencedT = ValueT>
        requires(std::is_const_v<ReferencedT>)
    [[nodiscard]] constexpr NonNull<std::remove_const_t<ReferencedT>> ConstCast() const noexcept
    {
        using MutableT = std::remove_const_t<ReferencedT>;
        return NonNull<MutableT>::UnsafeFromPointerUnchecked(const_cast<MutableT*>(Ptr_));
    }

    [[nodiscard]] constexpr bool operator==(const NonNull&) const noexcept = default;

private:
    struct UncheckedTag final {};

    constexpr NonNull(ValueT* pointer, UncheckedTag) noexcept : Ptr_(pointer)
    {
    }

    static constexpr void EnsureNonNull(const ValueT* pointer) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(pointer != nullptr,
                               detail::ContractViolationCode::NON_NULL_NULL_POINTER,
                               "zcore::NonNull requires a non-null pointer");
    }

    ValueT* Ptr_;
};

} // namespace zcore
