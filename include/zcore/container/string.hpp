/**************************************************************************/
/*  string.hpp                                                            */
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
 * @file include/zcore/container/string.hpp
 * @brief Allocator-backed UTF-8 owning string.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/string.hpp>
 * zcore::String value(allocator);
 * @endcode
 */

#pragma once

#include <cstring>
#include <string_view>
#include <zcore/allocator.hpp>
#include <zcore/contract_violation.hpp>
#include <zcore/foundation.hpp>
#include <zcore/option.hpp>
#include <zcore/result.hpp>
#include <zcore/string_view.hpp>
#include <zcore/utf8.hpp>
#include <zcore/vector.hpp>

namespace zcore {

/**
 * @brief String-domain error codes for allocator-backed UTF-8 string APIs.
 */
// NOLINTNEXTLINE(performance-enum-size): stable error-code ABI uses i32 payloads.
enum class StringErrorCode : i32 {
    /// @brief Input pointer/range argument is invalid.
    INVALID_ARGUMENT = 1,
    /// @brief Input UTF-8 sequence or Unicode scalar value is invalid.
    INVALID_UTF8 = 2,
};

/// @brief Built-in string error domain identifier.
inline constexpr ErrorDomain kStringErrorDomain{
        .id = 3U,
        .name = "string",
};

/**
 * @brief Constructs string-domain error payload.
 */
[[nodiscard]] constexpr Error
MakeStringError(StringErrorCode code, const char* operation, const char* message, const char* file = "", u32 line = 0U) noexcept
{
    return MakeError(kStringErrorDomain, static_cast<i32>(code), MakeErrorContext("string", operation, message, file, line));
}

/**
 * @brief Allocator-backed owning UTF-8 string.
 *
 * `String` stores UTF-8 bytes in `Vector<char>` and keeps a trailing null
 * terminator for non-empty state (`c_str()` compatibility).
 *
 * Compatibility byte-window mutation APIs can split code points; call
 * `IsValidUtf8()` when mixing byte APIs with UTF-8 code-point APIs.
 */
class [[nodiscard("String must be handled explicitly.")]] String final {
public:
    using ValueType = char;
    using Pointer = char*;
    using ConstPointer = const char*;
    using Iterator = char*;
    using ConstIterator = const char*;

    /// @brief Constructs an empty unbound string.
    String() noexcept = default;

    /// @brief Constructs an empty allocator-bound string.
    explicit String(Allocator& allocator) noexcept : Bytes_(allocator)
    {
    }

    String(const String&) = delete;
    String& operator=(const String&) = delete;
    String(String&&) noexcept = default;
    String& operator=(String&&) noexcept = default;
    ~String() = default;

    /**
   * @brief Creates allocator-bound empty string with reserved text capacity.
   * @param allocator Allocator used for storage.
   * @param capacity Requested UTF-8 byte capacity (excluding null terminator).
   */
    [[nodiscard]] static Result<String, Error> TryWithCapacity(Allocator& allocator, usize capacity) noexcept
    {
        String out(allocator);
        const Status reserveStatus = out.TryReserve(capacity);
        if (reserveStatus.HasError()) {
            return Result<String, Error>::Failure(reserveStatus.Error());
        }
        return Result<String, Error>::Success(std::move(out));
    }

    /**
   * @brief Creates allocator-bound string from validated UTF-8 view.
   * @param allocator Allocator used for storage.
   * @param value Source UTF-8 text.
   */
    [[nodiscard]] static Result<String, Error> TryFromStringView(Allocator& allocator, StringView value) noexcept
    {
        String out(allocator);
        const Status assignStatus = out.TryAssign(value);
        if (assignStatus.HasError()) {
            return Result<String, Error>::Failure(assignStatus.Error());
        }
        return Result<String, Error>::Success(std::move(out));
    }

    /**
   * @brief Creates allocator-bound string from UTF-8 C-string.
   * @param allocator Allocator used for storage.
   * @param value Source C-string.
   */
    [[nodiscard]] static Result<String, Error> TryFromCString(Allocator& allocator, ConstPointer value) noexcept
    {
        String out(allocator);
        const Status assignStatus = out.TryAssignCString(value);
        if (assignStatus.HasError()) {
            return Result<String, Error>::Failure(assignStatus.Error());
        }
        return Result<String, Error>::Success(std::move(out));
    }

    /**
   * @brief Creates allocator-bound string from UTF-8 C-string.
   * @param allocator Allocator used for storage.
   * @param value Source C-string.
   */
    [[nodiscard]] static Result<String, Error> TryFromUtf8CString(Allocator& allocator, ConstPointer value) noexcept
    {
        return TryFromCString(allocator, value);
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

    /// @brief Returns UTF-8 byte length (excluding trailing null terminator).
    [[nodiscard]] usize Size() const noexcept
    {
        return Bytes_.Size();
    }

    [[nodiscard]] usize size() const noexcept
    {
        return Size();
    }

    /// @brief Returns text byte capacity (excluding reserved null-terminator slot).
    [[nodiscard]] usize Capacity() const noexcept
    {
        if (Bytes_.Capacity() == 0U) {
            return 0U;
        }
        return Bytes_.Capacity() - 1U;
    }

    /// @brief Returns remaining UTF-8 byte capacity before growth is required.
    [[nodiscard]] usize RemainingCapacity() const noexcept
    {
        return Capacity() - Size();
    }

    /// @brief Returns `true` when no bytes are stored.
    [[nodiscard]] bool Empty() const noexcept
    {
        return Bytes_.Empty();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return Empty();
    }

    /// @brief Returns pointer to first UTF-8 byte or `nullptr` when empty.
    [[nodiscard]] Pointer Data() noexcept
    {
        return Bytes_.Data();
    }

    /// @brief Returns pointer to first UTF-8 byte or `nullptr` when empty.
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

    /// @brief Returns null-terminated C-string pointer.
    [[nodiscard]] ConstPointer CStr() const noexcept
    {
        if (Empty()) {
            return "";
        }
        return Data();
    }

    [[nodiscard]] ConstPointer c_str() const noexcept
    { // NOLINT(readability-identifier-naming)
        return CStr();
    }

    /// @brief Returns iterator to first byte.
    [[nodiscard]] Iterator begin() noexcept
    {
        return Data();
    }

    /// @brief Returns iterator past last byte.
    [[nodiscard]] Iterator end() noexcept
    {
        Iterator first = begin();
        return first == nullptr ? nullptr : first + Size();
    }

    /// @brief Returns const iterator to first byte.
    [[nodiscard]] ConstIterator begin() const noexcept
    {
        return Data();
    }

    /// @brief Returns const iterator past last byte.
    [[nodiscard]] ConstIterator end() const noexcept
    {
        ConstIterator first = begin();
        return first == nullptr ? nullptr : first + Size();
    }

    [[nodiscard]] ConstIterator cbegin() const noexcept
    {
        return begin();
    }

    [[nodiscard]] ConstIterator cend() const noexcept
    {
        return end();
    }

    /**
   * @brief Checked indexed mutable access.
   * @pre `index < Size()`.
   */
    [[nodiscard]] char& operator[](usize index) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(index < Size(),
                               detail::ContractViolationCode::FIXED_STRING_INDEX_OUT_OF_BOUNDS,
                               "zcore::String::operator[] index out of bounds");
        return Bytes_[index];
    }

    /**
   * @brief Checked indexed const access.
   * @pre `index < Size()`.
   */
    [[nodiscard]] char operator[](usize index) const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(index < Size(),
                               detail::ContractViolationCode::FIXED_STRING_INDEX_OUT_OF_BOUNDS,
                               "zcore::String::operator[] index out of bounds");
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
    [[nodiscard]] char& Front() noexcept
    {
        ZCORE_CONTRACT_REQUIRE(!Empty(),
                               detail::ContractViolationCode::FIXED_STRING_INDEX_OUT_OF_BOUNDS,
                               "zcore::String::Front() requires non-empty string");
        return Bytes_[0];
    }

    /**
   * @brief Returns first byte.
   * @pre `!Empty()`.
   */
    [[nodiscard]] char Front() const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(!Empty(),
                               detail::ContractViolationCode::FIXED_STRING_INDEX_OUT_OF_BOUNDS,
                               "zcore::String::Front() requires non-empty string");
        return Bytes_[0];
    }

    /**
   * @brief Returns last byte.
   * @pre `!Empty()`.
   */
    [[nodiscard]] char& Back() noexcept
    {
        ZCORE_CONTRACT_REQUIRE(!Empty(),
                               detail::ContractViolationCode::FIXED_STRING_INDEX_OUT_OF_BOUNDS,
                               "zcore::String::Back() requires non-empty string");
        return Bytes_[Size() - 1U];
    }

    /**
   * @brief Returns last byte.
   * @pre `!Empty()`.
   */
    [[nodiscard]] char Back() const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(!Empty(),
                               detail::ContractViolationCode::FIXED_STRING_INDEX_OUT_OF_BOUNDS,
                               "zcore::String::Back() requires non-empty string");
        return Bytes_[Size() - 1U];
    }

    /**
   * @brief Ensures text byte capacity is at least `capacity`.
   * @return Success or allocator/string-domain error.
   */
    [[nodiscard]] Status TryReserve(usize capacity) noexcept
    {
        if (capacity <= Capacity()) {
            return OkStatus();
        }
        Status reserveStatus = Bytes_.TryReserve(capacity + 1U);
        if (reserveStatus.HasError()) {
            return reserveStatus;
        }
        EnsureNullTerminator();
        return OkStatus();
    }

    /**
   * @brief Attempts assignment from UTF-8 text.
   * @return Success or allocator/string-domain error.
   */
    [[nodiscard]] Status TryAssign(StringView value) noexcept
    {
        if (!value.IsValidUtf8()) {
            return ErrorStatus(MakeStringError(StringErrorCode::INVALID_UTF8, "assign", "input must be valid UTF-8"));
        }

        if (value.EmptyView()) {
            Bytes_.Clear();
            return OkStatus();
        }

        Result<Vector<char>, Error> next = BuildFromView(value);
        if (next.HasError()) {
            return ErrorStatus(next.Error());
        }
        Bytes_ = std::move(next.Value());
        return OkStatus();
    }

    /**
   * @brief Attempts assignment from UTF-8 text.
   * @return Success or allocator/string-domain error.
   */
    [[nodiscard]] Status TryAssignUtf8(StringView value) noexcept
    {
        return TryAssign(value);
    }

    /**
   * @brief Attempts assignment from UTF-8 C-string.
   * @return Success or allocator/string-domain error.
   */
    [[nodiscard]] Status TryAssignCString(ConstPointer value) noexcept
    {
        if (value == nullptr) {
            return ErrorStatus(
                    MakeStringError(StringErrorCode::INVALID_ARGUMENT, "assign_cstring", "c-string pointer must be non-null"));
        }
        Option<StringView> maybe = StringView::TryFromCString(value);
        if (!maybe.HasValue()) {
            return ErrorStatus(MakeStringError(StringErrorCode::INVALID_UTF8, "assign_cstring", "c-string must be valid UTF-8"));
        }
        return TryAssign(maybe.Value());
    }

    /**
   * @brief Attempts assignment from UTF-8 C-string.
   * @return Success or allocator/string-domain error.
   */
    [[nodiscard]] Status TryAssignUtf8CString(ConstPointer value) noexcept
    {
        return TryAssignCString(value);
    }

    /**
   * @brief Attempts ASCII byte append.
   * @return Success or allocator/string-domain error.
   */
    [[nodiscard]] Status TryPushBack(char value) noexcept
    {
        if (static_cast<unsigned char>(value) > 0x7FU) {
            return ErrorStatus(
                    MakeStringError(StringErrorCode::INVALID_UTF8, "push_back", "single-byte append accepts ASCII only"));
        }
        char text[1] = {value};
        return TryAppend(StringView::FromRawUnchecked(text, 1U));
    }

    /**
   * @brief Attempts append of one Unicode scalar value.
   * @return Success or allocator/string-domain error.
   */
    [[nodiscard]] Status TryAppendCodePoint(u32 value) noexcept
    {
        char encoded[4] = {};
        Option<usize> width = utf8::EncodeCodePoint(value, encoded, sizeof(encoded));
        if (!width.HasValue()) {
            return ErrorStatus(
                    MakeStringError(StringErrorCode::INVALID_UTF8, "append_code_point", "value must be valid Unicode scalar"));
        }
        return TryAppend(StringView::FromRawUnchecked(encoded, width.Value()));
    }

    /**
   * @brief Attempts append from UTF-8 text.
   * @return Success or allocator/string-domain error.
   */
    [[nodiscard]] Status TryAppend(StringView value) noexcept
    {
        if (!value.IsValidUtf8()) {
            return ErrorStatus(MakeStringError(StringErrorCode::INVALID_UTF8, "append", "input must be valid UTF-8"));
        }
        if (value.EmptyView()) {
            return OkStatus();
        }

        Result<Vector<char>, Error> next = BuildFromTwoViews(AsStringView(), value);
        if (next.HasError()) {
            return ErrorStatus(next.Error());
        }
        Bytes_ = std::move(next.Value());
        return OkStatus();
    }

    /**
   * @brief Attempts append from UTF-8 text.
   * @return Success or allocator/string-domain error.
   */
    [[nodiscard]] Status TryAppendUtf8(StringView value) noexcept
    {
        return TryAppend(value);
    }

    /**
   * @brief Attempts to remove last UTF-8 code point.
   * @return `true` on success, `false` when empty or invalid UTF-8.
   */
    [[nodiscard]] bool TryPopBack() noexcept
    {
        if (Empty()) {
            return false;
        }
        Option<usize> totalCodePoints = utf8::CountCodePoints(Data(), Size());
        if (!totalCodePoints.HasValue() || totalCodePoints.Value() == 0U) {
            return false;
        }
        Option<usize> newSize = utf8::AdvanceCodePoints(Data(), Size(), 0U, totalCodePoints.Value() - 1U);
        if (!newSize.HasValue()) {
            return false;
        }
        return RemoveSuffixBytes(Size() - newSize.Value());
    }

    /**
   * @brief Removes `count` UTF-8 code points from front.
   * @return `true` on success, `false` when count exceeds available code points or UTF-8 is invalid.
   */
    [[nodiscard]] bool RemovePrefix(usize count) noexcept
    {
        Option<usize> bytesToDrop = utf8::AdvanceCodePoints(Data(), Size(), 0U, count);
        if (!bytesToDrop.HasValue()) {
            return false;
        }
        return RemovePrefixBytes(bytesToDrop.Value());
    }

    /**
   * @brief Removes `count` bytes from front.
   * @return `true` on success, `false` when `count > Size()`.
   */
    [[nodiscard]] bool RemovePrefixBytes(usize count) noexcept
    {
        if (count > Size()) {
            return false;
        }
        if (count == 0U) {
            return true;
        }
        if (count == Size()) {
            Clear();
            return true;
        }

        Pointer data = Data();
        const usize nextSize = Size() - count;
        std::memmove(data, data + count, nextSize);
        for (usize index = 0U; index < count; ++index) {
            const bool popped = Bytes_.TryPopBack();
            ZCORE_CONTRACT_REQUIRE(popped,
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::String::RemovePrefixBytes() expected pop to succeed");
        }
        EnsureNullTerminator();
        return true;
    }

    /**
   * @brief Removes `count` UTF-8 code points from back.
   * @return `true` on success, `false` when count exceeds available code points or UTF-8 is invalid.
   */
    [[nodiscard]] bool RemoveSuffix(usize count) noexcept
    {
        Option<usize> totalCodePoints = utf8::CountCodePoints(Data(), Size());
        if (!totalCodePoints.HasValue() || count > totalCodePoints.Value()) {
            return false;
        }
        const usize remainingCodePoints = totalCodePoints.Value() - count;
        Option<usize> newSize = utf8::AdvanceCodePoints(Data(), Size(), 0U, remainingCodePoints);
        if (!newSize.HasValue()) {
            return false;
        }
        return RemoveSuffixBytes(Size() - newSize.Value());
    }

    /**
   * @brief Removes `count` bytes from back.
   * @return `true` on success, `false` when `count > Size()`.
   */
    [[nodiscard]] bool RemoveSuffixBytes(usize count) noexcept
    {
        if (count > Size()) {
            return false;
        }
        for (usize index = 0U; index < count; ++index) {
            const bool popped = Bytes_.TryPopBack();
            ZCORE_CONTRACT_REQUIRE(popped,
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::String::RemoveSuffixBytes() expected pop to succeed");
        }
        EnsureNullTerminator();
        return true;
    }

    /// @brief Clears string to empty state (capacity retained).
    void Clear() noexcept
    {
        Bytes_.Clear();
    }

    /// @brief Destroys string bytes and releases owned allocation.
    void Reset() noexcept
    {
        Bytes_.Reset();
    }

    /// @brief Returns view over current bytes (unchecked UTF-8 validity).
    [[nodiscard]] StringView AsStringView() const noexcept
    {
        if (Empty()) {
            return StringView::Empty();
        }
        return StringView::FromRawUnchecked(Data(), Size());
    }

    /// @brief Returns `std::string_view` over current bytes.
    [[nodiscard]] std::string_view AsStdStringView() const noexcept
    {
        return std::string_view(CStr(), Size());
    }

    /// @brief Returns `true` when current bytes are valid UTF-8.
    [[nodiscard]] bool IsValidUtf8() const noexcept
    {
        return utf8::IsValid(Data(), Size());
    }

    /**
   * @brief Counts Unicode scalar values in current bytes.
   * @return `None` when bytes are invalid UTF-8.
   */
    [[nodiscard]] Option<usize> TryCodePointCount() const noexcept
    {
        return utf8::CountCodePoints(Data(), Size());
    }

    /**
   * @brief Returns code point count.
   * @pre `IsValidUtf8()`.
   */
    [[nodiscard]] usize CodePointCount() const noexcept
    {
        Option<usize> count = TryCodePointCount();
        ZCORE_CONTRACT_REQUIRE(count.HasValue(),
                               detail::ContractViolationCode::UTF8_INVALID_SEQUENCE,
                               "zcore::String::CodePointCount() requires valid UTF-8");
        return count.Value();
    }

    [[nodiscard]] operator StringView() const noexcept
    {
        return AsStringView();
    }

    [[nodiscard]] operator std::string_view() const noexcept
    {
        return AsStdStringView();
    }

    [[nodiscard]] bool operator==(const String& other) const noexcept
    {
        if (Size() != other.Size()) {
            return false;
        }
        for (usize index = 0U; index < Size(); ++index) {
            if ((*this)[index] != other[index]) {
                return false;
            }
        }
        return true;
    }

private:
    [[nodiscard]] Result<Vector<char>, Error> BuildFromView(StringView value) const noexcept
    {
        if (!HasAllocator()) {
            return Result<Vector<char>, Error>::Failure(
                    MakeStringError(StringErrorCode::INVALID_ARGUMENT, "assign", "string has no bound allocator"));
        }
        Vector<char> next(*AllocatorRef());
        const Status reserveStatus = next.TryReserve(value.Size() + 1U);
        if (reserveStatus.HasError()) {
            return Result<Vector<char>, Error>::Failure(reserveStatus.Error());
        }
        for (usize index = 0U; index < value.Size(); ++index) {
            const Status pushStatus = next.TryPushBack(value[index]);
            if (pushStatus.HasError()) {
                return Result<Vector<char>, Error>::Failure(pushStatus.Error());
            }
        }
        SetNullTerminator(next);
        return Result<Vector<char>, Error>::Success(std::move(next));
    }

    [[nodiscard]] Result<Vector<char>, Error> BuildFromTwoViews(StringView lhs, StringView rhs) const noexcept
    {
        if (!HasAllocator()) {
            return Result<Vector<char>, Error>::Failure(
                    MakeStringError(StringErrorCode::INVALID_ARGUMENT, "append", "string has no bound allocator"));
        }
        Vector<char> next(*AllocatorRef());
        const Status reserveStatus = next.TryReserve(lhs.Size() + rhs.Size() + 1U);
        if (reserveStatus.HasError()) {
            return Result<Vector<char>, Error>::Failure(reserveStatus.Error());
        }
        for (usize index = 0U; index < lhs.Size(); ++index) {
            const Status pushStatus = next.TryPushBack(lhs[index]);
            if (pushStatus.HasError()) {
                return Result<Vector<char>, Error>::Failure(pushStatus.Error());
            }
        }
        for (usize index = 0U; index < rhs.Size(); ++index) {
            const Status pushStatus = next.TryPushBack(rhs[index]);
            if (pushStatus.HasError()) {
                return Result<Vector<char>, Error>::Failure(pushStatus.Error());
            }
        }
        SetNullTerminator(next);
        return Result<Vector<char>, Error>::Success(std::move(next));
    }

    static void SetNullTerminator(Vector<char>& bytes) noexcept
    {
        if (bytes.Empty()) {
            return;
        }
        ZCORE_CONTRACT_REQUIRE(bytes.Capacity() > bytes.Size(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::String requires reserved null-terminator slot");
        char* const data = bytes.Data();
        data[bytes.Size()] = '\0';
    }

    void EnsureNullTerminator() noexcept
    {
        SetNullTerminator(Bytes_);
    }

    Vector<char> Bytes_;
};

} // namespace zcore
