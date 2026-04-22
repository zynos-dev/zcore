/**************************************************************************/
/*  inline_buffer.hpp                                                     */
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
 * @file include/zcore/container/inline_buffer.hpp
 * @brief Fixed-capacity owning byte buffer with no heap allocation.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/inline_buffer.hpp>
 * zcore::InlineBuffer<32> buffer;
 * buffer.TryPushBack(static_cast<zcore::Byte>(0x2AU));
 * @endcode
 */

#pragma once

#include <zcore/contract_violation.hpp>
#include <zcore/fixed_vector.hpp>
#include <zcore/foundation.hpp>
#include <zcore/option.hpp>
#include <zcore/slice.hpp>

namespace zcore {

/**
 * @brief Deterministic fixed-capacity owning byte buffer.
 *
 * @tparam CapacityV Maximum byte capacity.
 */
template <usize CapacityV>
class [[nodiscard("InlineBuffer must be handled explicitly.")]] InlineBuffer final {
public:
    using ValueType = Byte;
    using SizeType = usize;
    using Pointer = Byte*;
    using ConstPointer = const Byte*;
    using Iterator = Byte*;
    using ConstIterator = const Byte*;

    /// @brief Compile-time capacity bound.
    static constexpr usize kCapacity = CapacityV;

    constexpr InlineBuffer() noexcept = default;
    constexpr InlineBuffer(const InlineBuffer&) = default;
    constexpr InlineBuffer& operator=(const InlineBuffer&) = default;
    constexpr InlineBuffer(InlineBuffer&&) noexcept = default;
    constexpr InlineBuffer& operator=(InlineBuffer&&) noexcept = default;
    ~InlineBuffer() = default;

    /// @brief Returns compile-time capacity.
    [[nodiscard]] static constexpr usize Capacity() noexcept
    {
        return kCapacity;
    }

    /// @brief Returns current byte count.
    [[nodiscard]] constexpr usize Size() const noexcept
    {
        return Bytes_.Size();
    }

    [[nodiscard]] constexpr usize size() const noexcept
    {
        return Size();
    }

    /// @brief Returns remaining insertion capacity.
    [[nodiscard]] constexpr usize RemainingCapacity() const noexcept
    {
        return Bytes_.RemainingCapacity();
    }

    /// @brief Returns true when buffer has no bytes.
    [[nodiscard]] constexpr bool Empty() const noexcept
    {
        return Bytes_.Empty();
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return Empty();
    }

    /// @brief Returns true when `Size() == Capacity()`.
    [[nodiscard]] constexpr bool Full() const noexcept
    {
        return Bytes_.Full();
    }

    /// @brief Returns mutable byte pointer, or null when empty.
    [[nodiscard]] constexpr Pointer Data() noexcept
    {
        return Bytes_.Data();
    }

    /// @brief Returns const byte pointer, or null when empty.
    [[nodiscard]] constexpr ConstPointer Data() const noexcept
    {
        return Bytes_.Data();
    }

    [[nodiscard]] constexpr Pointer data() noexcept
    {
        return Data();
    }

    [[nodiscard]] constexpr ConstPointer data() const noexcept
    {
        return Data();
    }

    /// @brief Returns mutable iterator to first byte.
    [[nodiscard]] constexpr Iterator begin() noexcept
    {
        return Bytes_.begin();
    }

    /// @brief Returns mutable iterator past last byte.
    [[nodiscard]] constexpr Iterator end() noexcept
    {
        return Bytes_.end();
    }

    /// @brief Returns const iterator to first byte.
    [[nodiscard]] constexpr ConstIterator begin() const noexcept
    {
        return Bytes_.begin();
    }

    /// @brief Returns const iterator past last byte.
    [[nodiscard]] constexpr ConstIterator end() const noexcept
    {
        return Bytes_.end();
    }

    [[nodiscard]] constexpr ConstIterator cbegin() const noexcept
    {
        return Bytes_.cbegin();
    }

    [[nodiscard]] constexpr ConstIterator cend() const noexcept
    {
        return Bytes_.cend();
    }

    /**
   * @brief Checked indexed mutable access.
   * @pre `index < Size()`.
   */
    [[nodiscard]] constexpr Byte& operator[](usize index) noexcept
    {
        return Bytes_[index];
    }

    /**
   * @brief Checked indexed const access.
   * @pre `index < Size()`.
   */
    [[nodiscard]] constexpr Byte operator[](usize index) const noexcept
    {
        return Bytes_[index];
    }

    /// @brief Returns pointer to byte at `index` or null when out-of-range.
    [[nodiscard]] constexpr Pointer TryAt(usize index) noexcept
    {
        return Bytes_.TryAt(index);
    }

    /// @brief Returns pointer to byte at `index` or null when out-of-range.
    [[nodiscard]] constexpr ConstPointer TryAt(usize index) const noexcept
    {
        return Bytes_.TryAt(index);
    }

    /**
   * @brief Returns first byte.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr Byte& Front() noexcept
    {
        return Bytes_.Front();
    }

    /**
   * @brief Returns first byte.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr Byte Front() const noexcept
    {
        return Bytes_.Front();
    }

    /**
   * @brief Returns last byte.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr Byte& Back() noexcept
    {
        return Bytes_.Back();
    }

    /**
   * @brief Returns last byte.
   * @pre `!Empty()`.
   */
    [[nodiscard]] constexpr Byte Back() const noexcept
    {
        return Bytes_.Back();
    }

    /**
   * @brief Attempts assignment from byte view.
   * @return `true` on success, `false` when source exceeds fixed capacity.
   */
    [[nodiscard]] constexpr bool TryAssign(ByteSpan value) noexcept
    {
        if (value.Size() > kCapacity) {
            return false;
        }

        Bytes_.Clear();
        for (usize index = 0U; index < value.Size(); ++index) {
            const bool appended = Bytes_.TryPushBack(value[index]);
            if (!appended) {
                return false;
            }
        }
        return true;
    }

    /**
   * @brief Assigns from byte view.
   * @pre `value.Size() <= Capacity()`.
   */
    constexpr void Assign(ByteSpan value) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(TryAssign(value),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::InlineBuffer::Assign() source exceeds fixed capacity");
    }

    /**
   * @brief Attempts append from byte view.
   * @return `true` on success, `false` when append exceeds fixed capacity.
   */
    [[nodiscard]] constexpr bool TryAppend(ByteSpan value) noexcept
    {
        if (value.Size() > RemainingCapacity()) {
            return false;
        }

        for (usize index = 0U; index < value.Size(); ++index) {
            const bool appended = Bytes_.TryPushBack(value[index]);
            if (!appended) {
                return false;
            }
        }
        return true;
    }

    /**
   * @brief Appends from byte view.
   * @pre `value.Size() <= RemainingCapacity()`.
   */
    constexpr void Append(ByteSpan value) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(TryAppend(value),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::InlineBuffer::Append() exceeds fixed capacity");
    }

    /**
   * @brief Attempts to resize byte count.
   * @param size Target size.
   * @param fillValue Fill byte used when growing.
   * @return `true` on success, `false` when target size exceeds fixed capacity.
   */
    [[nodiscard]] constexpr bool TryResize(usize size, Byte fillValue = Byte{0U}) noexcept
    {
        if (size > kCapacity) {
            return false;
        }
        while (Size() > size) {
            static_cast<void>(Bytes_.TryPopBack());
        }
        while (Size() < size) {
            const bool appended = Bytes_.TryPushBack(fillValue);
            if (!appended) {
                return false;
            }
        }
        return true;
    }

    /**
   * @brief Resizes byte count.
   * @pre `size <= Capacity()`.
   */
    constexpr void Resize(usize size, Byte fillValue = Byte{0U}) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(TryResize(size, fillValue),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::InlineBuffer::Resize() exceeds fixed capacity");
    }

    /**
   * @brief Attempts to append one byte.
   * @return `true` on success, `false` when full.
   */
    [[nodiscard]] constexpr bool TryPushBack(Byte value) noexcept
    {
        return Bytes_.TryPushBack(value);
    }

    /**
   * @brief Appends one byte.
   * @pre `!Full()`.
   */
    constexpr void PushBack(Byte value) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(TryPushBack(value),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::InlineBuffer::PushBack() exceeds fixed capacity");
    }

    /**
   * @brief Attempts to remove last byte.
   * @return `true` on success, `false` when empty.
   */
    [[nodiscard]] constexpr bool TryPopBack() noexcept
    {
        return Bytes_.TryPopBack();
    }

    /**
   * @brief Removes last byte.
   * @pre `!Empty()`.
   */
    constexpr void PopBack() noexcept
    {
        ZCORE_CONTRACT_REQUIRE(TryPopBack(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::InlineBuffer::PopBack() requires non-empty buffer");
    }

    /**
   * @brief Attempts to move out last byte.
   * @return `None` when empty.
   */
    [[nodiscard]] constexpr Option<Byte> TryPopBackValue() noexcept
    {
        return Bytes_.TryPopBackValue();
    }

    /// @brief Clears bytes and retains fixed capacity.
    constexpr void Clear() noexcept
    {
        Bytes_.Clear();
    }

    /// @brief Returns immutable byte view.
    [[nodiscard]] constexpr ByteSpan AsBytes() const noexcept
    {
        return Bytes_.AsSlice();
    }

    /// @brief Returns mutable byte view.
    [[nodiscard]] constexpr ByteSpanMut AsBytesMut() noexcept
    {
        return Bytes_.AsSliceMut();
    }

    [[nodiscard]] constexpr operator ByteSpan() const noexcept
    {
        return AsBytes();
    }

    [[nodiscard]] constexpr bool operator==(const InlineBuffer& other) const noexcept = default;

private:
    FixedVector<Byte, kCapacity> Bytes_;
};

} // namespace zcore
