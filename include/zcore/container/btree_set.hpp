/**************************************************************************/
/*  btree_set.hpp                                                         */
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
 * @file include/zcore/container/btree_set.hpp
 * @brief Allocator-backed ordered set built on `BTreeMap`.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/btree_set.hpp>
 * zcore::BTreeSet<int> values(allocator);
 * values.TryInsert(7);
 * @endcode
 */

#pragma once

#include <functional>
#include <type_traits>
#include <utility>
#include <zcore/container/btree_map.hpp>
#include <zcore/foundation.hpp>
#include <zcore/result.hpp>
#include <zcore/status.hpp>

namespace zcore {

/**
 * @brief Allocator-backed ordered associative set.
 *
 * @tparam ValueT Element type.
 * @tparam CompareT Strict-weak ordering comparator.
 */
template <typename ValueT, typename CompareT = std::less<ValueT>>
class [[nodiscard("BTreeSet must be handled explicitly.")]] BTreeSet final {
public:
    using ValueType = ValueT;
    using CompareType = CompareT;
    using SizeType = usize;

    constexpr BTreeSet() noexcept = default;
    constexpr explicit BTreeSet(Allocator& allocator) noexcept : Storage_(allocator)
    {
    }

    BTreeSet(const BTreeSet&) = delete;
    BTreeSet& operator=(const BTreeSet&) = delete;
    constexpr BTreeSet(BTreeSet&&) noexcept = default;
    constexpr BTreeSet& operator=(BTreeSet&&) noexcept = default;
    ~BTreeSet() = default;

    /**
   * @brief Creates allocator-bound set with reserved element capacity.
   */
    [[nodiscard]] static Result<BTreeSet, Error> TryWithCapacity(Allocator& allocator, usize elementCapacity) noexcept
    {
        auto mapResult = StorageType::TryWithCapacity(allocator, elementCapacity);
        if (mapResult.HasError()) {
            return Result<BTreeSet, Error>::Failure(mapResult.Error());
        }
        BTreeSet out;
        out.Storage_ = std::move(mapResult.Value());
        return Result<BTreeSet, Error>::Success(std::move(out));
    }

    /// @brief Returns whether allocator binding is present.
    [[nodiscard]] constexpr bool HasAllocator() const noexcept
    {
        return Storage_.HasAllocator();
    }

    /// @brief Returns bound allocator pointer or null when unbound.
    [[nodiscard]] constexpr Allocator* AllocatorRef() const noexcept
    {
        return Storage_.AllocatorRef();
    }

    /// @brief Returns current element count.
    [[nodiscard]] constexpr usize Size() const noexcept
    {
        return Storage_.Size();
    }

    [[nodiscard]] constexpr usize size() const noexcept
    {
        return Size();
    }

    /// @brief Returns allocated entry capacity.
    [[nodiscard]] constexpr usize Capacity() const noexcept
    {
        return Storage_.Capacity();
    }

    /// @brief Returns remaining insertion capacity before growth.
    [[nodiscard]] constexpr usize RemainingCapacity() const noexcept
    {
        return Storage_.RemainingCapacity();
    }

    /// @brief Returns `true` when set is empty.
    [[nodiscard]] constexpr bool Empty() const noexcept
    {
        return Storage_.Empty();
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return Empty();
    }

    /// @brief Returns `true` when value exists.
    [[nodiscard]] bool Contains(const ValueT& value) const noexcept
    {
        return Storage_.Contains(value);
    }

    /**
   * @brief Ensures capacity for at least `elementCapacity` elements.
   * @return Success or allocator-domain error.
   */
    [[nodiscard]] Status TryReserve(usize elementCapacity) noexcept
    {
        return Storage_.TryReserve(elementCapacity);
    }

    /**
   * @brief Attempts to insert copied value.
   * @return `Result<bool, Error>` where `true` = inserted, `false` = already present.
   */
    [[nodiscard]] Result<bool, Error> TryInsert(const ValueT& value)
        requires(std::is_copy_constructible_v<ValueT>)
    {
        return Storage_.TryInsert(value, static_cast<u8>(0U));
    }

    /**
   * @brief Attempts to insert moved value.
   * @return `Result<bool, Error>` where `true` = inserted, `false` = already present.
   */
    [[nodiscard]] Result<bool, Error> TryInsert(ValueT&& value)
        requires(std::is_move_constructible_v<ValueT>)
    {
        return Storage_.TryInsert(std::move(value), static_cast<u8>(0U));
    }

    /**
   * @brief Attempts to remove value.
   * @return `true` on success, `false` when value is missing.
   */
    [[nodiscard]] bool TryRemove(const ValueT& value) noexcept
    {
        return Storage_.TryRemove(value);
    }

    /// @brief Clears elements while retaining allocated capacity.
    void Clear() noexcept
    {
        Storage_.Clear();
    }

    /// @brief Clears elements and deallocates owned storage.
    void Reset() noexcept
    {
        Storage_.Reset();
    }

private:
    using StorageType = BTreeMap<ValueT, u8, CompareT>;
    StorageType Storage_;
};

} // namespace zcore
