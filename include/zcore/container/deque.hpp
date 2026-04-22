/**************************************************************************/
/*  deque.hpp                                                             */
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
 * @file include/zcore/container/deque.hpp
 * @brief Allocator-backed growable ring-buffer deque container.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/deque.hpp>
 * zcore::Deque<int> values(allocator);
 * values.TryPushBack(7);
 * values.TryPushFront(5);
 * @endcode
 */

#pragma once

#include <concepts>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <zcore/allocator.hpp>
#include <zcore/contract_violation.hpp>
#include <zcore/foundation.hpp>
#include <zcore/option.hpp>
#include <zcore/result.hpp>
#include <zcore/status.hpp>
#include <zcore/type_constraints.hpp>

namespace zcore {

/**
 * @brief Allocator-backed double-ended queue using ring-buffer storage.
 *
 * @tparam ValueT Element type.
 *
 * `Deque<ValueT>` is move-only and uses explicit fallible APIs for growth.
 */
template <typename ValueT>
class [[nodiscard("Deque must be handled explicitly.")]] Deque final {
public:
    ZCORE_STATIC_REQUIRE(constraints::MutableValueObjectType<ValueT>,
                         "Deque<ValueT> requires mutable non-void object element type.");
    ZCORE_STATIC_REQUIRE(constraints::MoveConstructibleType<ValueT>, "Deque<ValueT> requires move-constructible element type.");
    ZCORE_STATIC_REQUIRE(constraints::NothrowMoveConstructibleType<ValueT>,
                         "Deque<ValueT> requires nothrow move construction for reallocation safety.");

    using ValueType = ValueT;
    using SizeType = usize;
    using Pointer = ValueT*;
    using ConstPointer = const ValueT*;

    template <bool IsConstV>
    class BasicIterator final {
    public:
        using OwnerType = std::conditional_t<IsConstV, const Deque, Deque>;
        using DifferenceType = std::ptrdiff_t;
        using ValueType = ValueT;
        using Pointer = std::conditional_t<IsConstV, const ValueT*, ValueT*>;
        using Reference = std::conditional_t<IsConstV, const ValueT&, ValueT&>;
        using IteratorCategory = std::forward_iterator_tag;

        constexpr BasicIterator() noexcept : Owner_(nullptr), Offset_(0U)
        {
        }

        constexpr BasicIterator(OwnerType* owner, usize offset) noexcept : Owner_(owner), Offset_(offset)
        {
        }

        [[nodiscard]] constexpr Reference operator*() const noexcept
        {
            return (*Owner_)[Offset_];
        }

        [[nodiscard]] constexpr Pointer operator->() const noexcept
        {
            return std::addressof((*Owner_)[Offset_]);
        }

        constexpr BasicIterator& operator++() noexcept
        {
            ++Offset_;
            return *this;
        }

        constexpr BasicIterator operator++(int) noexcept
        {
            BasicIterator copy(*this);
            ++(*this);
            return copy;
        }

        [[nodiscard]] constexpr bool operator==(const BasicIterator& other) const noexcept = default;

    private:
        OwnerType* Owner_;
        usize Offset_;
    };

    using Iterator = BasicIterator<false>;
    using ConstIterator = BasicIterator<true>;

    /// @brief Constructs an empty unbound deque.
    constexpr Deque() noexcept : Allocation_(Allocation::Empty()), Allocator_(nullptr), Head_(0U), Size_(0U)
    {
    }

    /// @brief Constructs an empty deque bound to allocator.
    constexpr explicit Deque(Allocator& allocator) noexcept
            : Allocation_(Allocation::Empty()), Allocator_(&allocator), Head_(0U), Size_(0U)
    {
    }

    Deque(const Deque&) = delete;
    Deque& operator=(const Deque&) = delete;

    constexpr Deque(Deque&& other) noexcept
            : Allocation_(other.Allocation_), Allocator_(other.Allocator_), Head_(other.Head_), Size_(other.Size_)
    {
        other.ClearState();
    }

    constexpr Deque& operator=(Deque&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        Reset();
        Allocation_ = other.Allocation_;
        Allocator_ = other.Allocator_;
        Head_ = other.Head_;
        Size_ = other.Size_;
        other.ClearState();
        return *this;
    }

    ~Deque()
    {
        Reset();
    }

    /**
   * @brief Creates allocator-bound deque with reserved capacity.
   * @param allocator Allocator used for storage.
   * @param capacity Initial element capacity.
   */
    [[nodiscard]] static Result<Deque, Error> TryWithCapacity(Allocator& allocator, usize capacity) noexcept
    {
        Deque deque(allocator);
        const Status reserveStatus = deque.TryReserve(capacity);
        if (reserveStatus.HasError()) {
            return Result<Deque, Error>::Failure(reserveStatus.Error());
        }
        return Result<Deque, Error>::Success(std::move(deque));
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

    /// @brief Returns iterator to first element.
    [[nodiscard]] constexpr Iterator begin() noexcept
    {
        return Iterator(this, 0U);
    }

    /// @brief Returns iterator past last element.
    [[nodiscard]] constexpr Iterator end() noexcept
    {
        return Iterator(this, Size_);
    }

    /// @brief Returns const iterator to first element.
    [[nodiscard]] constexpr ConstIterator begin() const noexcept
    {
        return ConstIterator(this, 0U);
    }

    /// @brief Returns const iterator past last element.
    [[nodiscard]] constexpr ConstIterator end() const noexcept
    {
        return ConstIterator(this, Size_);
    }

    [[nodiscard]] constexpr ConstIterator cbegin() const noexcept
    {
        return begin();
    }

    [[nodiscard]] constexpr ConstIterator cend() const noexcept
    {
        return end();
    }

    /// @brief Returns pointer to element at `index` or `nullptr` when out-of-range.
    [[nodiscard]] constexpr Pointer TryAt(usize index) noexcept
    {
        return index < Size_ ? PtrAtLogical(index) : nullptr;
    }

    /// @brief Returns pointer to element at `index` or `nullptr` when out-of-range.
    [[nodiscard]] constexpr ConstPointer TryAt(usize index) const noexcept
    {
        return index < Size_ ? PtrAtLogical(index) : nullptr;
    }

    /**
   * @brief Checked indexed access.
   * @pre `index < Size()`.
   */
    [[nodiscard]] constexpr ValueT& operator[](usize index) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(index < Size_,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Deque::operator[] index out of bounds");
        return *PtrAtLogical(index);
    }

    /**
   * @brief Checked indexed access.
   * @pre `index < Size()`.
   */
    [[nodiscard]] constexpr const ValueT& operator[](usize index) const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(index < Size_,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Deque::operator[] index out of bounds");
        return *PtrAtLogical(index);
    }

    /**
   * @brief Returns first element.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr ValueT& Front() noexcept
    {
        ZCORE_CONTRACT_REQUIRE(!Empty(), detail::ContractViolationCode::PRECONDITION, "zcore::Deque::Front() requires non-empty");
        return *PtrAtPhysical(Head_);
    }

    /**
   * @brief Returns first element.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr const ValueT& Front() const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(!Empty(), detail::ContractViolationCode::PRECONDITION, "zcore::Deque::Front() requires non-empty");
        return *PtrAtPhysical(Head_);
    }

    /**
   * @brief Returns last element.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr ValueT& Back() noexcept
    {
        ZCORE_CONTRACT_REQUIRE(!Empty(), detail::ContractViolationCode::PRECONDITION, "zcore::Deque::Back() requires non-empty");
        return *PtrAtLogical(Size_ - 1U);
    }

    /**
   * @brief Returns last element.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr const ValueT& Back() const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(!Empty(), detail::ContractViolationCode::PRECONDITION, "zcore::Deque::Back() requires non-empty");
        return *PtrAtLogical(Size_ - 1U);
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
                    MakeAllocatorError(AllocatorErrorCode::UNSUPPORTED_OPERATION, "reserve", "deque has no bound allocator"));
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
                               "zcore::Deque::TryReserve requires allocator to return valid allocation");
        ZCORE_CONTRACT_REQUIRE(!nextAllocation.IsEmpty(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Deque::TryReserve requires non-empty allocation for non-zero capacity");

        const usize previousSize = Size_;
        Pointer nextData = std::launder(reinterpret_cast<Pointer>(nextAllocation.data));
        for (usize index = 0U; index < previousSize; ++index) {
            std::construct_at(nextData + index, std::move(*PtrAtLogical(index)));
        }
        for (usize index = 0U; index < previousSize; ++index) {
            std::destroy_at(PtrAtLogical(index));
        }

        if (!Allocation_.IsEmpty()) {
            const Status deallocateStatus = Allocator_->Deallocate(Allocation_);
            ZCORE_CONTRACT_REQUIRE(deallocateStatus.HasValue(),
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::Deque requires allocator deallocation success for owned allocations");
        }

        Allocation_ = nextAllocation;
        Head_ = 0U;
        Size_ = previousSize;
        return OkStatus();
    }

    /**
   * @brief Attempts to append copied value.
   * @return Success or allocator-domain error.
   */
    [[nodiscard]] Status TryPushBack(const ValueT& value)
        requires(std::is_copy_constructible_v<ValueT>)
    {
        Status capacityStatus = EnsureCapacityFor(Size_ + 1U, "push_back");
        if (capacityStatus.HasError()) {
            return capacityStatus;
        }
        const usize insertPhysical = PhysicalIndexForLogical(Size_);
        std::construct_at(PtrAtPhysical(insertPhysical), value);
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
        Status capacityStatus = EnsureCapacityFor(Size_ + 1U, "push_back");
        if (capacityStatus.HasError()) {
            return capacityStatus;
        }
        const usize insertPhysical = PhysicalIndexForLogical(Size_);
        std::construct_at(PtrAtPhysical(insertPhysical), std::move(value));
        ++Size_;
        return OkStatus();
    }

    /**
   * @brief Attempts to prepend copied value.
   * @return Success or allocator-domain error.
   */
    [[nodiscard]] Status TryPushFront(const ValueT& value)
        requires(std::is_copy_constructible_v<ValueT>)
    {
        Status capacityStatus = EnsureCapacityFor(Size_ + 1U, "push_front");
        if (capacityStatus.HasError()) {
            return capacityStatus;
        }
        Head_ = DecrementWrapped(Head_);
        std::construct_at(PtrAtPhysical(Head_), value);
        ++Size_;
        return OkStatus();
    }

    /**
   * @brief Attempts to prepend moved value.
   * @return Success or allocator-domain error.
   */
    [[nodiscard]] Status TryPushFront(ValueT&& value)
        requires(std::is_move_constructible_v<ValueT>)
    {
        Status capacityStatus = EnsureCapacityFor(Size_ + 1U, "push_front");
        if (capacityStatus.HasError()) {
            return capacityStatus;
        }
        Head_ = DecrementWrapped(Head_);
        std::construct_at(PtrAtPhysical(Head_), std::move(value));
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
        const Status capacityStatus = EnsureCapacityFor(Size_ + 1U, "emplace_back");
        if (capacityStatus.HasError()) {
            return Result<ValueT*, Error>::Failure(capacityStatus.Error());
        }
        const usize insertPhysical = PhysicalIndexForLogical(Size_);
        ValueT* const slot = PtrAtPhysical(insertPhysical);
        std::construct_at(slot, std::forward<ArgsT>(args)...);
        ++Size_;
        return Result<ValueT*, Error>::Success(slot);
    }

    /**
   * @brief Attempts in-place prepend.
   * @return Pointer to prepended element or allocator-domain error.
   */
    template <typename... ArgsT>
        requires(std::is_constructible_v<ValueT, ArgsT && ...>)
    [[nodiscard]] Result<ValueT*, Error> TryEmplaceFront(ArgsT&&... args)
    {
        const Status capacityStatus = EnsureCapacityFor(Size_ + 1U, "emplace_front");
        if (capacityStatus.HasError()) {
            return Result<ValueT*, Error>::Failure(capacityStatus.Error());
        }
        Head_ = DecrementWrapped(Head_);
        ValueT* const slot = PtrAtPhysical(Head_);
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
        const usize tailPhysical = PhysicalIndexForLogical(Size_ - 1U);
        std::destroy_at(PtrAtPhysical(tailPhysical));
        --Size_;
        if (Size_ == 0U) {
            Head_ = 0U;
        }
        return true;
    }

    /**
   * @brief Attempts to remove first element.
   * @return `true` on success, `false` when empty.
   */
    [[nodiscard]] constexpr bool TryPopFront() noexcept
    {
        if (Empty()) {
            return false;
        }
        std::destroy_at(PtrAtPhysical(Head_));
        --Size_;
        if (Size_ == 0U) {
            Head_ = 0U;
        }
        else {
            Head_ = IncrementWrapped(Head_);
        }
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
        const usize tailPhysical = PhysicalIndexForLogical(Size_ - 1U);
        ValueT* const slot = PtrAtPhysical(tailPhysical);
        Option<ValueT> moved(std::move(*slot));
        std::destroy_at(slot);
        --Size_;
        if (Size_ == 0U) {
            Head_ = 0U;
        }
        return moved;
    }

    /**
   * @brief Moves out first element if present.
   * @return `None` when empty.
   */
    [[nodiscard]] Option<ValueT> TryPopFrontValue()
        requires(std::is_move_constructible_v<ValueT>)
    {
        if (Empty()) {
            return None;
        }
        ValueT* const slot = PtrAtPhysical(Head_);
        Option<ValueT> moved(std::move(*slot));
        std::destroy_at(slot);
        --Size_;
        if (Size_ == 0U) {
            Head_ = 0U;
        }
        else {
            Head_ = IncrementWrapped(Head_);
        }
        return moved;
    }

    /// @brief Destroys all stored elements; capacity is retained.
    constexpr void Clear() noexcept
    {
        DestroyAllElements();
        Head_ = 0U;
    }

    /// @brief Destroys elements and releases owned allocation.
    void Reset() noexcept
    {
        Clear();
        if (!Allocation_.IsEmpty()) {
            ZCORE_CONTRACT_REQUIRE(Allocator_ != nullptr,
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::Deque requires allocator to release owned allocation");
            const Status deallocateStatus = Allocator_->Deallocate(Allocation_);
            ZCORE_CONTRACT_REQUIRE(deallocateStatus.HasValue(),
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::Deque requires allocator deallocation success for owned allocations");
        }
        Allocation_ = Allocation::Empty();
    }

    [[nodiscard]] constexpr bool operator==(const Deque& other) const
        requires(std::equality_comparable<ValueT>)
    {
        if (Size_ != other.Size_) {
            return false;
        }
        for (usize index = 0U; index < Size_; ++index) {
            if (!((*this)[index] == other[index])) {
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

    [[nodiscard]] constexpr Pointer PtrAtPhysical(usize physicalIndex) noexcept
    {
        return RawData() + physicalIndex;
    }

    [[nodiscard]] constexpr ConstPointer PtrAtPhysical(usize physicalIndex) const noexcept
    {
        return RawData() + physicalIndex;
    }

    [[nodiscard]] constexpr usize PhysicalIndexForLogical(usize logicalIndex) const noexcept
    {
        const usize capacity = Capacity();
        ZCORE_CONTRACT_REQUIRE(capacity > 0U,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Deque requires non-zero capacity for physical indexing");
        usize physicalIndex = Head_ + logicalIndex;
        if (physicalIndex >= capacity) {
            physicalIndex -= capacity;
        }
        return physicalIndex;
    }

    [[nodiscard]] constexpr Pointer PtrAtLogical(usize logicalIndex) noexcept
    {
        return PtrAtPhysical(PhysicalIndexForLogical(logicalIndex));
    }

    [[nodiscard]] constexpr ConstPointer PtrAtLogical(usize logicalIndex) const noexcept
    {
        return PtrAtPhysical(PhysicalIndexForLogical(logicalIndex));
    }

    [[nodiscard]] constexpr usize IncrementWrapped(usize index) const noexcept
    {
        const usize capacity = Capacity();
        ZCORE_CONTRACT_REQUIRE(capacity > 0U,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Deque requires non-zero capacity for wrap increment");
        return (index + 1U) == capacity ? 0U : (index + 1U);
    }

    [[nodiscard]] constexpr usize DecrementWrapped(usize index) const noexcept
    {
        const usize capacity = Capacity();
        ZCORE_CONTRACT_REQUIRE(capacity > 0U,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Deque requires non-zero capacity for wrap decrement");
        return index == 0U ? (capacity - 1U) : (index - 1U);
    }

    [[nodiscard]] Status EnsureCapacityFor(usize requiredSize, const char* operation) noexcept
    {
        if (requiredSize <= Capacity()) {
            return OkStatus();
        }

        if (requiredSize > MaxElementCount()) {
            return ErrorStatus(MakeAllocatorError(AllocatorErrorCode::INVALID_REQUEST,
                                                  operation,
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

    constexpr void DestroyAllElements() noexcept
    {
        for (usize index = 0U; index < Size_; ++index) {
            std::destroy_at(PtrAtLogical(index));
        }
        Size_ = 0U;
    }

    constexpr void ClearState() noexcept
    {
        Allocation_ = Allocation::Empty();
        Allocator_ = nullptr;
        Head_ = 0U;
        Size_ = 0U;
    }

    Allocation Allocation_;
    Allocator* Allocator_;
    usize Head_;
    usize Size_;
};

} // namespace zcore
