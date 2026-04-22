/**************************************************************************/
/*  byte_buffer.hpp                                                       */
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
 * @file include/zcore/io/byte_buffer.hpp
 * @brief Allocator-backed growable owning byte storage.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/byte_buffer.hpp>
 * zcore::ByteBuffer buffer(allocator);
 * @endcode
 */

#pragma once

#include <zcore/allocator.hpp>
#include <zcore/contract_violation.hpp>
#include <zcore/foundation.hpp>
#include <zcore/option.hpp>
#include <zcore/result.hpp>
#include <zcore/slice.hpp>
#include <zcore/status.hpp>
#include <zcore/vector.hpp>

namespace zcore {

/**
 * @brief Allocator-backed growable owning byte container.
 */
class [[nodiscard("ByteBuffer must be handled explicitly.")]] ByteBuffer final {
public:
    using ValueType = Byte;
    using Pointer = Byte*;
    using ConstPointer = const Byte*;
    using Iterator = Byte*;
    using ConstIterator = const Byte*;

    /// @brief Constructs an empty unbound buffer.
    ByteBuffer() noexcept = default;

    /// @brief Constructs an empty allocator-bound buffer.
    explicit ByteBuffer(Allocator& allocator) noexcept : Bytes_(allocator)
    {
    }

    ByteBuffer(const ByteBuffer&) = delete;
    ByteBuffer& operator=(const ByteBuffer&) = delete;
    ByteBuffer(ByteBuffer&&) noexcept = default;
    ByteBuffer& operator=(ByteBuffer&&) noexcept = default;
    ~ByteBuffer() = default;

    /**
   * @brief Creates allocator-bound byte buffer with reserved capacity.
   */
    [[nodiscard]] static Result<ByteBuffer, Error> TryWithCapacity(Allocator& allocator, usize capacity) noexcept
    {
        ByteBuffer out(allocator);
        Status reserveStatus = out.TryReserve(capacity);
        if (reserveStatus.HasError()) {
            return Result<ByteBuffer, Error>::Failure(reserveStatus.Error());
        }
        return Result<ByteBuffer, Error>::Success(std::move(out));
    }

    /// @brief Returns whether allocator binding is present.
    [[nodiscard]] bool HasAllocator() const noexcept
    {
        return Bytes_.HasAllocator();
    }

    /// @brief Returns bound allocator pointer or null when unbound.
    [[nodiscard]] Allocator* AllocatorRef() const noexcept
    {
        return Bytes_.AllocatorRef();
    }

    /// @brief Returns byte count.
    [[nodiscard]] usize Size() const noexcept
    {
        return Bytes_.Size();
    }

    [[nodiscard]] usize size() const noexcept
    {
        return Size();
    }

    /// @brief Returns byte capacity.
    [[nodiscard]] usize Capacity() const noexcept
    {
        return Bytes_.Capacity();
    }

    /// @brief Returns remaining byte capacity.
    [[nodiscard]] usize RemainingCapacity() const noexcept
    {
        return Bytes_.RemainingCapacity();
    }

    /// @brief Returns `true` when empty.
    [[nodiscard]] bool Empty() const noexcept
    {
        return Bytes_.Empty();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return Empty();
    }

    /// @brief Returns mutable pointer to first byte or `nullptr` when empty.
    [[nodiscard]] Pointer Data() noexcept
    {
        return Bytes_.Data();
    }

    /// @brief Returns const pointer to first byte or `nullptr` when empty.
    [[nodiscard]] ConstPointer Data() const noexcept
    {
        return Bytes_.Data();
    }

    [[nodiscard]] Pointer data() noexcept
    {
        return Data();
    }

    [[nodiscard]] ConstPointer data() const noexcept
    {
        return Data();
    }

    /// @brief Returns iterator to first byte.
    [[nodiscard]] Iterator begin() noexcept
    {
        return Bytes_.begin();
    }

    /// @brief Returns iterator past last byte.
    [[nodiscard]] Iterator end() noexcept
    {
        return Bytes_.end();
    }

    /// @brief Returns const iterator to first byte.
    [[nodiscard]] ConstIterator begin() const noexcept
    {
        return Bytes_.begin();
    }

    /// @brief Returns const iterator past last byte.
    [[nodiscard]] ConstIterator end() const noexcept
    {
        return Bytes_.end();
    }

    [[nodiscard]] ConstIterator cbegin() const noexcept
    {
        return Bytes_.cbegin();
    }

    [[nodiscard]] ConstIterator cend() const noexcept
    {
        return Bytes_.cend();
    }

    /**
   * @brief Checked indexed mutable access.
   * @pre `index < Size()`.
   */
    [[nodiscard]] Byte& operator[](usize index) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(index < Size(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::ByteBuffer::operator[] index out of bounds");
        return Bytes_[index];
    }

    /**
   * @brief Checked indexed const access.
   * @pre `index < Size()`.
   */
    [[nodiscard]] Byte operator[](usize index) const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(index < Size(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::ByteBuffer::operator[] index out of bounds");
        return Bytes_[index];
    }

    /// @brief Returns pointer to byte at `index` or `nullptr` when out-of-range.
    [[nodiscard]] Pointer TryAt(usize index) noexcept
    {
        return Bytes_.TryAt(index);
    }

    /// @brief Returns pointer to byte at `index` or `nullptr` when out-of-range.
    [[nodiscard]] ConstPointer TryAt(usize index) const noexcept
    {
        return Bytes_.TryAt(index);
    }

    /**
   * @brief Returns first byte.
   * @pre `!Empty()`.
   */
    [[nodiscard]] Byte& Front() noexcept
    {
        return Bytes_.Front();
    }

    /**
   * @brief Returns first byte.
   * @pre `!Empty()`.
   */
    [[nodiscard]] Byte Front() const noexcept
    {
        return Bytes_.Front();
    }

    /**
   * @brief Returns last byte.
   * @pre `!Empty()`.
   */
    [[nodiscard]] Byte& Back() noexcept
    {
        return Bytes_.Back();
    }

    /**
   * @brief Returns last byte.
   * @pre `!Empty()`.
   */
    [[nodiscard]] Byte Back() const noexcept
    {
        return Bytes_.Back();
    }

    /**
   * @brief Ensures capacity for at least `capacity` bytes.
   * @return Success or allocator-domain error.
   */
    [[nodiscard]] Status TryReserve(usize capacity) noexcept
    {
        return Bytes_.TryReserve(capacity);
    }

    /**
   * @brief Attempts to resize buffer.
   * @param size Target size.
   * @param fillValue Fill byte used when growing.
   * @return Success or allocator-domain error.
   */
    [[nodiscard]] Status TryResize(usize size, Byte fillValue = Byte{0U}) noexcept
    {
        if (size <= Size()) {
            while (Size() > size) {
                static_cast<void>(Bytes_.TryPopBack());
            }
            return OkStatus();
        }

        Status reserveStatus = Bytes_.TryReserve(size);
        if (reserveStatus.HasError()) {
            return reserveStatus;
        }
        while (Size() < size) {
            const Status pushStatus = Bytes_.TryPushBack(fillValue);
            if (pushStatus.HasError()) {
                return pushStatus;
            }
        }
        return OkStatus();
    }

    /**
   * @brief Attempts assignment from byte view.
   * @return Success or allocator-domain error.
   */
    [[nodiscard]] Status TryAssign(ByteSpan value) noexcept
    {
        if (value.EmptyView()) {
            Bytes_.Clear();
            return OkStatus();
        }

        if (!HasAllocator()) {
            return ErrorStatus(
                    MakeAllocatorError(AllocatorErrorCode::UNSUPPORTED_OPERATION, "assign", "byte buffer has no bound allocator"));
        }

        auto nextResult = Vector<Byte>::TryWithCapacity(*AllocatorRef(), value.Size());
        if (nextResult.HasError()) {
            return ErrorStatus(nextResult.Error());
        }
        Vector<Byte> next = std::move(nextResult.Value());
        for (usize index = 0U; index < value.Size(); ++index) {
            const Status pushStatus = next.TryPushBack(value[index]);
            if (pushStatus.HasError()) {
                return pushStatus;
            }
        }
        Bytes_ = std::move(next);
        return OkStatus();
    }

    /**
   * @brief Attempts append from byte view.
   * @return Success or allocator-domain error.
   */
    [[nodiscard]] Status TryAppend(ByteSpan value) noexcept
    {
        if (value.EmptyView()) {
            return OkStatus();
        }
        Status reserveStatus = Bytes_.TryReserve(Size() + value.Size());
        if (reserveStatus.HasError()) {
            return reserveStatus;
        }
        for (usize index = 0U; index < value.Size(); ++index) {
            const Status pushStatus = Bytes_.TryPushBack(value[index]);
            if (pushStatus.HasError()) {
                return pushStatus;
            }
        }
        return OkStatus();
    }

    /**
   * @brief Attempts to append one byte.
   * @return Success or allocator-domain error.
   */
    [[nodiscard]] Status TryPushBack(Byte value) noexcept
    {
        return Bytes_.TryPushBack(value);
    }

    /**
   * @brief Attempts to remove last byte.
   * @return `true` on success, `false` when empty.
   */
    [[nodiscard]] bool TryPopBack() noexcept
    {
        return Bytes_.TryPopBack();
    }

    /**
   * @brief Attempts to move out last byte.
   * @return `None` when empty.
   */
    [[nodiscard]] Option<Byte> TryPopBackValue() noexcept
    {
        return Bytes_.TryPopBackValue();
    }

    /// @brief Clears bytes while retaining capacity.
    void Clear() noexcept
    {
        Bytes_.Clear();
    }

    /// @brief Clears and deallocates owned storage.
    void Reset() noexcept
    {
        Bytes_.Reset();
    }

    /// @brief Returns immutable byte view.
    [[nodiscard]] ByteSpan AsBytes() const noexcept
    {
        if (Empty()) {
            return ByteSpan::Empty();
        }
        return ByteSpan::FromRawUnchecked(Data(), Size());
    }

    /// @brief Returns mutable byte view.
    [[nodiscard]] ByteSpanMut AsBytesMut() noexcept
    {
        if (Empty()) {
            return ByteSpanMut::Empty();
        }
        return ByteSpanMut::FromRawUnchecked(Data(), Size());
    }

    [[nodiscard]] operator ByteSpan() const noexcept
    {
        return AsBytes();
    }

private:
    Vector<Byte> Bytes_;
};

} // namespace zcore
