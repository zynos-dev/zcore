/**************************************************************************/
/*  hash_map.hpp                                                          */
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
 * @file include/zcore/container/hash_map.hpp
 * @brief Allocator-backed hash map with deterministic open addressing.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/hash_map.hpp>
 * zcore::HashMap<int, int> values(allocator);
 * values.TryInsert(7, 11);
 * @endcode
 */

#pragma once

#include <concepts>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <zcore/allocator.hpp>
#include <zcore/contract_violation.hpp>
#include <zcore/foundation.hpp>
#include <zcore/hash/customization.hpp>
#include <zcore/option.hpp>
#include <zcore/result.hpp>
#include <zcore/status.hpp>
#include <zcore/type_constraints.hpp>

namespace zcore {

/**
 * @brief Allocator-backed hash map with open-addressing + linear probing.
 *
 * @tparam KeyT Key type.
 * @tparam ValueT Mapped value type.
 *
 * `HashMap<KeyT, ValueT>` is move-only and uses explicit fallible APIs for
 * growth and insertion.
 */
template <typename KeyT, typename ValueT>
class [[nodiscard("HashMap must be handled explicitly.")]] HashMap final {
public:
    ZCORE_STATIC_REQUIRE(constraints::MutableValueObjectType<KeyT>,
                         "HashMap<KeyT, ValueT> requires mutable non-void object key type.");
    ZCORE_STATIC_REQUIRE(constraints::MutableValueObjectType<ValueT>,
                         "HashMap<KeyT, ValueT> requires mutable non-void object value type.");
    ZCORE_STATIC_REQUIRE(constraints::MoveConstructibleType<KeyT>, "HashMap<KeyT, ValueT> requires move-constructible key type.");
    ZCORE_STATIC_REQUIRE(constraints::MoveConstructibleType<ValueT>,
                         "HashMap<KeyT, ValueT> requires move-constructible value type.");
    ZCORE_STATIC_REQUIRE(constraints::NothrowMoveConstructibleType<KeyT>,
                         "HashMap<KeyT, ValueT> requires nothrow move-constructible key type.");
    ZCORE_STATIC_REQUIRE(constraints::NothrowMoveConstructibleType<ValueT>,
                         "HashMap<KeyT, ValueT> requires nothrow move-constructible value type.");
    ZCORE_STATIC_REQUIRE(std::equality_comparable<KeyT>, "HashMap<KeyT, ValueT> requires equality-comparable key type.");
    ZCORE_STATIC_REQUIRE(hash::Hashable<KeyT>, "HashMap<KeyT, ValueT> requires zcore::hash::Hashable key type.");

    using KeyType = KeyT;
    using MappedType = ValueT;
    using SizeType = usize;

    /// @brief Constructs an empty unbound map.
    constexpr HashMap() noexcept
            : StateAllocation_(Allocation::Empty())
            , EntryAllocation_(Allocation::Empty())
            , States_(nullptr)
            , Entries_(nullptr)
            , Allocator_(nullptr)
            , Capacity_(0U)
            , Size_(0U)
            , Tombstones_(0U)
    {
    }

    /// @brief Constructs an empty map bound to allocator.
    constexpr explicit HashMap(Allocator& allocator) noexcept
            : StateAllocation_(Allocation::Empty())
            , EntryAllocation_(Allocation::Empty())
            , States_(nullptr)
            , Entries_(nullptr)
            , Allocator_(&allocator)
            , Capacity_(0U)
            , Size_(0U)
            , Tombstones_(0U)
    {
    }

    HashMap(const HashMap&) = delete;
    HashMap& operator=(const HashMap&) = delete;

    constexpr HashMap(HashMap&& other) noexcept
            : StateAllocation_(other.StateAllocation_)
            , EntryAllocation_(other.EntryAllocation_)
            , States_(other.States_)
            , Entries_(other.Entries_)
            , Allocator_(other.Allocator_)
            , Capacity_(other.Capacity_)
            , Size_(other.Size_)
            , Tombstones_(other.Tombstones_)
    {
        other.ClearState();
    }

    constexpr HashMap& operator=(HashMap&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        Reset();
        StateAllocation_ = other.StateAllocation_;
        EntryAllocation_ = other.EntryAllocation_;
        States_ = other.States_;
        Entries_ = other.Entries_;
        Allocator_ = other.Allocator_;
        Capacity_ = other.Capacity_;
        Size_ = other.Size_;
        Tombstones_ = other.Tombstones_;
        other.ClearState();
        return *this;
    }

    ~HashMap()
    {
        Reset();
    }

    /**
   * @brief Creates allocator-bound map with reserved entry capacity.
   * @param allocator Allocator used for storage.
   * @param entryCapacity Minimum entry capacity under default load factor.
   */
    [[nodiscard]] static Result<HashMap, Error> TryWithCapacity(Allocator& allocator, usize entryCapacity) noexcept
    {
        HashMap map(allocator);
        Status reserveStatus = map.TryReserve(entryCapacity);
        if (reserveStatus.HasError()) {
            return Result<HashMap, Error>::Failure(reserveStatus.Error());
        }
        return Result<HashMap, Error>::Success(std::move(map));
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

    /// @brief Returns current entry count.
    [[nodiscard]] constexpr usize Size() const noexcept
    {
        return Size_;
    }

    [[nodiscard]] constexpr usize size() const noexcept
    {
        return Size();
    }

    /// @brief Returns bucket capacity.
    [[nodiscard]] constexpr usize Capacity() const noexcept
    {
        return Capacity_;
    }

    /// @brief Returns remaining entries before growth at target load factor.
    [[nodiscard]] constexpr usize RemainingCapacity() const noexcept
    {
        const usize threshold = LoadThresholdForCapacity(Capacity_);
        return threshold > Size_ ? (threshold - Size_) : 0U;
    }

    /// @brief Returns `true` when no entries are stored.
    [[nodiscard]] constexpr bool Empty() const noexcept
    {
        return Size_ == 0U;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return Empty();
    }

    /// @brief Returns `true` when key exists.
    [[nodiscard]] bool Contains(const KeyT& key) const noexcept
    {
        return FindExistingIndex(key) != kInvalidIndex;
    }

    /// @brief Returns pointer to mapped value or `nullptr` when missing.
    [[nodiscard]] ValueT* TryGet(const KeyT& key) noexcept
    {
        const usize index = FindExistingIndex(key);
        return index == kInvalidIndex ? nullptr : std::addressof(Entries_[index].value);
    }

    /// @brief Returns pointer to mapped value or `nullptr` when missing.
    [[nodiscard]] const ValueT* TryGet(const KeyT& key) const noexcept
    {
        const usize index = FindExistingIndex(key);
        return index == kInvalidIndex ? nullptr : std::addressof(Entries_[index].value);
    }

    /**
   * @brief Returns mapped value reference for existing key.
   * @pre `Contains(key)`.
   */
    [[nodiscard]] ValueT& Get(const KeyT& key) noexcept
    {
        const usize index = FindExistingIndex(key);
        ZCORE_CONTRACT_REQUIRE(index != kInvalidIndex,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::HashMap::Get() key not found");
        return Entries_[index].value;
    }

    /**
   * @brief Returns mapped value reference for existing key.
   * @pre `Contains(key)`.
   */
    [[nodiscard]] const ValueT& Get(const KeyT& key) const noexcept
    {
        const usize index = FindExistingIndex(key);
        ZCORE_CONTRACT_REQUIRE(index != kInvalidIndex,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::HashMap::Get() key not found");
        return Entries_[index].value;
    }

    /**
   * @brief Ensures capacity for at least `entryCapacity` entries.
   * @return Success or allocator-domain error.
   */
    [[nodiscard]] Status TryReserve(usize entryCapacity) noexcept
    {
        if (entryCapacity <= RemainingCapacity()) {
            return OkStatus();
        }

        const usize requiredBuckets = BucketsForEntries(entryCapacity);
        if (requiredBuckets <= Capacity_) {
            return OkStatus();
        }
        return Rehash(requiredBuckets);
    }

    /**
   * @brief Attempts to insert copied key/value pair.
   * @return `Result<bool, Error>` where `true` = inserted, `false` = key already present.
   */
    [[nodiscard]] Result<bool, Error> TryInsert(const KeyT& key, const ValueT& value)
        requires(std::is_copy_constructible_v<KeyT> && std::is_copy_constructible_v<ValueT>)
    {
        Status capacityStatus = EnsureCapacityForInsert();
        if (capacityStatus.HasError()) {
            return Result<bool, Error>::Failure(capacityStatus.Error());
        }
        RequireStorageInitialized("zcore::HashMap::TryInsert() requires initialized storage");

        const InsertProbeResult probe = FindInsertIndex(key);
        if (probe.foundExisting) {
            return Result<bool, Error>::Success(false);
        }
        ZCORE_CONTRACT_REQUIRE(probe.index != kInvalidIndex,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::HashMap::TryInsert() failed to find insertion slot");
        ConstructEntryAt(probe.index, key, value);
        return Result<bool, Error>::Success(true);
    }

    /**
   * @brief Attempts to insert moved key/value pair.
   * @return `Result<bool, Error>` where `true` = inserted, `false` = key already present.
   */
    [[nodiscard]] Result<bool, Error> TryInsert(KeyT&& key, ValueT&& value)
        requires(std::is_move_constructible_v<KeyT> && std::is_move_constructible_v<ValueT>)
    {
        Status capacityStatus = EnsureCapacityForInsert();
        if (capacityStatus.HasError()) {
            return Result<bool, Error>::Failure(capacityStatus.Error());
        }
        RequireStorageInitialized("zcore::HashMap::TryInsert() requires initialized storage");

        const InsertProbeResult probe = FindInsertIndex(key);
        if (probe.foundExisting) {
            return Result<bool, Error>::Success(false);
        }
        ZCORE_CONTRACT_REQUIRE(probe.index != kInvalidIndex,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::HashMap::TryInsert() failed to find insertion slot");
        ConstructEntryAt(probe.index, std::move(key), std::move(value));
        return Result<bool, Error>::Success(true);
    }

    /**
   * @brief Attempts to insert or assign copied mapped value.
   * @return Success or allocator-domain error.
   */
    [[nodiscard]] Status TryInsertOrAssign(const KeyT& key, const ValueT& value)
        requires(std::is_copy_constructible_v<KeyT> && std::is_copy_constructible_v<ValueT> && std::is_copy_assignable_v<ValueT>)
    {
        Status capacityStatus = EnsureCapacityForInsert();
        if (capacityStatus.HasError()) {
            return capacityStatus;
        }
        RequireStorageInitialized("zcore::HashMap::TryInsertOrAssign() requires initialized storage");

        const InsertProbeResult probe = FindInsertIndex(key);
        ZCORE_CONTRACT_REQUIRE(probe.index != kInvalidIndex,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::HashMap::TryInsertOrAssign() failed to find slot");
        if (probe.foundExisting) {
            Entries_[probe.index].value = value;
            return OkStatus();
        }
        ConstructEntryAt(probe.index, key, value);
        return OkStatus();
    }

    /**
   * @brief Attempts to remove key.
   * @return `true` on success, `false` when key is missing.
   */
    [[nodiscard]] bool TryRemove(const KeyT& key) noexcept
    {
        const usize index = FindExistingIndex(key);
        if (index == kInvalidIndex) {
            return false;
        }
        DestroyEntryAt(index);
        return true;
    }

    /**
   * @brief Attempts to remove key and move out mapped value.
   * @return `None` when key is missing.
   */
    [[nodiscard]] Option<ValueT> TryRemoveValue(const KeyT& key)
        requires(std::is_move_constructible_v<ValueT>)
    {
        const usize index = FindExistingIndex(key);
        if (index == kInvalidIndex) {
            return None;
        }
        Option<ValueT> moved(std::move(Entries_[index].value));
        DestroyEntryAt(index);
        return moved;
    }

    /// @brief Clears entries while retaining allocated buckets.
    void Clear() noexcept
    {
        DestroyAllEntries();
        if (Capacity_ > 0U) {
            InitializeStates(States_, Capacity_);
        }
        Tombstones_ = 0U;
    }

    /// @brief Clears entries and deallocates owned bucket storage.
    void Reset() noexcept
    {
        Clear();
        ReleaseStorage();
    }

private:
    struct Entry final {
        KeyT key;
        ValueT value;
    };

    enum class SlotState : u8 {
        EMPTY = 0,
        OCCUPIED = 1,
        TOMBSTONE = 2,
    };

    struct InsertProbeResult final {
        usize index;
        bool foundExisting;
    };

    static constexpr usize kMinBucketCount = 8U;
    static constexpr usize kMaxLoadFactorPercent = 70U;
    static constexpr usize kLoadFactorScale = 100U;
    static constexpr usize kInvalidIndex = std::numeric_limits<usize>::max();

    [[nodiscard]] static constexpr usize MaxBucketCount() noexcept
    {
        return std::numeric_limits<usize>::max() / sizeof(Entry);
    }

    [[nodiscard]] static constexpr usize HighestPowerOfTwoAtMost(usize value) noexcept
    {
        if (value == 0U) {
            return 0U;
        }
        usize out = 1U;
        while (out <= (value / 2U)) {
            out *= 2U;
        }
        return out;
    }

    [[nodiscard]] static constexpr usize MaxPowerOfTwoBucketCount() noexcept
    {
        return HighestPowerOfTwoAtMost(MaxBucketCount());
    }

    [[nodiscard]] static constexpr usize LoadThresholdForCapacity(usize bucketCount) noexcept
    {
        return (bucketCount * kMaxLoadFactorPercent) / kLoadFactorScale;
    }

    [[nodiscard]] static constexpr usize NextPowerOfTwo(usize value) noexcept
    {
        if (value <= 1U) {
            return 1U;
        }
        usize out = 1U;
        while (out < value) {
            if (out > (std::numeric_limits<usize>::max() / 2U)) {
                return std::numeric_limits<usize>::max();
            }
            out *= 2U;
        }
        return out;
    }

    [[nodiscard]] static constexpr usize BucketsForEntries(usize entryCapacity) noexcept
    {
        if (entryCapacity == 0U) {
            return 0U;
        }
        const usize scaled = (entryCapacity * kLoadFactorScale) + (kMaxLoadFactorPercent - 1U);
        const usize minBuckets = scaled / kMaxLoadFactorPercent;
        usize bucketCount = NextPowerOfTwo(minBuckets < kMinBucketCount ? kMinBucketCount : minBuckets);
        if (bucketCount > MaxPowerOfTwoBucketCount()) {
            bucketCount = MaxPowerOfTwoBucketCount();
        }
        return bucketCount;
    }

    [[nodiscard]] static constexpr usize AdvanceIndex(usize index, usize mask) noexcept
    {
        return (index + 1U) & mask;
    }

    [[nodiscard]] usize HashIndexForKey(const KeyT& key, usize capacity) const noexcept
    {
        const u64 digest = hash::HashObject(key, 0ULL);
        return static_cast<usize>(digest) & (capacity - 1U);
    }

    static void InitializeStates(SlotState* states, usize count) noexcept
    {
        for (usize index = 0U; index < count; ++index) {
            states[index] = SlotState::EMPTY;
        }
    }

    [[nodiscard]] Status EnsureCapacityForInsert() noexcept
    {
        if (Capacity_ == 0U) {
            return Rehash(kMinBucketCount);
        }

        const usize usedSlots = Size_ + Tombstones_;
        const usize threshold = LoadThresholdForCapacity(Capacity_);
        if ((usedSlots + 1U) <= threshold) {
            return OkStatus();
        }

        usize nextCapacity = Capacity_ * 2U;
        if (nextCapacity < kMinBucketCount) {
            nextCapacity = kMinBucketCount;
        }
        if (nextCapacity > MaxPowerOfTwoBucketCount()) {
            nextCapacity = MaxPowerOfTwoBucketCount();
        }
        if (nextCapacity <= Capacity_) {
            return ErrorStatus(
                    MakeAllocatorError(AllocatorErrorCode::INVALID_REQUEST, "insert", "hash map capacity limit reached"));
        }
        return Rehash(nextCapacity);
    }

    [[nodiscard]] Status Rehash(usize newCapacity) noexcept
    {
        if (newCapacity == 0U) {
            return OkStatus();
        }
        if (!HasAllocator()) {
            return ErrorStatus(
                    MakeAllocatorError(AllocatorErrorCode::UNSUPPORTED_OPERATION, "reserve", "hash map has no bound allocator"));
        }
        if (newCapacity > MaxPowerOfTwoBucketCount()) {
            return ErrorStatus(MakeAllocatorError(AllocatorErrorCode::INVALID_REQUEST,
                                                  "reserve",
                                                  "requested capacity exceeds max bucket count"));
        }
        ZCORE_CONTRACT_REQUIRE((newCapacity & (newCapacity - 1U)) == 0U,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::HashMap::Rehash() requires power-of-two capacity");

        auto stateAllocationResult = Allocator_->Allocate(AllocationRequest{
                .size = newCapacity * sizeof(SlotState),
                .alignment = alignof(SlotState),
        });
        if (stateAllocationResult.HasError()) {
            return ErrorStatus(stateAllocationResult.Error());
        }

        const Allocation nextStateAllocation = stateAllocationResult.Value();
        ZCORE_CONTRACT_REQUIRE(nextStateAllocation.IsValid(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::HashMap::Rehash() requires valid state allocation");
        ZCORE_CONTRACT_REQUIRE(!nextStateAllocation.IsEmpty(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::HashMap::Rehash() requires non-empty state allocation");

        auto entryAllocationResult = Allocator_->Allocate(AllocationRequest{
                .size = newCapacity * sizeof(Entry),
                .alignment = alignof(Entry),
        });
        if (entryAllocationResult.HasError()) {
            const Status releaseStatus = Allocator_->Deallocate(nextStateAllocation);
            ZCORE_CONTRACT_REQUIRE(releaseStatus.HasValue(),
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::HashMap::Rehash() failed to release state allocation after entry allocation failure");
            return ErrorStatus(entryAllocationResult.Error());
        }

        const Allocation nextEntryAllocation = entryAllocationResult.Value();
        ZCORE_CONTRACT_REQUIRE(nextEntryAllocation.IsValid(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::HashMap::Rehash() requires valid entry allocation");
        ZCORE_CONTRACT_REQUIRE(!nextEntryAllocation.IsEmpty(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::HashMap::Rehash() requires non-empty entry allocation");

        SlotState* const nextStates = std::launder(reinterpret_cast<SlotState*>(nextStateAllocation.data));
        Entry* const nextEntries = std::launder(reinterpret_cast<Entry*>(nextEntryAllocation.data));
        InitializeStates(nextStates, newCapacity);

        const usize oldCapacity = Capacity_;
        SlotState* const oldStates = States_;
        Entry* const oldEntries = Entries_;
        const Allocation oldStateAllocation = StateAllocation_;
        const Allocation oldEntryAllocation = EntryAllocation_;
        const usize oldSize = Size_;

        for (usize index = 0U; index < oldCapacity; ++index) {
            if (oldStates[index] != SlotState::OCCUPIED) {
                continue;
            }

            const KeyT& key = oldEntries[index].key;
            const usize insertIndex = FindInsertIndexInStorage(nextStates, nextEntries, newCapacity, key).index;
            ZCORE_CONTRACT_REQUIRE(insertIndex != kInvalidIndex,
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::HashMap::Rehash() failed to find destination slot");
            std::construct_at(std::addressof(nextEntries[insertIndex]),
                              std::move(oldEntries[index].key),
                              std::move(oldEntries[index].value));
            nextStates[insertIndex] = SlotState::OCCUPIED;
            std::destroy_at(std::addressof(oldEntries[index]));
        }

        StateAllocation_ = nextStateAllocation;
        EntryAllocation_ = nextEntryAllocation;
        States_ = nextStates;
        Entries_ = nextEntries;
        Capacity_ = newCapacity;
        Size_ = oldSize;
        Tombstones_ = 0U;

        if (!oldStateAllocation.IsEmpty()) {
            const Status releaseStateStatus = Allocator_->Deallocate(oldStateAllocation);
            ZCORE_CONTRACT_REQUIRE(releaseStateStatus.HasValue(),
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::HashMap::Rehash() failed to release old state allocation");
        }
        if (!oldEntryAllocation.IsEmpty()) {
            const Status releaseEntryStatus = Allocator_->Deallocate(oldEntryAllocation);
            ZCORE_CONTRACT_REQUIRE(releaseEntryStatus.HasValue(),
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::HashMap::Rehash() failed to release old entry allocation");
        }

        return OkStatus();
    }

    [[nodiscard]] InsertProbeResult FindInsertIndex(const KeyT& key) const noexcept
    {
        return FindInsertIndexInStorage(States_, Entries_, Capacity_, key);
    }

    [[nodiscard]] InsertProbeResult
    FindInsertIndexInStorage(const SlotState* states, const Entry* entries, usize capacity, const KeyT& key) const noexcept
    {
        if (capacity == 0U) {
            return InsertProbeResult{.index = kInvalidIndex, .foundExisting = false};
        }

        const usize mask = capacity - 1U;
        usize slot = HashIndexForKey(key, capacity);
        usize firstTombstone = kInvalidIndex;

        for (usize probe = 0U; probe < capacity; ++probe) {
            const SlotState state = states[slot];
            if (state == SlotState::EMPTY) {
                return InsertProbeResult{
                        .index = firstTombstone == kInvalidIndex ? slot : firstTombstone,
                        .foundExisting = false,
                };
            }
            if (state == SlotState::TOMBSTONE) {
                if (firstTombstone == kInvalidIndex) {
                    firstTombstone = slot;
                }
            }
            else if (entries[slot].key == key) {
                return InsertProbeResult{.index = slot, .foundExisting = true};
            }
            slot = AdvanceIndex(slot, mask);
        }

        if (firstTombstone != kInvalidIndex) {
            return InsertProbeResult{.index = firstTombstone, .foundExisting = false};
        }
        return InsertProbeResult{.index = kInvalidIndex, .foundExisting = false};
    }

    [[nodiscard]] usize FindExistingIndex(const KeyT& key) const noexcept
    {
        if (Capacity_ == 0U) {
            return kInvalidIndex;
        }

        const usize mask = Capacity_ - 1U;
        usize slot = HashIndexForKey(key, Capacity_);
        for (usize probe = 0U; probe < Capacity_; ++probe) {
            const SlotState state = States_[slot];
            if (state == SlotState::EMPTY) {
                return kInvalidIndex;
            }
            if (state == SlotState::OCCUPIED && Entries_[slot].key == key) {
                return slot;
            }
            slot = AdvanceIndex(slot, mask);
        }
        return kInvalidIndex;
    }

    template <typename InsertKeyT, typename InsertValueT>
    void ConstructEntryAt(usize index, InsertKeyT&& key, InsertValueT&& value)
    {
        if (States_[index] == SlotState::TOMBSTONE) {
            ZCORE_CONTRACT_REQUIRE(Tombstones_ > 0U,
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::HashMap::ConstructEntryAt() tombstone accounting underflow");
            --Tombstones_;
        }
        std::construct_at(std::addressof(Entries_[index]), std::forward<InsertKeyT>(key), std::forward<InsertValueT>(value));
        States_[index] = SlotState::OCCUPIED;
        ++Size_;
    }

    void DestroyEntryAt(usize index) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(States_[index] == SlotState::OCCUPIED,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::HashMap::DestroyEntryAt() requires occupied slot");
        std::destroy_at(std::addressof(Entries_[index]));
        States_[index] = SlotState::TOMBSTONE;
        --Size_;
        ++Tombstones_;
    }

    void DestroyAllEntries() noexcept
    {
        if (Capacity_ == 0U) {
            Size_ = 0U;
            Tombstones_ = 0U;
            return;
        }

        for (usize index = 0U; index < Capacity_; ++index) {
            if (States_[index] == SlotState::OCCUPIED) {
                std::destroy_at(std::addressof(Entries_[index]));
            }
        }
        Size_ = 0U;
        Tombstones_ = 0U;
    }

    void ReleaseStorage() noexcept
    {
        if (!StateAllocation_.IsEmpty()) {
            ZCORE_CONTRACT_REQUIRE(Allocator_ != nullptr,
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::HashMap requires allocator to release owned state allocation");
            const Status releaseStatus = Allocator_->Deallocate(StateAllocation_);
            ZCORE_CONTRACT_REQUIRE(releaseStatus.HasValue(),
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::HashMap failed to deallocate owned state allocation");
        }
        if (!EntryAllocation_.IsEmpty()) {
            ZCORE_CONTRACT_REQUIRE(Allocator_ != nullptr,
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::HashMap requires allocator to release owned entry allocation");
            const Status releaseStatus = Allocator_->Deallocate(EntryAllocation_);
            ZCORE_CONTRACT_REQUIRE(releaseStatus.HasValue(),
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::HashMap failed to deallocate owned entry allocation");
        }
        StateAllocation_ = Allocation::Empty();
        EntryAllocation_ = Allocation::Empty();
        States_ = nullptr;
        Entries_ = nullptr;
        Capacity_ = 0U;
    }

    void RequireStorageInitialized(const char* message) const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(Capacity_ > 0U && States_ != nullptr && Entries_ != nullptr,
                               detail::ContractViolationCode::PRECONDITION,
                               message);
    }

    constexpr void ClearState() noexcept
    {
        StateAllocation_ = Allocation::Empty();
        EntryAllocation_ = Allocation::Empty();
        States_ = nullptr;
        Entries_ = nullptr;
        Allocator_ = nullptr;
        Capacity_ = 0U;
        Size_ = 0U;
        Tombstones_ = 0U;
    }

    Allocation StateAllocation_;
    Allocation EntryAllocation_;
    SlotState* States_;
    Entry* Entries_;
    Allocator* Allocator_;
    usize Capacity_;
    usize Size_;
    usize Tombstones_;
};

} // namespace zcore
