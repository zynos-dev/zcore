/**************************************************************************/
/*  memory/borrow_mut.hpp                                                 */
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
 * @file include/zcore/memory/borrow_mut.hpp
 * @brief Non-owning mutable borrow wrapper.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/borrow_mut.hpp>
 * @endcode
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <zcore/contract_violation.hpp>
#include <zcore/memory/borrow.hpp>
#include <zcore/non_null.hpp>
#include <zcore/type_constraints.hpp>

namespace zcore {

/**
 * @brief Non-owning mutable borrowed pointer wrapper.
 *
 * @tparam ValueT Borrowed value type (`void` supported).
 *
 * `BorrowMut<ValueT>` enforces non-null at construction and models a mutable
 * non-owning borrow over existing storage. It does not extend lifetime.
 */
template <typename ValueT>
class [[nodiscard("BorrowMut must be handled explicitly.")]] BorrowMut final {
public:
    ZCORE_STATIC_REQUIRE(!std::is_null_pointer_v<ValueT>, "BorrowMut<std::nullptr_t> is invalid.");
    ZCORE_STATIC_REQUIRE(constraints::MutableBorrowablePointeeType<ValueT>,
                         "BorrowMut<ValueT> requires mutable non-reference object-or-void pointee type.");

    using ElementType = ValueT;
    using PointerType = ValueT*;

    BorrowMut() = delete;
    BorrowMut(std::nullptr_t) = delete;
    BorrowMut& operator=(std::nullptr_t) = delete;

    constexpr BorrowMut(const BorrowMut&) noexcept = default;
    constexpr BorrowMut& operator=(const BorrowMut&) noexcept = default;
    constexpr BorrowMut(BorrowMut&&) noexcept = default;
    constexpr BorrowMut& operator=(BorrowMut&&) noexcept = default;
    ~BorrowMut() = default;

    /**
   * @brief Constructs from a raw pointer and validates non-null.
   * @param pointer Pointer that must not be null.
   */
    constexpr explicit BorrowMut(PointerType pointer) noexcept : Ptr_(pointer)
    {
        EnsureNonNull(pointer);
    }

    /**
   * @brief Constructs from a mutable reference.
   * @param reference Referenced object.
   */
    template <typename ReferencedT = ValueT>
        requires(!std::is_void_v<ReferencedT> && !std::is_pointer_v<ReferencedT>)
    constexpr explicit BorrowMut(ReferencedT& reference) noexcept : Ptr_(std::addressof(reference))
    {
    }

    /**
   * @brief Converts from compatible `BorrowMut<OtherT>`.
   * @param other Source borrow wrapper.
   */
    template <typename OtherT>
        requires(std::is_convertible_v<OtherT*, PointerType>)
    constexpr BorrowMut(BorrowMut<OtherT> other) noexcept : Ptr_(other.Get())
    {
    }

    /**
   * @brief Converts from compatible `NonNull<OtherT>`.
   * @param other Source non-null wrapper.
   */
    template <typename OtherT>
        requires(std::is_convertible_v<OtherT*, PointerType>)
    constexpr BorrowMut(NonNull<OtherT> other) noexcept : Ptr_(other.Get())
    {
    }

    /**
   * @brief Constructs without validating non-null.
   * @param pointer Pointer expected to already satisfy invariant.
   * @return `BorrowMut` wrapping `pointer`.
   */
    [[nodiscard]] static constexpr BorrowMut UnsafeFromPointerUnchecked(PointerType pointer) noexcept
    {
        return BorrowMut(pointer, UncheckedTag{});
    }

    /// @brief Returns `true` when `pointer` is non-null.
    [[nodiscard]] static constexpr bool IsValidPointer(PointerType pointer) noexcept
    {
        return pointer != nullptr;
    }

    /// @brief Returns the wrapped raw pointer.
    [[nodiscard]] constexpr PointerType Get() const noexcept
    {
        return Ptr_;
    }

    [[nodiscard]] constexpr explicit operator PointerType() const noexcept
    {
        return Ptr_;
    }

    /**
   * @brief Returns immutable borrow view for the same pointee.
   */
    [[nodiscard]] constexpr Borrow<ValueT> AsBorrow() const noexcept
    {
        return Borrow<ValueT>::UnsafeFromPointerUnchecked(Ptr_);
    }

    [[nodiscard]] constexpr operator Borrow<ValueT>() const noexcept
    {
        return AsBorrow();
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

    [[nodiscard]] constexpr bool operator==(const BorrowMut&) const noexcept = default;

private:
    struct UncheckedTag final {};

    constexpr BorrowMut(PointerType pointer, UncheckedTag) noexcept : Ptr_(pointer)
    {
    }

    static constexpr void EnsureNonNull(PointerType pointer) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(pointer != nullptr,
                               detail::ContractViolationCode::NON_NULL_NULL_POINTER,
                               "zcore::BorrowMut requires a non-null pointer");
    }

    PointerType Ptr_;
};

} // namespace zcore
