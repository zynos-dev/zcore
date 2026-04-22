/**************************************************************************/
/*  array.hpp                                                             */
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
 * @file include/zcore/container/array.hpp
 * @brief Deterministic fixed-extent owning contiguous container.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/array.hpp>
 * zcore::Array<int, 3> values;
 * values[0] = 7;
 * @endcode
 */

#pragma once

#include <array>
#include <concepts>
#include <type_traits>
#include <zcore/contract_violation.hpp>
#include <zcore/foundation.hpp>
#include <zcore/slice.hpp>
#include <zcore/type_constraints.hpp>

namespace zcore {

/**
 * @brief Fixed-extent owning contiguous container.
 *
 * @tparam ValueT Element type.
 * @tparam ExtentV Compile-time element count.
 */
template <typename ValueT, usize ExtentV>
class [[nodiscard("Array must be handled explicitly.")]] Array final {
public:
    ZCORE_STATIC_REQUIRE(constraints::MutableValueObjectType<ValueT>,
                         "Array<ValueT, ExtentV> requires mutable non-void object element type.");

    using ValueType = ValueT;
    using SizeType = usize;
    using Pointer = ValueT*;
    using ConstPointer = const ValueT*;
    using Iterator = ValueT*;
    using ConstIterator = const ValueT*;

    /// @brief Compile-time extent.
    static constexpr usize kExtent = ExtentV;

    /// @brief Constructs a value-initialized array.
    constexpr Array() = default;
    constexpr Array(const Array&) = default;
    constexpr Array& operator=(const Array&) = default;
    constexpr Array(Array&&) noexcept = default;
    constexpr Array& operator=(Array&&) noexcept = default;
    ~Array() = default;

    /// @brief Constructs from a `std::array` with matching extent.
    constexpr explicit Array(const std::array<ValueT, ExtentV>& values) noexcept : Storage_(values)
    {
    }

    /// @brief Constructs from a C array with matching extent.
    constexpr explicit Array(const ValueT (&values)[ExtentV]) noexcept
    {
        for (usize index = 0; index < kExtent; ++index) {
            Storage_[index] = values[index];
        }
    }

    /// @brief Returns compile-time extent.
    [[nodiscard]] static constexpr usize Extent() noexcept
    {
        return kExtent;
    }

    /// @brief Returns element count.
    [[nodiscard]] constexpr usize Size() const noexcept
    {
        return kExtent;
    }

    [[nodiscard]] constexpr usize size() const noexcept
    {
        return Size();
    }

    /// @brief Returns `true` when extent is zero.
    [[nodiscard]] constexpr bool Empty() const noexcept
    {
        return kExtent == 0;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return Empty();
    }

    /// @brief Returns pointer to contiguous storage, or null for zero-extent arrays.
    [[nodiscard]] constexpr Pointer Data() noexcept
    {
        if constexpr (kExtent == 0) {
            return nullptr;
        }
        return Storage_.data();
    }

    /// @brief Returns pointer to contiguous storage, or null for zero-extent arrays.
    [[nodiscard]] constexpr ConstPointer Data() const noexcept
    {
        if constexpr (kExtent == 0) {
            return nullptr;
        }
        return Storage_.data();
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
        if constexpr (kExtent == 0) {
            return nullptr;
        }
        return Data() + kExtent;
    }

    /// @brief Returns const iterator to first element.
    [[nodiscard]] constexpr ConstIterator begin() const noexcept
    {
        return Data();
    }

    /// @brief Returns const iterator past last element.
    [[nodiscard]] constexpr ConstIterator end() const noexcept
    {
        if constexpr (kExtent == 0) {
            return nullptr;
        }
        return Data() + kExtent;
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
   * @brief Bounds-checked indexed access.
   * @pre `index < Size()`.
   */
    [[nodiscard]] constexpr ValueT& operator[](usize index) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(index < kExtent,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Array::operator[] index out of bounds");
        return Storage_[index];
    }

    /**
   * @brief Bounds-checked indexed access.
   * @pre `index < Size()`.
   */
    [[nodiscard]] constexpr const ValueT& operator[](usize index) const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(index < kExtent,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Array::operator[] index out of bounds");
        return Storage_[index];
    }

    /// @brief Returns pointer to element at `index` or `nullptr` when out-of-range.
    [[nodiscard]] constexpr Pointer TryAt(usize index) noexcept
    {
        if (index >= kExtent) {
            return nullptr;
        }
        return Data() + index;
    }

    /// @brief Returns pointer to element at `index` or `nullptr` when out-of-range.
    [[nodiscard]] constexpr ConstPointer TryAt(usize index) const noexcept
    {
        if (index >= kExtent) {
            return nullptr;
        }
        return Data() + index;
    }

    /**
   * @brief Returns first element.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr ValueT& Front() noexcept
    {
        RequireNotEmpty("zcore::Array::Front() requires non-empty array");
        return Storage_[0];
    }

    /**
   * @brief Returns first element.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr const ValueT& Front() const noexcept
    {
        RequireNotEmpty("zcore::Array::Front() requires non-empty array");
        return Storage_[0];
    }

    /**
   * @brief Returns last element.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr ValueT& Back() noexcept
    {
        RequireNotEmpty("zcore::Array::Back() requires non-empty array");
        return Storage_[kExtent == 0 ? 0 : (kExtent - 1)];
    }

    /**
   * @brief Returns last element.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr const ValueT& Back() const noexcept
    {
        RequireNotEmpty("zcore::Array::Back() requires non-empty array");
        return Storage_[kExtent == 0 ? 0 : (kExtent - 1)];
    }

    /// @brief Assigns all elements to `value`.
    constexpr void Fill(const ValueT& value)
        requires(std::is_copy_assignable_v<ValueT>)
    {
        Storage_.fill(value);
    }

    /// @brief Returns immutable contiguous view over all elements.
    [[nodiscard]] constexpr Slice<const ValueT> AsSlice() const noexcept
    {
        if constexpr (kExtent == 0) {
            return Slice<const ValueT>::Empty();
        }
        return Slice<const ValueT>::FromRawUnchecked(Data(), kExtent);
    }

    /// @brief Returns mutable contiguous view over all elements.
    [[nodiscard]] constexpr SliceMut<ValueT> AsSliceMut() noexcept
    {
        if constexpr (kExtent == 0) {
            return SliceMut<ValueT>::Empty();
        }
        return SliceMut<ValueT>::FromRawUnchecked(Data(), kExtent);
    }

    [[nodiscard]] constexpr operator Slice<const ValueT>() const noexcept
    {
        return AsSlice();
    }

    [[nodiscard]] constexpr bool operator==(const Array& other) const
        requires(std::equality_comparable<ValueT>)
    {
        for (usize index = 0; index < kExtent; ++index) {
            if (!(Storage_[index] == other.Storage_[index])) {
                return false;
            }
        }
        return true;
    }

private:
    constexpr void RequireNotEmpty(const char* message) const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(kExtent > 0, detail::ContractViolationCode::PRECONDITION, message);
    }

    std::array<ValueT, kExtent> Storage_{};
};

} // namespace zcore
