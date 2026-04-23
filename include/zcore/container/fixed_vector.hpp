/**************************************************************************/
/*  fixed_vector.hpp                                                      */
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
 * @file include/zcore/container/fixed_vector.hpp
 * @brief Deterministic fixed-capacity contiguous value container.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/container/fixed_vector.hpp>
 * zcore::FixedVector<int, 4> values;
 * values.PushBack(10);
 * @endcode
 */

#pragma once

#include <concepts>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <zcore/contract_violation.hpp>
#include <zcore/foundation.hpp>
#include <zcore/option.hpp>
#include <zcore/slice.hpp>
#include <zcore/type_constraints.hpp>

namespace zcore {

/**
 * @brief Fixed-capacity contiguous container with explicit overflow behavior.
 *
 * @tparam ValueT Element type.
 * @tparam CapacityV Maximum element count.
 */
template <typename ValueT, usize CapacityV>
class [[nodiscard("FixedVector must be handled explicitly.")]] FixedVector final {
public:
    ZCORE_STATIC_REQUIRE(constraints::MutableValueObjectType<ValueT>,
                         "FixedVector<ValueT, CapacityV> requires mutable non-void object element type.");

    using ValueType = ValueT;
    using SizeType = usize;
    using Pointer = ValueT*;
    using ConstPointer = const ValueT*;
    using Iterator = ValueT*;
    using ConstIterator = const ValueT*;

    /// @brief Compile-time capacity bound.
    static constexpr usize kCapacity = CapacityV;

    /// @brief Constructs an empty fixed vector.
    constexpr FixedVector() noexcept = default;

    constexpr FixedVector(const FixedVector& other)
        requires(std::is_copy_constructible_v<ValueT>)
    {
        CopyFrom(other);
    }

    constexpr FixedVector(FixedVector&& other) noexcept(std::is_nothrow_move_constructible_v<ValueT>)
        requires(std::is_move_constructible_v<ValueT>)
    {
        MoveFrom(std::move(other));
    }

    constexpr FixedVector& operator=(const FixedVector& other)
        requires(std::is_copy_constructible_v<ValueT>)
    {
        if (this == &other) {
            return *this;
        }
        Clear();
        CopyFrom(other);
        return *this;
    }

    constexpr FixedVector& operator=(FixedVector&& other) noexcept(std::is_nothrow_move_constructible_v<ValueT>)
        requires(std::is_move_constructible_v<ValueT>)
    {
        if (this == &other) {
            return *this;
        }
        Clear();
        MoveFrom(std::move(other));
        return *this;
    }

    constexpr ~FixedVector()
    {
        Clear();
    }

    /// @brief Returns compile-time capacity.
    [[nodiscard]] static constexpr usize Capacity() noexcept
    {
        return kCapacity;
    }

    /// @brief Returns current element count.
    [[nodiscard]] constexpr usize Size() const noexcept
    {
        return Size_;
    }

    [[nodiscard]] constexpr usize size() const noexcept
    {
        return Size();
    }

    /// @brief Returns remaining insertion slots before full.
    [[nodiscard]] constexpr usize RemainingCapacity() const noexcept
    {
        return kCapacity - Size_;
    }

    /// @brief Returns `true` when no elements are present.
    [[nodiscard]] constexpr bool Empty() const noexcept
    {
        return Size_ == 0;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return Empty();
    }

    /// @brief Returns `true` when `Size() == Capacity()`.
    [[nodiscard]] constexpr bool Full() const noexcept
    {
        return Size_ == kCapacity;
    }

    [[nodiscard]] constexpr bool full() const noexcept
    {
        return Full();
    }

    /// @brief Returns pointer to contiguous element storage or `nullptr` when empty.
    [[nodiscard]] constexpr Pointer Data() noexcept
    {
        if (Empty()) {
            return nullptr;
        }
        return PtrAt(0);
    }

    [[nodiscard]] constexpr ConstPointer Data() const noexcept
    {
        if (Empty()) {
            return nullptr;
        }
        return PtrAt(0);
    }

    [[nodiscard]] constexpr Pointer data() noexcept
    {
        return Data();
    }

    [[nodiscard]] constexpr ConstPointer data() const noexcept
    {
        return Data();
    }

    /// @brief Returns iterator to first element.
    [[nodiscard]] constexpr Iterator begin() noexcept
    {
        return Data();
    }

    /// @brief Returns iterator past last element.
    [[nodiscard]] constexpr Iterator end() noexcept
    {
        Iterator first = begin();
        return first == nullptr ? nullptr : first + Size_;
    }

    /// @brief Returns const iterator to first element.
    [[nodiscard]] constexpr ConstIterator begin() const noexcept
    {
        return Data();
    }

    /// @brief Returns const iterator past last element.
    [[nodiscard]] constexpr ConstIterator end() const noexcept
    {
        ConstIterator first = begin();
        return first == nullptr ? nullptr : first + Size_;
    }

    [[nodiscard]] constexpr ConstIterator cbegin() const noexcept
    {
        return begin();
    }

    [[nodiscard]] constexpr ConstIterator cend() const noexcept
    {
        return end();
    }

    /**
   * @brief Checked indexed access.
   * @pre `index < Size()`.
   */
    [[nodiscard]] constexpr ValueT& operator[](usize index) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(index < Size_,
                               detail::ContractViolationCode::FIXED_VECTOR_INDEX_OUT_OF_BOUNDS,
                               "zcore::FixedVector::operator[] index out of bounds");
        return *PtrAt(index);
    }

    /**
   * @brief Checked indexed access.
   * @pre `index < Size()`.
   */
    [[nodiscard]] constexpr const ValueT& operator[](usize index) const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(index < Size_,
                               detail::ContractViolationCode::FIXED_VECTOR_INDEX_OUT_OF_BOUNDS,
                               "zcore::FixedVector::operator[] index out of bounds");
        return *PtrAt(index);
    }

    /// @brief Returns pointer to element at `index` or `nullptr` when out-of-range.
    [[nodiscard]] constexpr Pointer TryAt(usize index) noexcept
    {
        return index < Size_ ? PtrAt(index) : nullptr;
    }

    /// @brief Returns pointer to element at `index` or `nullptr` when out-of-range.
    [[nodiscard]] constexpr ConstPointer TryAt(usize index) const noexcept
    {
        return index < Size_ ? PtrAt(index) : nullptr;
    }

    /**
   * @brief Returns first element.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr ValueT& Front() noexcept
    {
        RequireNotEmpty("zcore::FixedVector::Front() requires non-empty container");
        return *PtrAt(0);
    }

    /**
   * @brief Returns first element.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr const ValueT& Front() const noexcept
    {
        RequireNotEmpty("zcore::FixedVector::Front() requires non-empty container");
        return *PtrAt(0);
    }

    /**
   * @brief Returns last element.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr ValueT& Back() noexcept
    {
        RequireNotEmpty("zcore::FixedVector::Back() requires non-empty container");
        return *PtrAt(Size_ - 1);
    }

    /**
   * @brief Returns last element.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr const ValueT& Back() const noexcept
    {
        RequireNotEmpty("zcore::FixedVector::Back() requires non-empty container");
        return *PtrAt(Size_ - 1);
    }

    /**
   * @brief Attempts to append a copied value.
   * @return `true` on success, `false` when full.
   */
    [[nodiscard]] constexpr bool TryPushBack(const ValueT& value)
        requires(std::is_copy_constructible_v<ValueT>)
    {
        if (Full()) {
            return false;
        }
        std::construct_at(PtrAt(Size_), value);
        ++Size_;
        return true;
    }

    /**
   * @brief Attempts to append a moved value.
   * @return `true` on success, `false` when full.
   */
    [[nodiscard]] constexpr bool TryPushBack(ValueT&& value)
        requires(std::is_move_constructible_v<ValueT>)
    {
        if (Full()) {
            return false;
        }
        std::construct_at(PtrAt(Size_), std::move(value));
        ++Size_;
        return true;
    }

    /**
   * @brief Appends a copied value.
   * @pre `!Full()`.
   */
    constexpr void PushBack(const ValueT& value)
        requires(std::is_copy_constructible_v<ValueT>)
    {
        ZCORE_CONTRACT_REQUIRE(TryPushBack(value),
                               detail::ContractViolationCode::FIXED_VECTOR_CAPACITY_EXCEEDED,
                               "zcore::FixedVector::PushBack() exceeds fixed capacity");
    }

    /**
   * @brief Appends a moved value.
   * @pre `!Full()`.
   */
    constexpr void PushBack(ValueT&& value)
        requires(std::is_move_constructible_v<ValueT>)
    {
        ZCORE_CONTRACT_REQUIRE(TryPushBack(std::move(value)),
                               detail::ContractViolationCode::FIXED_VECTOR_CAPACITY_EXCEEDED,
                               "zcore::FixedVector::PushBack() exceeds fixed capacity");
    }

    /**
   * @brief Attempts in-place append.
   * @return Pointer to new element, or `nullptr` when full.
   */
    template <typename... ArgsT>
        requires(std::is_constructible_v<ValueT, ArgsT && ...>)
    [[nodiscard]] constexpr ValueT* TryEmplaceBack(ArgsT&&... args)
    {
        if (Full()) {
            return nullptr;
        }
        ValueT* slot = PtrAt(Size_);
        std::construct_at(slot, std::forward<ArgsT>(args)...);
        ++Size_;
        return slot;
    }

    /**
   * @brief In-place appends a value.
   * @pre `!Full()`.
   */
    template <typename... ArgsT>
        requires(std::is_constructible_v<ValueT, ArgsT && ...>)
    [[nodiscard]] constexpr ValueT& EmplaceBack(ArgsT&&... args)
    {
        ValueT* slot = TryEmplaceBack(std::forward<ArgsT>(args)...);
        ZCORE_CONTRACT_REQUIRE(slot != nullptr,
                               detail::ContractViolationCode::FIXED_VECTOR_CAPACITY_EXCEEDED,
                               "zcore::FixedVector::EmplaceBack() exceeds fixed capacity");
        return *slot;
    }

    /**
   * @brief Attempts to remove last element.
   * @return `true` on success, `false` when empty.
   */
    [[nodiscard]] constexpr bool TryPopBack() noexcept
    {
        if (Empty()) {
            return false;
        }
        --Size_;
        std::destroy_at(PtrAt(Size_));
        return true;
    }

    /**
   * @brief Removes last element.
   * @pre `!Empty()`.
   */
    constexpr void PopBack() noexcept
    {
        ZCORE_CONTRACT_REQUIRE(TryPopBack(),
                               detail::ContractViolationCode::FIXED_VECTOR_EMPTY_ACCESS,
                               "zcore::FixedVector::PopBack() requires non-empty container");
    }

    /**
   * @brief Moves out last element if present.
   * @return `None` when empty.
   */
    [[nodiscard]] constexpr Option<ValueT> TryPopBackValue()
        requires(std::is_move_constructible_v<ValueT>)
    {
        if (Empty()) {
            return None;
        }
        --Size_;
        ValueT* slot = PtrAt(Size_);
        Option<ValueT> moved(std::move(*slot));
        std::destroy_at(slot);
        return moved;
    }

    /// @brief Destroys all elements and resets size to zero.
    constexpr void Clear() noexcept
    {
        while (Size_ > 0) {
            --Size_;
            std::destroy_at(PtrAt(Size_));
        }
    }

    /// @brief Returns immutable view over contained elements.
    [[nodiscard]] constexpr Slice<const ValueT> AsSlice() const noexcept
    {
        if (Empty()) {
            return Slice<const ValueT>::Empty();
        }
        return Slice<const ValueT>::FromRawUnchecked(Data(), Size_);
    }

    /// @brief Returns mutable view over contained elements.
    [[nodiscard]] constexpr SliceMut<ValueT> AsSliceMut() noexcept
    {
        if (Empty()) {
            return SliceMut<ValueT>::Empty();
        }
        return SliceMut<ValueT>::FromRawUnchecked(Data(), Size_);
    }

    [[nodiscard]] constexpr operator Slice<const ValueT>() const noexcept
    {
        return AsSlice();
    }

    [[nodiscard]] constexpr bool operator==(const FixedVector& other) const
        requires(std::equality_comparable<ValueT>)
    {
        if (Size_ != other.Size_) {
            return false;
        }
        for (usize index = 0; index < Size_; ++index) {
            if (!(*PtrAt(index) == *other.PtrAt(index))) {
                return false;
            }
        }
        return true;
    }

private:
    static constexpr usize kStorageCount = kCapacity == 0 ? 1 : kCapacity;

    [[nodiscard]] constexpr Pointer PtrAt(usize index) noexcept
    {
        return std::launder(reinterpret_cast<Pointer>(Storage_ + (index * sizeof(ValueT))));
    }

    [[nodiscard]] constexpr ConstPointer PtrAt(usize index) const noexcept
    {
        return std::launder(reinterpret_cast<ConstPointer>(Storage_ + (index * sizeof(ValueT))));
    }

    constexpr void RequireNotEmpty(const char* message) const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(!Empty(), detail::ContractViolationCode::FIXED_VECTOR_EMPTY_ACCESS, message);
    }

    constexpr void CopyFrom(const FixedVector& other)
        requires(std::is_copy_constructible_v<ValueT>)
    {
        for (usize index = 0; index < other.Size_; ++index) {
            std::construct_at(PtrAt(index), *other.PtrAt(index));
            ++Size_;
        }
    }

    constexpr void MoveFrom(FixedVector&& other)
        requires(std::is_move_constructible_v<ValueT>)
    {
        for (usize index = 0; index < other.Size_; ++index) {
            std::construct_at(PtrAt(index), std::move(*other.PtrAt(index)));
            ++Size_;
        }
    }

    alignas(ValueT) std::byte Storage_[sizeof(ValueT) * kStorageCount];
    usize Size_{0};
};

} // namespace zcore
