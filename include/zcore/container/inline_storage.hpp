/**************************************************************************/
/*  inline_storage.hpp                                                    */
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
 * @file include/zcore/container/inline_storage.hpp
 * @brief Fixed-capacity in-place slot storage with explicit construction/destruction.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/inline_storage.hpp>
 * zcore::InlineStorage<int, 4> storage;
 * storage.ConstructAt(0, 7);
 * const int value = storage.RefAt(0);
 * storage.DestroyAt(0);
 * @endcode
 */

#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <zcore/contract_violation.hpp>
#include <zcore/foundation.hpp>
#include <zcore/type_constraints.hpp>

namespace zcore {

/**
 * @brief Fixed-capacity in-place slot storage with explicit lifetime control.
 *
 * This type does not track which slots are constructed. Callers must ensure:
 * - each constructed slot is destroyed exactly once
 * - no read from unconstructed slots
 * - no double construction without an intervening destroy
 */
template <typename ValueT, usize CapacityV>
class [[nodiscard("InlineStorage must be handled explicitly.")]] InlineStorage final {
public:
    ZCORE_STATIC_REQUIRE(constraints::MutableValueObjectType<ValueT>,
                         "InlineStorage<ValueT, CapacityV> requires mutable non-void object value type.");

    using ValueType = ValueT;
    using SizeType = usize;
    using Pointer = ValueT*;
    using ConstPointer = const ValueT*;

    /// @brief Compile-time slot capacity.
    static constexpr usize kCapacity = CapacityV;

    /// @brief Constructs empty raw storage.
    constexpr InlineStorage() noexcept = default;
    ~InlineStorage() = default;
    InlineStorage(const InlineStorage&) = delete;
    InlineStorage& operator=(const InlineStorage&) = delete;
    InlineStorage(InlineStorage&&) = delete;
    InlineStorage& operator=(InlineStorage&&) = delete;

    /// @brief Returns compile-time slot capacity.
    [[nodiscard]] static constexpr usize Capacity() noexcept
    {
        return kCapacity;
    }

    /// @brief Returns pointer to first slot, or `nullptr` when capacity is zero.
    [[nodiscard]] constexpr Pointer Data() noexcept
    {
        if constexpr (kCapacity == 0) {
            return nullptr;
        }
        return PtrAtUnchecked(0);
    }

    /// @brief Returns pointer to first slot, or `nullptr` when capacity is zero.
    [[nodiscard]] constexpr ConstPointer Data() const noexcept
    {
        if constexpr (kCapacity == 0) {
            return nullptr;
        }
        return PtrAtUnchecked(0);
    }

    [[nodiscard]] constexpr Pointer data() noexcept
    {
        return Data();
    }

    [[nodiscard]] constexpr ConstPointer data() const noexcept
    {
        return Data();
    }

    /// @brief Returns slot pointer at `index` or `nullptr` when out-of-range.
    [[nodiscard]] constexpr Pointer TryPtrAt(const usize index) noexcept
    {
        return index < kCapacity ? PtrAtUnchecked(index) : nullptr;
    }

    /// @brief Returns slot pointer at `index` or `nullptr` when out-of-range.
    [[nodiscard]] constexpr ConstPointer TryPtrAt(const usize index) const noexcept
    {
        return index < kCapacity ? PtrAtUnchecked(index) : nullptr;
    }

    /**
   * @brief Returns slot pointer at `index`.
   * @pre `index < Capacity()`.
   */
    [[nodiscard]] constexpr Pointer PtrAt(const usize index) noexcept
    {
        RequireInRange(index, "zcore::InlineStorage::PtrAt() index out of bounds");
        return PtrAtUnchecked(index);
    }

    /**
   * @brief Returns slot pointer at `index`.
   * @pre `index < Capacity()`.
   */
    [[nodiscard]] constexpr ConstPointer PtrAt(const usize index) const noexcept
    {
        RequireInRange(index, "zcore::InlineStorage::PtrAt() index out of bounds");
        return PtrAtUnchecked(index);
    }

    /**
   * @brief Returns reference to constructed slot value.
   * @pre `index < Capacity()` and slot at `index` is constructed.
   */
    [[nodiscard]] constexpr ValueT& RefAt(const usize index) noexcept
    {
        return *PtrAt(index);
    }

    /**
   * @brief Returns reference to constructed slot value.
   * @pre `index < Capacity()` and slot at `index` is constructed.
   */
    [[nodiscard]] constexpr const ValueT& RefAt(const usize index) const noexcept
    {
        return *PtrAt(index);
    }

    /**
   * @brief Constructs value in slot.
   * @pre `index < Capacity()` and slot at `index` is not currently constructed.
   */
    template <typename... ArgsT>
        requires(std::is_constructible_v<ValueT, ArgsT && ...>)
    constexpr ValueT& ConstructAt(const usize index, ArgsT&&... args)
    {
        return *std::construct_at(PtrAt(index), std::forward<ArgsT>(args)...);
    }

    /**
   * @brief Destroys value in slot.
   * @pre `index < Capacity()` and slot at `index` is currently constructed.
   */
    constexpr void DestroyAt(const usize index) noexcept
    {
        std::destroy_at(PtrAt(index));
    }

private:
    static constexpr usize kStorageCount = kCapacity == 0 ? 1 : kCapacity;

    [[nodiscard]] constexpr Pointer PtrAtUnchecked(const usize index) noexcept
    {
        return std::launder(reinterpret_cast<Pointer>(Storage_ + (index * sizeof(ValueT))));
    }

    [[nodiscard]] constexpr ConstPointer PtrAtUnchecked(const usize index) const noexcept
    {
        return std::launder(reinterpret_cast<ConstPointer>(Storage_ + (index * sizeof(ValueT))));
    }

    constexpr void RequireInRange(const usize index, const char* message) const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(index < kCapacity, detail::ContractViolationCode::PRECONDITION, message);
    }

    alignas(ValueT) std::byte Storage_[sizeof(ValueT) * kStorageCount]{};
};

} // namespace zcore
