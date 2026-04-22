/**************************************************************************/
/*  memory/borrow.hpp                                                     */
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
 * @file include/zcore/memory/borrow.hpp
 * @brief Non-owning immutable borrow wrapper.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/borrow.hpp>
 * @endcode
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <zcore/contract_violation.hpp>
#include <zcore/non_null.hpp>
#include <zcore/type_constraints.hpp>

namespace zcore {

/**
 * @brief Non-owning immutable borrowed pointer wrapper.
 *
 * @tparam ValueT Borrowed value type (`void` supported).
 *
 * `Borrow<ValueT>` enforces non-null at construction and models an immutable
 * non-owning borrow over existing storage. It does not extend lifetime.
 */
template <typename ValueT>
class [[nodiscard("Borrow must be handled explicitly.")]] Borrow final {
public:
    ZCORE_STATIC_REQUIRE(!std::is_null_pointer_v<ValueT>, "Borrow<std::nullptr_t> is invalid.");
    ZCORE_STATIC_REQUIRE(constraints::BorrowablePointeeType<ValueT>,
                         "Borrow<ValueT> requires non-reference object-or-void pointee type.");

    using ElementType = ValueT;
    using PointerType = const ValueT*;

    Borrow() = delete;
    Borrow(std::nullptr_t) = delete;
    Borrow& operator=(std::nullptr_t) = delete;

    constexpr Borrow(const Borrow&) noexcept = default;
    constexpr Borrow& operator=(const Borrow&) noexcept = default;
    constexpr Borrow(Borrow&&) noexcept = default;
    constexpr Borrow& operator=(Borrow&&) noexcept = default;
    ~Borrow() = default;

    /**
   * @brief Constructs from a raw pointer and validates non-null.
   * @param pointer Pointer that must not be null.
   */
    constexpr explicit Borrow(PointerType pointer) noexcept : Ptr_(pointer)
    {
        EnsureNonNull(pointer);
    }

    /**
   * @brief Constructs from a const reference.
   * @param reference Referenced object.
   */
    template <typename ReferencedT = ValueT>
        requires(!std::is_void_v<ReferencedT> && !std::is_pointer_v<ReferencedT>)
    constexpr explicit Borrow(const ReferencedT& reference) noexcept : Ptr_(std::addressof(reference))
    {
    }

    /**
   * @brief Converts from compatible `Borrow<OtherT>`.
   * @param other Source borrow wrapper.
   */
    template <typename OtherT>
        requires(std::is_convertible_v<const OtherT*, PointerType>)
    constexpr Borrow(Borrow<OtherT> other) noexcept : Ptr_(other.Get())
    {
    }

    /**
   * @brief Converts from compatible `NonNull<OtherT>`.
   * @param other Source non-null wrapper.
   */
    template <typename OtherT>
        requires(std::is_convertible_v<OtherT*, PointerType>)
    constexpr Borrow(NonNull<OtherT> other) noexcept : Ptr_(other.Get())
    {
    }

    /**
   * @brief Constructs without validating non-null.
   * @param pointer Pointer expected to already satisfy invariant.
   * @return `Borrow` wrapping `pointer`.
   */
    [[nodiscard]] static constexpr Borrow UnsafeFromPointerUnchecked(PointerType pointer) noexcept
    {
        return Borrow(pointer, UncheckedTag{});
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

    /// @brief Returns pointer address as integer.
    [[nodiscard]] constexpr std::uintptr_t Address() const noexcept
    {
        return reinterpret_cast<std::uintptr_t>(Ptr_);
    }

    template <typename ReferencedT = ValueT>
        requires(!std::is_void_v<ReferencedT>)
    [[nodiscard]] constexpr const ReferencedT& operator*() const noexcept
    {
        return *Ptr_;
    }

    template <typename ReferencedT = ValueT>
        requires(!std::is_void_v<ReferencedT>)
    [[nodiscard]] constexpr const ReferencedT* operator->() const noexcept
    {
        return Ptr_;
    }

    [[nodiscard]] constexpr bool operator==(const Borrow&) const noexcept = default;

private:
    struct UncheckedTag final {};

    constexpr Borrow(PointerType pointer, UncheckedTag) noexcept : Ptr_(pointer)
    {
    }

    static constexpr void EnsureNonNull(PointerType pointer) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(pointer != nullptr,
                               detail::ContractViolationCode::NON_NULL_NULL_POINTER,
                               "zcore::Borrow requires a non-null pointer");
    }

    PointerType Ptr_;
};

} // namespace zcore
