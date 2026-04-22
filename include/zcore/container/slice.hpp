/**************************************************************************/
/*  slice.hpp                                                             */
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
 * @file include/zcore/container/slice.hpp
 * @brief Non-owning bounds-aware contiguous view types.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/slice.hpp>
 * int values[3] = {1, 2, 3};
 * zcore::Slice<int> view(values);
 * @endcode
 */

#pragma once

#include <array>
#include <span>
#include <type_traits>
#include <zcore/contract_violation.hpp>
#include <zcore/foundation.hpp>
#include <zcore/option.hpp>
#include <zcore/type_constraints.hpp>

namespace zcore {

/**
 * @brief Immutable non-owning contiguous view.
 *
 * @tparam ValueT Element type (object type only).
 *
 * `Slice` stores pointer + length and never owns memory.
 */
template <typename ValueT>
class [[nodiscard("Slice must be handled explicitly.")]] Slice final {
public:
    ZCORE_STATIC_REQUIRE(constraints::ObjectType<ValueT>&& constraints::NonReferenceType<ValueT>,
                         "Slice<ValueT> requires non-reference object element type.");

    using ElementType = ValueT;
    using Pointer = const ValueT*;
    using Iterator = const ValueT*;

    /// @brief Constructs an empty slice.
    constexpr Slice() noexcept : Ptr_(nullptr), Size_(0)
    {
    }

    /**
   * @brief Constructs from pointer and length.
   * @param pointer Start address.
   * @param size Element count.
   *
   * Aborts if `size > 0` and `pointer == nullptr`.
   */
    constexpr Slice(Pointer pointer, usize size) noexcept : Ptr_(pointer), Size_(size)
    {
        EnsureValidRange(pointer, size);
    }

    template <usize ExtentV>
    constexpr explicit Slice(const std::array<std::remove_const_t<ValueT>, ExtentV>& values) noexcept
            : Ptr_(values.data()), Size_(ExtentV)
    {
    }

    template <usize ExtentV>
    constexpr Slice(const ValueT (&values)[ExtentV]) noexcept : Ptr_(values), Size_(ExtentV)
    {
    }

    constexpr explicit Slice(std::span<const ValueT> values) noexcept : Ptr_(values.data()), Size_(values.size())
    {
    }

    constexpr Slice(const Slice&) noexcept = default;
    constexpr Slice& operator=(const Slice&) noexcept = default;
    constexpr Slice(Slice&&) noexcept = default;
    constexpr Slice& operator=(Slice&&) noexcept = default;
    ~Slice() = default;

    /// @brief Returns an empty slice.
    [[nodiscard]] static constexpr Slice Empty() noexcept
    {
        return Slice();
    }

    /**
   * @brief Attempts construction from pointer + size.
   * @return `None` when `size > 0` and `pointer == nullptr`.
   */
    [[nodiscard]] static constexpr Option<Slice> TryFromRaw(Pointer pointer, usize size) noexcept
    {
        if (size > 0 && pointer == nullptr) {
            return None;
        }
        return Option<Slice>(Slice(pointer, size, UncheckedTag{}));
    }

    /**
   * @brief Constructs from pointer + size without validation.
   * @warning Caller must ensure range validity.
   */
    [[nodiscard]] static constexpr Slice FromRawUnchecked(Pointer pointer, usize size) noexcept
    {
        return Slice(pointer, size, UncheckedTag{});
    }

    /// @brief Returns raw data pointer.
    [[nodiscard]] constexpr Pointer Data() const noexcept
    {
        return Ptr_;
    }

    [[nodiscard]] constexpr Pointer data() const noexcept
    {
        return Data();
    }

    /// @brief Returns number of elements in the view.
    [[nodiscard]] constexpr usize Size() const noexcept
    {
        return Size_;
    }

    [[nodiscard]] constexpr usize size() const noexcept
    {
        return Size();
    }

    /// @brief Returns `true` when the view is empty.
    [[nodiscard]] constexpr bool EmptyView() const noexcept
    {
        return Size_ == 0;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return EmptyView();
    }

    /// @brief Returns iterator to first element.
    [[nodiscard]] constexpr Iterator begin() const noexcept
    {
        return Ptr_;
    }

    /// @brief Returns iterator past last element.
    [[nodiscard]] constexpr Iterator end() const noexcept
    {
        return Size_ == 0 ? Ptr_ : Ptr_ + Size_;
    }

    /**
   * @brief Unchecked indexed access with debug assertion.
   * @param index Element index.
   * @return Reference to element at `index`.
   */
    [[nodiscard]] constexpr const ValueT& operator[](usize index) const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(index < Size_,
                               detail::ContractViolationCode::SLICE_INDEX_OUT_OF_BOUNDS,
                               "zcore::Slice::operator[] index out of bounds");
        return Ptr_[index];
    }

    /**
   * @brief Bounds-checked pointer access.
   * @param index Element index.
   * @return Pointer to element or `nullptr` if out-of-range.
   */
    [[nodiscard]] constexpr Pointer TryAt(usize index) const noexcept
    {
        return index < Size_ ? Ptr_ + index : nullptr;
    }

    /**
   * @brief Returns first `count` elements.
   * @return `None` if `count > Size()`.
   */
    [[nodiscard]] constexpr Option<Slice> First(usize count) const noexcept
    {
        if (count > Size_) {
            return None;
        }
        return Option<Slice>(Slice(Ptr_, count, UncheckedTag{}));
    }

    /**
   * @brief Returns last `count` elements.
   * @return `None` if `count > Size()`.
   */
    [[nodiscard]] constexpr Option<Slice> Last(usize count) const noexcept
    {
        if (count > Size_) {
            return None;
        }
        if (count == 0) {
            return Option<Slice>(Slice());
        }
        return Option<Slice>(Slice(Ptr_ + (Size_ - count), count, UncheckedTag{}));
    }

    /**
   * @brief Returns a subrange view.
   * @param offset Start offset.
   * @param count Number of elements.
   * @return `None` for invalid range.
   */
    [[nodiscard]] constexpr Option<Slice> Subslice(usize offset, usize count) const noexcept
    {
        if (offset > Size_ || count > (Size_ - offset)) {
            return None;
        }
        if (count == 0) {
            return Option<Slice>(Slice());
        }
        return Option<Slice>(Slice(Ptr_ + offset, count, UncheckedTag{}));
    }

    /**
   * @brief Removes `count` elements from the front.
   * @return `true` on success, `false` when `count > Size()`.
   */
    constexpr bool RemovePrefix(usize count) noexcept
    {
        if (count > Size_) {
            return false;
        }
        if (count == Size_) {
            Clear();
            return true;
        }
        if (count > 0) {
            Ptr_ += count;
            Size_ -= count;
        }
        return true;
    }

    /**
   * @brief Removes `count` elements from the back.
   * @return `true` on success, `false` when `count > Size()`.
   */
    constexpr bool RemoveSuffix(usize count) noexcept
    {
        if (count > Size_) {
            return false;
        }
        if (count == Size_) {
            Clear();
            return true;
        }
        Size_ -= count;
        return true;
    }

    /// @brief Resets to empty slice.
    constexpr void Clear() noexcept
    {
        Ptr_ = nullptr;
        Size_ = 0;
    }

    [[nodiscard]] constexpr bool operator==(const Slice&) const noexcept = default;

private:
    struct UncheckedTag final {};

    constexpr Slice(Pointer pointer, usize size, UncheckedTag) noexcept : Ptr_(pointer), Size_(size)
    {
    }

    static constexpr void EnsureValidRange(Pointer pointer, usize size) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(size == 0 || pointer != nullptr,
                               detail::ContractViolationCode::SLICE_INVALID_RANGE,
                               "zcore::Slice requires non-null pointer when size is non-zero");
    }

    Pointer Ptr_;
    usize Size_;
};

/**
 * @brief Mutable non-owning contiguous view.
 *
 * @tparam ValueT Mutable element type (non-const object type).
 */
template <typename ValueT>
class [[nodiscard("SliceMut must be handled explicitly.")]] SliceMut final {
public:
    ZCORE_STATIC_REQUIRE(
            constraints::ObjectType<ValueT>&& constraints::NonReferenceType<ValueT>&& constraints::NonConstType<ValueT>,
            "SliceMut<ValueT> requires mutable non-reference object element type.");

    using ElementType = ValueT;
    using Pointer = ValueT*;
    using Iterator = ValueT*;

    /// @brief Constructs an empty mutable slice.
    constexpr SliceMut() noexcept : Ptr_(nullptr), Size_(0)
    {
    }

    /**
   * @brief Constructs from pointer and length.
   * @param pointer Start address.
   * @param size Element count.
   *
   * Aborts if `size > 0` and `pointer == nullptr`.
   */
    constexpr SliceMut(Pointer pointer, usize size) noexcept : Ptr_(pointer), Size_(size)
    {
        EnsureValidRange(pointer, size);
    }

    template <usize ExtentV>
    constexpr explicit SliceMut(std::array<ValueT, ExtentV>& values) noexcept : Ptr_(values.data()), Size_(ExtentV)
    {
    }

    template <usize ExtentV>
    constexpr SliceMut(ValueT (&values)[ExtentV]) noexcept : Ptr_(values), Size_(ExtentV)
    {
    }

    constexpr explicit SliceMut(std::span<ValueT> values) noexcept : Ptr_(values.data()), Size_(values.size())
    {
    }

    constexpr SliceMut(const SliceMut&) noexcept = default;
    constexpr SliceMut& operator=(const SliceMut&) noexcept = default;
    constexpr SliceMut(SliceMut&&) noexcept = default;
    constexpr SliceMut& operator=(SliceMut&&) noexcept = default;
    ~SliceMut() = default;

    /// @brief Returns an empty mutable slice.
    [[nodiscard]] static constexpr SliceMut Empty() noexcept
    {
        return SliceMut();
    }

    /**
   * @brief Attempts construction from pointer + size.
   * @return `None` when `size > 0` and `pointer == nullptr`.
   */
    [[nodiscard]] static constexpr Option<SliceMut> TryFromRaw(Pointer pointer, usize size) noexcept
    {
        if (size > 0 && pointer == nullptr) {
            return None;
        }
        return Option<SliceMut>(SliceMut(pointer, size, UncheckedTag{}));
    }

    /**
   * @brief Constructs from pointer + size without validation.
   * @warning Caller must ensure range validity.
   */
    [[nodiscard]] static constexpr SliceMut FromRawUnchecked(Pointer pointer, usize size) noexcept
    {
        return SliceMut(pointer, size, UncheckedTag{});
    }

    /// @brief Returns raw mutable pointer.
    [[nodiscard]] constexpr Pointer Data() const noexcept
    {
        return Ptr_;
    }

    [[nodiscard]] constexpr Pointer data() const noexcept
    {
        return Data();
    }

    /// @brief Returns number of elements in the view.
    [[nodiscard]] constexpr usize Size() const noexcept
    {
        return Size_;
    }

    [[nodiscard]] constexpr usize size() const noexcept
    {
        return Size();
    }

    /// @brief Returns `true` when view is empty.
    [[nodiscard]] constexpr bool EmptyView() const noexcept
    {
        return Size_ == 0;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return EmptyView();
    }

    /// @brief Returns iterator to first element.
    [[nodiscard]] constexpr Iterator begin() const noexcept
    {
        return Ptr_;
    }

    /// @brief Returns iterator past last element.
    [[nodiscard]] constexpr Iterator end() const noexcept
    {
        return Size_ == 0 ? Ptr_ : Ptr_ + Size_;
    }

    /**
   * @brief Unchecked indexed access with debug assertion.
   * @param index Element index.
   * @return Mutable reference to element at `index`.
   */
    [[nodiscard]] constexpr ValueT& operator[](usize index) const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(index < Size_,
                               detail::ContractViolationCode::SLICE_INDEX_OUT_OF_BOUNDS,
                               "zcore::SliceMut::operator[] index out of bounds");
        return Ptr_[index];
    }

    /**
   * @brief Bounds-checked pointer access.
   * @param index Element index.
   * @return Pointer to element or `nullptr` if out-of-range.
   */
    [[nodiscard]] constexpr Pointer TryAt(usize index) const noexcept
    {
        return index < Size_ ? Ptr_ + index : nullptr;
    }

    /**
   * @brief Returns first `count` elements.
   * @return `None` if `count > Size()`.
   */
    [[nodiscard]] constexpr Option<SliceMut> First(usize count) const noexcept
    {
        if (count > Size_) {
            return None;
        }
        return Option<SliceMut>(SliceMut(Ptr_, count, UncheckedTag{}));
    }

    /**
   * @brief Returns last `count` elements.
   * @return `None` if `count > Size()`.
   */
    [[nodiscard]] constexpr Option<SliceMut> Last(usize count) const noexcept
    {
        if (count > Size_) {
            return None;
        }
        if (count == 0) {
            return Option<SliceMut>(SliceMut());
        }
        return Option<SliceMut>(SliceMut(Ptr_ + (Size_ - count), count, UncheckedTag{}));
    }

    /**
   * @brief Returns a mutable subrange view.
   * @param offset Start offset.
   * @param count Number of elements.
   * @return `None` for invalid range.
   */
    [[nodiscard]] constexpr Option<SliceMut> Subslice(usize offset, usize count) const noexcept
    {
        if (offset > Size_ || count > (Size_ - offset)) {
            return None;
        }
        if (count == 0) {
            return Option<SliceMut>(SliceMut());
        }
        return Option<SliceMut>(SliceMut(Ptr_ + offset, count, UncheckedTag{}));
    }

    /**
   * @brief Removes `count` elements from the front.
   * @return `true` on success, `false` when `count > Size()`.
   */
    constexpr bool RemovePrefix(usize count) noexcept
    {
        if (count > Size_) {
            return false;
        }
        if (count == Size_) {
            Clear();
            return true;
        }
        if (count > 0) {
            Ptr_ += count;
            Size_ -= count;
        }
        return true;
    }

    /**
   * @brief Removes `count` elements from the back.
   * @return `true` on success, `false` when `count > Size()`.
   */
    constexpr bool RemoveSuffix(usize count) noexcept
    {
        if (count > Size_) {
            return false;
        }
        if (count == Size_) {
            Clear();
            return true;
        }
        Size_ -= count;
        return true;
    }

    /// @brief Resets to empty mutable slice.
    constexpr void Clear() noexcept
    {
        Ptr_ = nullptr;
        Size_ = 0;
    }

    /// @brief Converts mutable view to immutable view.
    [[nodiscard]] constexpr Slice<const ValueT> AsConst() const noexcept
    {
        return Slice<const ValueT>::FromRawUnchecked(Ptr_, Size_);
    }

    [[nodiscard]] constexpr operator Slice<const ValueT>() const noexcept
    {
        return AsConst();
    }

    [[nodiscard]] constexpr bool operator==(const SliceMut&) const noexcept = default;

private:
    struct UncheckedTag final {};

    constexpr SliceMut(Pointer pointer, usize size, UncheckedTag) noexcept : Ptr_(pointer), Size_(size)
    {
    }

    static constexpr void EnsureValidRange(Pointer pointer, usize size) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(size == 0 || pointer != nullptr,
                               detail::ContractViolationCode::SLICE_INVALID_RANGE,
                               "zcore::SliceMut requires non-null pointer when size is non-zero");
    }

    Pointer Ptr_;
    usize Size_;
};

/// @brief Read-only byte slice alias.
using ByteSpan = Slice<const Byte>;
/// @brief Writable byte slice alias.
using ByteSpanMut = SliceMut<Byte>;

/**
 * @brief Reinterprets an immutable slice as bytes.
 * @param values Source values.
 * @return Byte view over object representation.
 */
template <typename ValueT>
[[nodiscard]] constexpr ByteSpan AsBytes(Slice<ValueT> values) noexcept
{
    if (values.Size() == 0) {
        return ByteSpan();
    }
    return ByteSpan::FromRawUnchecked(reinterpret_cast<const Byte*>(values.Data()), values.Size() * sizeof(ValueT));
}

/**
 * @brief Reinterprets a mutable slice as read-only bytes.
 * @param values Source values.
 * @return Read-only byte view over object representation.
 */
template <typename ValueT>
[[nodiscard]] constexpr ByteSpan AsBytes(SliceMut<ValueT> values) noexcept
{
    if (values.Size() == 0) {
        return ByteSpan();
    }
    return ByteSpan::FromRawUnchecked(reinterpret_cast<const Byte*>(values.Data()), values.Size() * sizeof(ValueT));
}

/**
 * @brief Reinterprets a mutable slice as writable bytes.
 * @param values Source values.
 * @return Writable byte view over object representation.
 */
template <typename ValueT>
[[nodiscard]] constexpr ByteSpanMut AsWritableBytes(SliceMut<ValueT> values) noexcept
{
    if (values.Size() == 0) {
        return ByteSpanMut();
    }
    return ByteSpanMut::FromRawUnchecked(reinterpret_cast<Byte*>(values.Data()), values.Size() * sizeof(ValueT));
}

} // namespace zcore
