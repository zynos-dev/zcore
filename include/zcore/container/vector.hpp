/**************************************************************************/
/*  vector.hpp                                                            */
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
 * @file include/zcore/container/vector.hpp
 * @brief Allocator-backed growable contiguous value container.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/vector.hpp>
 * zcore::Vector<int> values(allocator);
 * @endcode
 */

#pragma once

#include <concepts>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <zcore/allocator.hpp>
#include <zcore/contract_violation.hpp>
#include <zcore/foundation.hpp>
#include <zcore/option.hpp>
#include <zcore/result.hpp>
#include <zcore/slice.hpp>
#include <zcore/status.hpp>
#include <zcore/type_constraints.hpp>

namespace zcore {

/**
 * @brief Allocator-backed contiguous growable container.
 *
 * @tparam ValueT Element type.
 *
 * `Vector<ValueT>` is move-only and uses explicit fallible APIs for growth.
 */
template <typename ValueT>
class [[nodiscard("Vector must be handled explicitly.")]] Vector final {
public:
    ZCORE_STATIC_REQUIRE(constraints::MutableValueObjectType<ValueT>,
                         "Vector<ValueT> requires mutable non-void object element type.");
    ZCORE_STATIC_REQUIRE(constraints::MoveConstructibleType<ValueT>, "Vector<ValueT> requires move-constructible element type.");
    ZCORE_STATIC_REQUIRE(constraints::NothrowMoveConstructibleType<ValueT>,
                         "Vector<ValueT> requires nothrow move construction for reallocation safety.");

    using ValueType = ValueT;
    using SizeType = usize;
    using Pointer = ValueT*;
    using ConstPointer = const ValueT*;
    using Iterator = ValueT*;
    using ConstIterator = const ValueT*;

    /// @brief Constructs an empty unbound vector.
    constexpr Vector() noexcept : Allocation_(Allocation::Empty()), Allocator_(nullptr), Size_(0U)
    {
    }

    /// @brief Constructs an empty vector bound to allocator.
    constexpr explicit Vector(Allocator& allocator) noexcept : Allocation_(Allocation::Empty()), Allocator_(&allocator), Size_(0U)
    {
    }

    Vector(const Vector&) = delete;
    Vector& operator=(const Vector&) = delete;

    constexpr Vector(Vector&& other) noexcept : Allocation_(other.Allocation_), Allocator_(other.Allocator_), Size_(other.Size_)
    {
        other.ClearState();
    }

    constexpr Vector& operator=(Vector&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        Reset();
        Allocation_ = other.Allocation_;
        Allocator_ = other.Allocator_;
        Size_ = other.Size_;
        other.ClearState();
        return *this;
    }

    ~Vector()
    {
        Reset();
    }

    /**
   * @brief Creates allocator-bound vector with reserved capacity.
   * @param allocator Allocator used for storage.
   * @param capacity Initial element capacity.
   */
    [[nodiscard]] static Result<Vector, Error> TryWithCapacity(Allocator& allocator, usize capacity) noexcept
    {
        Vector vector(allocator);
        const Status reserveStatus = vector.TryReserve(capacity);
        if (reserveStatus.HasError()) {
            return Result<Vector, Error>::Failure(reserveStatus.Error());
        }
        return Result<Vector, Error>::Success(std::move(vector));
    }

    /// @brief Returns whether allocator binding is present.
    [[nodiscard]] constexpr bool HasAllocator() const noexcept
    {
        return Allocator_ != nullptr;
    }

    /// @brief Returns bound allocator pointer or null when unbound.
    [[nodiscard]] constexpr Allocator* AllocatorRef() const noexcept
    {
        return Allocator_;
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

    /// @brief Returns current element capacity.
    [[nodiscard]] constexpr usize Capacity() const noexcept
    {
        if (Allocation_.IsEmpty()) {
            return 0U;
        }
        return Allocation_.size / sizeof(ValueT);
    }

    /// @brief Returns remaining capacity before growth is required.
    [[nodiscard]] constexpr usize RemainingCapacity() const noexcept
    {
        return Capacity() - Size_;
    }

    /// @brief Returns `true` when no elements are present.
    [[nodiscard]] constexpr bool Empty() const noexcept
    {
        return Size_ == 0U;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return Empty();
    }

    /// @brief Returns pointer to contiguous element storage, or `nullptr` when no storage is allocated.
    [[nodiscard]] constexpr Pointer Data() noexcept
    {
        return RawData();
    }

    /// @brief Returns pointer to contiguous element storage, or `nullptr` when no storage is allocated.
    [[nodiscard]] constexpr ConstPointer Data() const noexcept
    {
        return RawData();
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
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Vector::operator[] index out of bounds");
        return RawData()[index];
    }

    /**
   * @brief Checked indexed access.
   * @pre `index < Size()`.
   */
    [[nodiscard]] constexpr const ValueT& operator[](usize index) const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(index < Size_,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Vector::operator[] index out of bounds");
        return RawData()[index];
    }

    /// @brief Returns pointer to element at `index` or `nullptr` when out-of-range.
    [[nodiscard]] constexpr Pointer TryAt(usize index) noexcept
    {
        return index < Size_ ? RawData() + index : nullptr;
    }

    /// @brief Returns pointer to element at `index` or `nullptr` when out-of-range.
    [[nodiscard]] constexpr ConstPointer TryAt(usize index) const noexcept
    {
        return index < Size_ ? RawData() + index : nullptr;
    }

    /**
   * @brief Returns first element.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr ValueT& Front() noexcept
    {
        ZCORE_CONTRACT_REQUIRE(!Empty(), detail::ContractViolationCode::PRECONDITION, "zcore::Vector::Front() requires non-empty");
        return RawData()[0];
    }

    /**
   * @brief Returns first element.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr const ValueT& Front() const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(!Empty(), detail::ContractViolationCode::PRECONDITION, "zcore::Vector::Front() requires non-empty");
        return RawData()[0];
    }

    /**
   * @brief Returns last element.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr ValueT& Back() noexcept
    {
        ZCORE_CONTRACT_REQUIRE(!Empty(), detail::ContractViolationCode::PRECONDITION, "zcore::Vector::Back() requires non-empty");
        return RawData()[Size_ - 1U];
    }

    /**
   * @brief Returns last element.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr const ValueT& Back() const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(!Empty(), detail::ContractViolationCode::PRECONDITION, "zcore::Vector::Back() requires non-empty");
        return RawData()[Size_ - 1U];
    }

    /**
   * @brief Ensures at least `newCapacity` element capacity.
   * @return Success or allocator-domain error.
   */
    [[nodiscard]] Status TryReserve(usize newCapacity) noexcept
    {
        if (newCapacity <= Capacity()) {
            return OkStatus();
        }

        if (!HasAllocator()) {
            return ErrorStatus(
                    MakeAllocatorError(AllocatorErrorCode::UNSUPPORTED_OPERATION, "reserve", "vector has no bound allocator"));
        }

        if (newCapacity > MaxElementCount()) {
            return ErrorStatus(MakeAllocatorError(AllocatorErrorCode::INVALID_REQUEST,
                                                  "reserve",
                                                  "requested capacity exceeds max element count"));
        }

        auto allocationResult = Allocator_->Allocate(AllocationRequest{
                .size = newCapacity * sizeof(ValueT),
                .alignment = alignof(ValueT),
        });
        if (allocationResult.HasError()) {
            return ErrorStatus(allocationResult.Error());
        }

        const Allocation nextAllocation = allocationResult.Value();
        ZCORE_CONTRACT_REQUIRE(nextAllocation.IsValid(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Vector::TryReserve requires allocator to return valid allocation");
        ZCORE_CONTRACT_REQUIRE(!nextAllocation.IsEmpty(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Vector::TryReserve requires non-empty allocation for non-zero capacity");

        Pointer nextData = std::launder(reinterpret_cast<Pointer>(nextAllocation.data));
        Pointer currentData = RawData();

        for (usize index = 0; index < Size_; ++index) {
            std::construct_at(nextData + index, std::move(currentData[index]));
        }
        for (usize index = 0; index < Size_; ++index) {
            std::destroy_at(currentData + index);
        }

        if (!Allocation_.IsEmpty()) {
            const Status deallocateStatus = Allocator_->Deallocate(Allocation_);
            ZCORE_CONTRACT_REQUIRE(deallocateStatus.HasValue(),
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::Vector requires allocator deallocation success for owned allocations");
        }

        Allocation_ = nextAllocation;
        return OkStatus();
    }

    /**
   * @brief Attempts to append copied value.
   * @return Success or allocator-domain error.
   */
    [[nodiscard]] Status TryPushBack(const ValueT& value)
        requires(std::is_copy_constructible_v<ValueT>)
    {
        Status capacityStatus = EnsureCapacityFor(Size_ + 1U);
        if (capacityStatus.HasError()) {
            return capacityStatus;
        }
        std::construct_at(RawData() + Size_, value);
        ++Size_;
        return OkStatus();
    }

    /**
   * @brief Attempts to append moved value.
   * @return Success or allocator-domain error.
   */
    [[nodiscard]] Status TryPushBack(ValueT&& value)
        requires(std::is_move_constructible_v<ValueT>)
    {
        Status capacityStatus = EnsureCapacityFor(Size_ + 1U);
        if (capacityStatus.HasError()) {
            return capacityStatus;
        }
        std::construct_at(RawData() + Size_, std::move(value));
        ++Size_;
        return OkStatus();
    }

    /**
   * @brief Attempts in-place append.
   * @return Pointer to appended element or allocator-domain error.
   */
    template <typename... ArgsT>
        requires(std::is_constructible_v<ValueT, ArgsT && ...>)
    [[nodiscard]] Result<ValueT*, Error> TryEmplaceBack(ArgsT&&... args)
    {
        const Status capacityStatus = EnsureCapacityFor(Size_ + 1U);
        if (capacityStatus.HasError()) {
            return Result<ValueT*, Error>::Failure(capacityStatus.Error());
        }
        ValueT* const slot = RawData() + Size_;
        std::construct_at(slot, std::forward<ArgsT>(args)...);
        ++Size_;
        return Result<ValueT*, Error>::Success(slot);
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
        std::destroy_at(RawData() + Size_);
        return true;
    }

    /**
   * @brief Moves out last element if present.
   * @return `None` when empty.
   */
    [[nodiscard]] Option<ValueT> TryPopBackValue()
        requires(std::is_move_constructible_v<ValueT>)
    {
        if (Empty()) {
            return None;
        }
        --Size_;
        Pointer slot = RawData() + Size_;
        Option<ValueT> moved(std::move(*slot));
        std::destroy_at(slot);
        return moved;
    }

    /// @brief Destroys all stored elements; capacity is retained.
    constexpr void Clear() noexcept
    {
        while (Size_ > 0U) {
            --Size_;
            std::destroy_at(RawData() + Size_);
        }
    }

    /// @brief Destroys elements and releases owned allocation.
    void Reset() noexcept
    {
        Clear();
        if (!Allocation_.IsEmpty()) {
            ZCORE_CONTRACT_REQUIRE(Allocator_ != nullptr,
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::Vector requires allocator to release owned allocation");
            const Status deallocateStatus = Allocator_->Deallocate(Allocation_);
            ZCORE_CONTRACT_REQUIRE(deallocateStatus.HasValue(),
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::Vector requires allocator deallocation success for owned allocations");
        }
        Allocation_ = Allocation::Empty();
    }

    /// @brief Returns immutable contiguous view over stored elements.
    [[nodiscard]] constexpr Slice<const ValueT> AsSlice() const noexcept
    {
        if (Empty()) {
            return Slice<const ValueT>::Empty();
        }
        return Slice<const ValueT>::FromRawUnchecked(Data(), Size_);
    }

    /// @brief Returns mutable contiguous view over stored elements.
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

    [[nodiscard]] constexpr bool operator==(const Vector& other) const
        requires(std::equality_comparable<ValueT>)
    {
        if (Size_ != other.Size_) {
            return false;
        }
        for (usize index = 0; index < Size_; ++index) {
            if (!(RawData()[index] == other.RawData()[index])) {
                return false;
            }
        }
        return true;
    }

private:
    [[nodiscard]] static constexpr usize MaxElementCount() noexcept
    {
        return std::numeric_limits<usize>::max() / sizeof(ValueT);
    }

    [[nodiscard]] constexpr Pointer RawData() noexcept
    {
        return Allocation_.IsEmpty() ? nullptr : std::launder(reinterpret_cast<Pointer>(Allocation_.data));
    }

    [[nodiscard]] constexpr ConstPointer RawData() const noexcept
    {
        return Allocation_.IsEmpty() ? nullptr : std::launder(reinterpret_cast<ConstPointer>(Allocation_.data));
    }

    [[nodiscard]] Status EnsureCapacityFor(usize requiredSize) noexcept
    {
        if (requiredSize <= Capacity()) {
            return OkStatus();
        }

        if (requiredSize > MaxElementCount()) {
            return ErrorStatus(MakeAllocatorError(AllocatorErrorCode::INVALID_REQUEST,
                                                  "push_back",
                                                  "required capacity exceeds max element count"));
        }

        usize nextCapacity = Capacity() == 0U ? 1U : Capacity();
        while (nextCapacity < requiredSize) {
            if (nextCapacity > (MaxElementCount() / 2U)) {
                nextCapacity = requiredSize;
                break;
            }
            nextCapacity *= 2U;
        }
        if (nextCapacity < requiredSize) {
            nextCapacity = requiredSize;
        }
        return TryReserve(nextCapacity);
    }

    constexpr void ClearState() noexcept
    {
        Allocation_ = Allocation::Empty();
        Allocator_ = nullptr;
        Size_ = 0U;
    }

    Allocation Allocation_;
    Allocator* Allocator_;
    usize Size_;
};

} // namespace zcore
