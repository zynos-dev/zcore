/**************************************************************************/
/*  hash_set.hpp                                                          */
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
 * @file include/zcore/container/hash_set.hpp
 * @brief Allocator-backed hash set built on `HashMap`.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/hash_set.hpp>
 * zcore::HashSet<int> values(allocator);
 * values.TryInsert(7);
 * @endcode
 */

#pragma once

#include <zcore/container/hash_map.hpp>
#include <zcore/foundation.hpp>
#include <zcore/result.hpp>
#include <zcore/status.hpp>

namespace zcore {

/**
 * @brief Allocator-backed hash set with open-addressing storage.
 *
 * @tparam ValueT Element type.
 */
template <typename ValueT>
class [[nodiscard("HashSet must be handled explicitly.")]] HashSet final {
public:
    using ValueType = ValueT;
    using SizeType = usize;

    constexpr HashSet() noexcept = default;
    constexpr explicit HashSet(Allocator& allocator) noexcept : Storage_(allocator)
    {
    }

    HashSet(const HashSet&) = delete;
    HashSet& operator=(const HashSet&) = delete;
    constexpr HashSet(HashSet&&) noexcept = default;
    constexpr HashSet& operator=(HashSet&&) noexcept = default;
    ~HashSet() = default;

    /**
   * @brief Creates allocator-bound set with reserved element capacity.
   * @param allocator Allocator used for storage.
   * @param elementCapacity Minimum element capacity under default load factor.
   */
    [[nodiscard]] static Result<HashSet, Error> TryWithCapacity(Allocator& allocator, usize elementCapacity) noexcept
    {
        auto mapResult = StorageType::TryWithCapacity(allocator, elementCapacity);
        if (mapResult.HasError()) {
            return Result<HashSet, Error>::Failure(mapResult.Error());
        }
        HashSet out;
        out.Storage_ = std::move(mapResult.Value());
        return Result<HashSet, Error>::Success(std::move(out));
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

    /// @brief Returns bucket capacity.
    [[nodiscard]] constexpr usize Capacity() const noexcept
    {
        return Storage_.Capacity();
    }

    /// @brief Returns remaining entries before growth at target load factor.
    [[nodiscard]] constexpr usize RemainingCapacity() const noexcept
    {
        return Storage_.RemainingCapacity();
    }

    /// @brief Returns `true` when no elements are stored.
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

    /// @brief Clears elements while retaining allocated buckets.
    void Clear() noexcept
    {
        Storage_.Clear();
    }

    /// @brief Clears elements and deallocates owned bucket storage.
    void Reset() noexcept
    {
        Storage_.Reset();
    }

private:
    using StorageType = HashMap<ValueT, u8>;
    StorageType Storage_;
};

} // namespace zcore
