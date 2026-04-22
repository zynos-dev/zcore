/**************************************************************************/
/*  btree_map.hpp                                                         */
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
 * @file include/zcore/container/btree_map.hpp
 * @brief Allocator-backed ordered map with deterministic sorted storage.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/btree_map.hpp>
 * zcore::BTreeMap<int, int> values(allocator);
 * values.TryInsert(7, 11);
 * @endcode
 */

#pragma once

#include <concepts>
#include <functional>
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
#include <zcore/vector.hpp>

namespace zcore {

/**
 * @brief Allocator-backed ordered associative container.
 *
 * @tparam KeyT Key type.
 * @tparam ValueT Mapped value type.
 * @tparam CompareT Strict-weak ordering comparator (defaults to `std::less<KeyT>`).
 *
 * This v1 implementation stores entries in sorted contiguous order.
 */
template <typename KeyT, typename ValueT, typename CompareT = std::less<KeyT>>
class [[nodiscard("BTreeMap must be handled explicitly.")]] BTreeMap final {
public:
    ZCORE_STATIC_REQUIRE(constraints::MutableNothrowMovableValueType<KeyT>,
                         "BTreeMap<KeyT, ValueT, CompareT> requires mutable nothrow-movable key type.");
    ZCORE_STATIC_REQUIRE(constraints::MutableNothrowMovableValueType<ValueT>,
                         "BTreeMap<KeyT, ValueT, CompareT> requires mutable nothrow-movable value type.");
    ZCORE_STATIC_REQUIRE(std::equality_comparable<KeyT>, "BTreeMap<KeyT, ValueT, CompareT> requires equality-comparable key type.");

    struct Entry final {
        KeyT key;
        ValueT value;
    };

    using KeyType = KeyT;
    using MappedType = ValueT;
    using CompareType = CompareT;
    using SizeType = usize;
    using Iterator = Entry*;
    using ConstIterator = const Entry*;

    /// @brief Constructs an empty unbound map.
    constexpr BTreeMap() noexcept = default;

    /// @brief Constructs an empty allocator-bound map.
    constexpr explicit BTreeMap(Allocator& allocator) noexcept : Entries_(allocator)
    {
    }

    BTreeMap(const BTreeMap&) = delete;
    BTreeMap& operator=(const BTreeMap&) = delete;
    constexpr BTreeMap(BTreeMap&&) noexcept = default;
    constexpr BTreeMap& operator=(BTreeMap&&) noexcept = default;
    ~BTreeMap() = default;

    /**
   * @brief Creates allocator-bound map with reserved entry capacity.
   */
    [[nodiscard]] static Result<BTreeMap, Error> TryWithCapacity(Allocator& allocator, usize capacity) noexcept
    {
        BTreeMap out(allocator);
        Status reserveStatus = out.TryReserve(capacity);
        if (reserveStatus.HasError()) {
            return Result<BTreeMap, Error>::Failure(reserveStatus.Error());
        }
        return Result<BTreeMap, Error>::Success(std::move(out));
    }

    /// @brief Returns whether allocator binding is present.
    [[nodiscard]] constexpr bool HasAllocator() const noexcept
    {
        return Entries_.HasAllocator();
    }

    /// @brief Returns bound allocator pointer or null when unbound.
    [[nodiscard]] constexpr Allocator* AllocatorRef() const noexcept
    {
        return Entries_.AllocatorRef();
    }

    /// @brief Returns current entry count.
    [[nodiscard]] constexpr usize Size() const noexcept
    {
        return Entries_.Size();
    }

    [[nodiscard]] constexpr usize size() const noexcept
    {
        return Size();
    }

    /// @brief Returns allocated entry capacity.
    [[nodiscard]] constexpr usize Capacity() const noexcept
    {
        return Entries_.Capacity();
    }

    /// @brief Returns remaining insertion capacity before growth.
    [[nodiscard]] constexpr usize RemainingCapacity() const noexcept
    {
        return Entries_.RemainingCapacity();
    }

    /// @brief Returns `true` when map is empty.
    [[nodiscard]] constexpr bool Empty() const noexcept
    {
        return Entries_.Empty();
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return Empty();
    }

    /// @brief Returns iterator to first entry.
    [[nodiscard]] constexpr Iterator begin() noexcept
    {
        return Entries_.begin();
    }

    /// @brief Returns iterator past last entry.
    [[nodiscard]] constexpr Iterator end() noexcept
    {
        return Entries_.end();
    }

    /// @brief Returns const iterator to first entry.
    [[nodiscard]] constexpr ConstIterator begin() const noexcept
    {
        return Entries_.begin();
    }

    /// @brief Returns const iterator past last entry.
    [[nodiscard]] constexpr ConstIterator end() const noexcept
    {
        return Entries_.end();
    }

    [[nodiscard]] constexpr ConstIterator cbegin() const noexcept
    {
        return Entries_.cbegin();
    }

    [[nodiscard]] constexpr ConstIterator cend() const noexcept
    {
        return Entries_.cend();
    }

    /// @brief Returns whether key exists.
    [[nodiscard]] bool Contains(const KeyT& key) const noexcept
    {
        return FindIndex(key).HasValue();
    }

    /// @brief Returns pointer to mapped value or `nullptr` when key is missing.
    [[nodiscard]] ValueT* TryGet(const KeyT& key) noexcept
    {
        auto found = FindIndex(key);
        return found.HasValue() ? std::addressof(Entries_[found.Value()].value) : nullptr;
    }

    /// @brief Returns pointer to mapped value or `nullptr` when key is missing.
    [[nodiscard]] const ValueT* TryGet(const KeyT& key) const noexcept
    {
        auto found = FindIndex(key);
        return found.HasValue() ? std::addressof(Entries_[found.Value()].value) : nullptr;
    }

    /**
   * @brief Returns mapped value reference for existing key.
   * @pre `Contains(key)`.
   */
    [[nodiscard]] ValueT& Get(const KeyT& key) noexcept
    {
        auto found = FindIndex(key);
        ZCORE_CONTRACT_REQUIRE(found.HasValue(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::BTreeMap::Get() key not found");
        return Entries_[found.Value()].value;
    }

    /**
   * @brief Returns mapped value reference for existing key.
   * @pre `Contains(key)`.
   */
    [[nodiscard]] const ValueT& Get(const KeyT& key) const noexcept
    {
        auto found = FindIndex(key);
        ZCORE_CONTRACT_REQUIRE(found.HasValue(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::BTreeMap::Get() key not found");
        return Entries_[found.Value()].value;
    }

    /**
   * @brief Ensures capacity for at least `capacity` entries.
   * @return Success or allocator-domain error.
   */
    [[nodiscard]] Status TryReserve(usize capacity) noexcept
    {
        return Entries_.TryReserve(capacity);
    }

    /**
   * @brief Attempts to insert copied key/value pair.
   * @return `Result<bool, Error>` where `true` = inserted, `false` = key already present.
   */
    [[nodiscard]] Result<bool, Error> TryInsert(const KeyT& key, const ValueT& value)
        requires(std::is_copy_constructible_v<KeyT> && std::is_copy_constructible_v<ValueT>)
    {
        return InsertImpl(key, value);
    }

    /**
   * @brief Attempts to insert moved key/value pair.
   * @return `Result<bool, Error>` where `true` = inserted, `false` = key already present.
   */
    [[nodiscard]] Result<bool, Error> TryInsert(KeyT&& key, ValueT&& value)
        requires(std::is_move_constructible_v<KeyT> && std::is_move_constructible_v<ValueT>)
    {
        return InsertImpl(std::move(key), std::move(value));
    }

    /**
   * @brief Attempts to insert or assign copied mapped value.
   * @return Success or allocator-domain error.
   */
    [[nodiscard]] Status TryInsertOrAssign(const KeyT& key, const ValueT& value)
        requires(std::is_copy_constructible_v<KeyT> && std::is_copy_constructible_v<ValueT> && std::is_copy_assignable_v<ValueT>)
    {
        auto found = FindIndex(key);
        if (found.HasValue()) {
            Entries_[found.Value()].value = value;
            return OkStatus();
        }
        auto inserted = InsertImpl(key, value);
        if (inserted.HasError()) {
            return ErrorStatus(inserted.Error());
        }
        return OkStatus();
    }

    /**
   * @brief Attempts to insert or assign moved mapped value.
   * @return Success or allocator-domain error.
   */
    [[nodiscard]] Status TryInsertOrAssign(KeyT&& key, ValueT&& value)
        requires(std::is_move_constructible_v<KeyT> && std::is_move_constructible_v<ValueT> && std::is_move_assignable_v<ValueT>)
    {
        auto found = FindIndex(key);
        if (found.HasValue()) {
            Entries_[found.Value()].value = std::move(value);
            return OkStatus();
        }
        auto inserted = InsertImpl(std::move(key), std::move(value));
        if (inserted.HasError()) {
            return ErrorStatus(inserted.Error());
        }
        return OkStatus();
    }

    /**
   * @brief Attempts to remove key.
   * @return `true` on success, `false` when key is missing.
   */
    [[nodiscard]] bool TryRemove(const KeyT& key) noexcept
    {
        auto found = FindIndex(key);
        if (!found.HasValue()) {
            return false;
        }
        RemoveAt(found.Value());
        return true;
    }

    /**
   * @brief Attempts to remove key and move out mapped value.
   * @return `None` when key is missing.
   */
    [[nodiscard]] Option<ValueT> TryRemoveValue(const KeyT& key)
        requires(std::is_move_constructible_v<ValueT>)
    {
        auto found = FindIndex(key);
        if (!found.HasValue()) {
            return None;
        }
        const usize index = found.Value();
        Option<ValueT> moved(std::move(Entries_[index].value));
        RemoveAt(index);
        return moved;
    }

    /// @brief Clears entries while retaining allocated capacity.
    void Clear() noexcept
    {
        Entries_.Clear();
    }

    /// @brief Clears entries and deallocates owned storage.
    void Reset() noexcept
    {
        Entries_.Reset();
    }

private:
    [[nodiscard]] bool KeysEqual(const KeyT& lhs, const KeyT& rhs) const
    {
        return !Compare_(lhs, rhs) && !Compare_(rhs, lhs);
    }

    [[nodiscard]] usize LowerBoundIndex(const KeyT& key) const
    {
        usize first = 0U;
        usize count = Size();
        while (count > 0U) {
            const usize step = count / 2U;
            const usize middle = first + step;
            if (Compare_(Entries_[middle].key, key)) {
                first = middle + 1U;
                count -= step + 1U;
            }
            else {
                count = step;
            }
        }
        return first;
    }

    [[nodiscard]] Option<usize> FindIndex(const KeyT& key) const
    {
        if (Empty()) {
            return None;
        }
        const usize index = LowerBoundIndex(key);
        if (index < Size() && KeysEqual(Entries_[index].key, key)) {
            return Option<usize>(index);
        }
        return None;
    }

    template <typename InsertKeyT, typename InsertValueT>
    [[nodiscard]] Result<bool, Error> InsertImpl(InsertKeyT&& key, InsertValueT&& value)
    {
        const usize index = LowerBoundIndex(key);
        if (index < Size() && KeysEqual(Entries_[index].key, key)) {
            return Result<bool, Error>::Success(false);
        }

        Status reserveStatus = Entries_.TryReserve(Size() + 1U);
        if (reserveStatus.HasError()) {
            return Result<bool, Error>::Failure(reserveStatus.Error());
        }

        if (index == Size()) {
            Status pushStatus = Entries_.TryPushBack(Entry{
                    .key = std::forward<InsertKeyT>(key),
                    .value = std::forward<InsertValueT>(value),
            });
            if (pushStatus.HasError()) {
                return Result<bool, Error>::Failure(pushStatus.Error());
            }
            return Result<bool, Error>::Success(true);
        }

        const usize previousSize = Size();
        Status expandStatus = Entries_.TryPushBack(std::move(Entries_[previousSize - 1U]));
        if (expandStatus.HasError()) {
            return Result<bool, Error>::Failure(expandStatus.Error());
        }

        for (usize cursor = previousSize - 1U; cursor > index; --cursor) {
            Entries_[cursor] = std::move(Entries_[cursor - 1U]);
        }
        Entries_[index] = Entry{
                .key = std::forward<InsertKeyT>(key),
                .value = std::forward<InsertValueT>(value),
        };
        return Result<bool, Error>::Success(true);
    }

    void RemoveAt(usize index) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(index < Size(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::BTreeMap::RemoveAt() index out of bounds");
        const usize count = Size();
        for (usize cursor = index; (cursor + 1U) < count; ++cursor) {
            Entries_[cursor] = std::move(Entries_[cursor + 1U]);
        }
        const bool popped = Entries_.TryPopBack();
        ZCORE_CONTRACT_REQUIRE(popped,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::BTreeMap::RemoveAt() pop back must succeed");
    }

    Vector<Entry> Entries_;
    [[no_unique_address]] CompareT Compare_{};
};

} // namespace zcore
