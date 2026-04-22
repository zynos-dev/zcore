/**************************************************************************/
/*  fixed_string.hpp                                                      */
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
 * @file include/zcore/container/fixed_string.hpp
 * @brief Deterministic fixed-capacity UTF-8 string storage.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/fixed_string.hpp>
 * zcore::FixedString<16> value = zcore::FixedString<16>::FromCString("zcore");
 * @endcode
 */

#pragma once

#include <cstring>
#include <string_view>
#include <zcore/contract_violation.hpp>
#include <zcore/foundation.hpp>
#include <zcore/option.hpp>
#include <zcore/string_view.hpp>
#include <zcore/utf8.hpp>

namespace zcore {

/**
 * @brief Fixed-capacity UTF-8 string with inline owned storage and explicit overflow policy.
 *
 * @tparam CapacityV Maximum number of stored bytes (excluding null terminator).
 */
template <usize CapacityV>
class [[nodiscard("FixedString must be handled explicitly.")]] FixedString final {
public:
    using ValueType = char;
    using Pointer = char*;
    using ConstPointer = const char*;
    using Iterator = char*;
    using ConstIterator = const char*;

    static constexpr usize kCapacity = CapacityV;

    /// @brief Constructs an empty fixed string.
    constexpr FixedString() noexcept : Size_(0)
    {
        Storage_[0] = '\0';
    }

    /**
   * @brief Constructs from `StringView` and enforces capacity.
   * @pre `value.Size() <= Capacity()`.
   */
    explicit constexpr FixedString(StringView value) noexcept : Size_(0)
    {
        Assign(value);
    }

    constexpr FixedString(const FixedString&) noexcept = default;
    constexpr FixedString& operator=(const FixedString&) noexcept = default;
    constexpr FixedString(FixedString&&) noexcept = default;
    constexpr FixedString& operator=(FixedString&&) noexcept = default;
    ~FixedString() = default;

    /// @brief Returns compile-time byte capacity.
    [[nodiscard]] static constexpr usize Capacity() noexcept
    {
        return kCapacity;
    }

    /// @brief Returns stored byte count.
    [[nodiscard]] constexpr usize Size() const noexcept
    {
        return Size_;
    }

    [[nodiscard]] constexpr usize size() const noexcept
    {
        return Size();
    }

    /// @brief Returns remaining write capacity in bytes.
    [[nodiscard]] constexpr usize RemainingCapacity() const noexcept
    {
        return kCapacity - Size_;
    }

    /// @brief Returns `true` when no bytes are stored.
    [[nodiscard]] constexpr bool Empty() const noexcept
    {
        return Size_ == 0;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return Empty();
    }

    /// @brief Returns `true` when `Size() == Capacity()`.
    [[nodiscard]] constexpr bool Full() const noexcept
    {
        return Size_ == kCapacity;
    }

    [[nodiscard]] constexpr bool full() const noexcept
    { // NOLINT(readability-identifier-naming)
        return Full();
    }

    /// @brief Returns mutable pointer to null-terminated character storage.
    [[nodiscard]] constexpr Pointer Data() noexcept
    {
        return Storage_;
    }

    /// @brief Returns const pointer to null-terminated character storage.
    [[nodiscard]] constexpr ConstPointer Data() const noexcept
    {
        return Storage_;
    }

    [[nodiscard]] constexpr Pointer data() noexcept
    {
        return Data();
    }

    [[nodiscard]] constexpr ConstPointer data() const noexcept
    {
        return Data();
    }

    /// @brief Returns null-terminated C-string pointer.
    [[nodiscard]] constexpr ConstPointer CStr() const noexcept
    {
        return Storage_;
    }

    [[nodiscard]] constexpr ConstPointer c_str() const noexcept
    { // NOLINT(readability-identifier-naming)
        return CStr();
    }

    /// @brief Returns iterator to first character.
    [[nodiscard]] constexpr Iterator begin() noexcept
    {
        return Storage_;
    }

    /// @brief Returns iterator past last character.
    [[nodiscard]] constexpr Iterator end() noexcept
    {
        return Storage_ + Size_;
    }

    /// @brief Returns const iterator to first character.
    [[nodiscard]] constexpr ConstIterator begin() const noexcept
    {
        return Storage_;
    }

    /// @brief Returns const iterator past last character.
    [[nodiscard]] constexpr ConstIterator end() const noexcept
    {
        return Storage_ + Size_;
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
   * @brief Checked indexed mutable access.
   * @pre `index < Size()`.
   */
    [[nodiscard]] constexpr char& operator[](usize index) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(index < Size_,
                               detail::ContractViolationCode::FIXED_STRING_INDEX_OUT_OF_BOUNDS,
                               "zcore::FixedString::operator[] index out of bounds");
        return Storage_[index];
    }

    /**
   * @brief Checked indexed const access.
   * @pre `index < Size()`.
   */
    [[nodiscard]] constexpr char operator[](usize index) const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(index < Size_,
                               detail::ContractViolationCode::FIXED_STRING_INDEX_OUT_OF_BOUNDS,
                               "zcore::FixedString::operator[] index out of bounds");
        return Storage_[index];
    }

    /// @brief Returns pointer to character at `index` or `nullptr` when out-of-range.
    [[nodiscard]] constexpr Pointer TryAt(usize index) noexcept
    {
        return index < Size_ ? Storage_ + index : nullptr;
    }

    /// @brief Returns pointer to character at `index` or `nullptr` when out-of-range.
    [[nodiscard]] constexpr ConstPointer TryAt(usize index) const noexcept
    {
        return index < Size_ ? Storage_ + index : nullptr;
    }

    /**
   * @brief Assigns content from `value` if it fits.
   * @return `true` when assigned, `false` when invalid UTF-8 or overflow would occur.
   */
    [[nodiscard]] constexpr bool TryAssign(StringView value) noexcept
    {
        if (!value.IsValidUtf8()) {
            return false;
        }
        if (value.Size() > kCapacity) {
            return false;
        }
        for (usize index = 0; index < value.Size(); ++index) {
            Storage_[index] = value[index];
        }
        Size_ = value.Size();
        Storage_[Size_] = '\0';
        return true;
    }

    /**
   * @brief Attempts assignment from UTF-8 text.
   * @return `true` when assigned, `false` when invalid UTF-8 or overflow.
   */
    [[nodiscard]] constexpr bool TryAssignUtf8(StringView value) noexcept
    {
        return TryAssign(value);
    }

    /**
   * @brief Assigns content from `value`.
   * @pre `value` is valid UTF-8 and `value.Size() <= Capacity()`.
   */
    constexpr void Assign(StringView value) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(value.IsValidUtf8(),
                               detail::ContractViolationCode::UTF8_INVALID_SEQUENCE,
                               "zcore::FixedString::Assign() requires valid UTF-8");
        ZCORE_CONTRACT_REQUIRE(value.Size() <= kCapacity,
                               detail::ContractViolationCode::FIXED_STRING_CAPACITY_EXCEEDED,
                               "zcore::FixedString::Assign() exceeds fixed capacity");
        static_cast<void>(TryAssign(value));
    }

    /**
   * @brief Assigns UTF-8 text.
   * @pre `value` is valid UTF-8 and fits capacity.
   */
    constexpr void AssignUtf8(StringView value) noexcept
    {
        Assign(value);
    }

    /**
   * @brief Attempts assignment from C-string.
   * @return `true` on success, `false` when null/invalid UTF-8/overflow.
   */
    [[nodiscard]] constexpr bool TryAssignCString(ConstPointer value) noexcept
    {
        Option<StringView> maybe = StringView::TryFromCString(value);
        if (!maybe.HasValue()) {
            return false;
        }
        return TryAssign(maybe.Value());
    }

    /**
   * @brief Attempts to append one character.
   * @return `true` on success, `false` when full or byte is not ASCII.
   */
    [[nodiscard]] constexpr bool TryPushBack(char value) noexcept
    {
        if (static_cast<unsigned char>(value) > 0x7FU) {
            return false;
        }
        if (Full()) {
            return false;
        }
        Storage_[Size_] = value;
        ++Size_;
        Storage_[Size_] = '\0';
        return true;
    }

    /**
   * @brief Appends one character.
   * @pre `value` is ASCII and `!Full()`.
   */
    constexpr void PushBack(char value) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(static_cast<unsigned char>(value) <= 0x7FU,
                               detail::ContractViolationCode::UTF8_INVALID_SEQUENCE,
                               "zcore::FixedString::PushBack() only accepts ASCII bytes");
        ZCORE_CONTRACT_REQUIRE(!Full(),
                               detail::ContractViolationCode::FIXED_STRING_CAPACITY_EXCEEDED,
                               "zcore::FixedString::PushBack() exceeds fixed capacity");
        static_cast<void>(TryPushBack(value));
    }

    /**
   * @brief Attempts to append one Unicode scalar value.
   * @return `true` on success, `false` when value is invalid or capacity is insufficient.
   */
    [[nodiscard]] constexpr bool TryAppendCodePoint(u32 value) noexcept
    {
        char encoded[4] = {};
        Option<usize> width = utf8::EncodeCodePoint(value, encoded, sizeof(encoded));
        if (!width.HasValue() || width.Value() > RemainingCapacity()) {
            return false;
        }
        const usize writeOffset = Size_;
        for (usize index = 0U; index < width.Value(); ++index) {
            Storage_[writeOffset + index] = encoded[index];
        }
        Size_ += width.Value();
        Storage_[Size_] = '\0';
        return true;
    }

    /**
   * @brief Appends one Unicode scalar value.
   * @pre `value` is valid scalar and encoded bytes fit remaining capacity.
   */
    constexpr void AppendCodePoint(u32 value) noexcept
    {
        char encoded[4] = {};
        Option<usize> width = utf8::EncodeCodePoint(value, encoded, sizeof(encoded));
        ZCORE_CONTRACT_REQUIRE(width.HasValue(),
                               detail::ContractViolationCode::UTF8_INVALID_SEQUENCE,
                               "zcore::FixedString::AppendCodePoint() requires valid Unicode scalar value");
        ZCORE_CONTRACT_REQUIRE(width.Value() <= RemainingCapacity(),
                               detail::ContractViolationCode::FIXED_STRING_CAPACITY_EXCEEDED,
                               "zcore::FixedString::AppendCodePoint() exceeds fixed capacity");
        const usize writeOffset = Size_;
        for (usize index = 0U; index < width.Value(); ++index) {
            Storage_[writeOffset + index] = encoded[index];
        }
        Size_ += width.Value();
        Storage_[Size_] = '\0';
    }

    /**
   * @brief Attempts append from `value`.
   * @return `true` on success, `false` when invalid UTF-8 or overflow would occur.
   */
    [[nodiscard]] constexpr bool TryAppend(StringView value) noexcept
    {
        if (!value.IsValidUtf8()) {
            return false;
        }
        if (value.Size() > RemainingCapacity()) {
            return false;
        }
        const usize writeOffset = Size_;
        for (usize index = 0; index < value.Size(); ++index) {
            Storage_[writeOffset + index] = value[index];
        }
        Size_ += value.Size();
        Storage_[Size_] = '\0';
        return true;
    }

    /**
   * @brief Attempts append from UTF-8 text.
   * @return `true` on success, `false` when invalid UTF-8 or overflow.
   */
    [[nodiscard]] constexpr bool TryAppendUtf8(StringView value) noexcept
    {
        return TryAppend(value);
    }

    /**
   * @brief Appends `value`.
   * @pre `value` is valid UTF-8 and `Size() + value.Size() <= Capacity()`.
   */
    constexpr void Append(StringView value) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(value.IsValidUtf8(),
                               detail::ContractViolationCode::UTF8_INVALID_SEQUENCE,
                               "zcore::FixedString::Append() requires valid UTF-8");
        ZCORE_CONTRACT_REQUIRE(value.Size() <= RemainingCapacity(),
                               detail::ContractViolationCode::FIXED_STRING_CAPACITY_EXCEEDED,
                               "zcore::FixedString::Append() exceeds fixed capacity");
        static_cast<void>(TryAppend(value));
    }

    /**
   * @brief Appends UTF-8 text.
   * @pre `value` is valid UTF-8 and resulting size fits capacity.
   */
    constexpr void AppendUtf8(StringView value) noexcept
    {
        Append(value);
    }

    /**
   * @brief Attempts to remove last UTF-8 code point.
   * @return `true` on success, `false` when empty.
   */
    [[nodiscard]] constexpr bool TryPopBack() noexcept
    {
        if (Empty()) {
            return false;
        }
        Option<usize> totalCodePoints = utf8::CountCodePoints(Storage_, Size_);
        if (!totalCodePoints.HasValue() || totalCodePoints.Value() == 0U) {
            return false;
        }
        Option<usize> nextSize = utf8::AdvanceCodePoints(Storage_, Size_, 0U, totalCodePoints.Value() - 1U);
        if (!nextSize.HasValue()) {
            return false;
        }
        Size_ = nextSize.Value();
        Storage_[Size_] = '\0';
        return true;
    }

    /**
   * @brief Removes last UTF-8 code point.
   * @pre `!Empty()`.
   */
    constexpr void PopBack() noexcept
    {
        ZCORE_CONTRACT_REQUIRE(TryPopBack(),
                               detail::ContractViolationCode::FIXED_STRING_INDEX_OUT_OF_BOUNDS,
                               "zcore::FixedString::PopBack() requires non-empty string");
    }

    /**
   * @brief Removes `count` UTF-8 code points from front.
   * @return `true` on success, `false` when count exceeds available code points.
   */
    constexpr bool RemovePrefix(usize count) noexcept
    {
        Option<usize> bytesToDrop = utf8::AdvanceCodePoints(Storage_, Size_, 0U, count);
        if (!bytesToDrop.HasValue()) {
            return false;
        }
        if (bytesToDrop.Value() == 0U) {
            return true;
        }
        if (bytesToDrop.Value() == Size_) {
            Clear();
            return true;
        }
        const usize nextSize = Size_ - bytesToDrop.Value();
        std::memmove(Storage_, Storage_ + bytesToDrop.Value(), nextSize);
        Size_ = nextSize;
        Storage_[Size_] = '\0';
        return true;
    }

    /**
   * @brief Removes `count` bytes from front.
   * @return `true` on success, `false` when `count > Size()`.
   */
    constexpr bool RemovePrefixBytes(usize count) noexcept
    {
        if (count > Size_) {
            return false;
        }
        if (count == 0U) {
            return true;
        }
        if (count == Size_) {
            Clear();
            return true;
        }
        const usize nextSize = Size_ - count;
        std::memmove(Storage_, Storage_ + count, nextSize);
        Size_ = nextSize;
        Storage_[Size_] = '\0';
        return true;
    }

    /**
   * @brief Removes `count` UTF-8 code points from back.
   * @return `true` on success, `false` when count exceeds available code points.
   */
    constexpr bool RemoveSuffix(usize count) noexcept
    {
        Option<usize> totalCodePoints = utf8::CountCodePoints(Storage_, Size_);
        if (!totalCodePoints.HasValue() || count > totalCodePoints.Value()) {
            return false;
        }
        const usize remainingCodePoints = totalCodePoints.Value() - count;
        Option<usize> nextSize = utf8::AdvanceCodePoints(Storage_, Size_, 0U, remainingCodePoints);
        if (!nextSize.HasValue()) {
            return false;
        }
        Size_ = nextSize.Value();
        Storage_[Size_] = '\0';
        return true;
    }

    /**
   * @brief Removes `count` bytes from back.
   * @return `true` on success, `false` when `count > Size()`.
   */
    constexpr bool RemoveSuffixBytes(usize count) noexcept
    {
        if (count > Size_) {
            return false;
        }
        Size_ -= count;
        Storage_[Size_] = '\0';
        return true;
    }

    /// @brief Clears to empty string.
    constexpr void Clear() noexcept
    {
        Size_ = 0;
        Storage_[0] = '\0';
    }

    /// @brief Returns view over current characters.
    [[nodiscard]] constexpr StringView AsStringView() const noexcept
    {
        return StringView::FromRawUnchecked(Storage_, Size_);
    }

    /// @brief Returns `true` when current contents are valid UTF-8.
    [[nodiscard]] constexpr bool IsValidUtf8() const noexcept
    {
        return utf8::IsValid(Storage_, Size_);
    }

    /**
   * @brief Counts Unicode scalar values in current UTF-8 text.
   * @return `None` when current text is invalid UTF-8.
   */
    [[nodiscard]] constexpr Option<usize> TryCodePointCount() const noexcept
    {
        return utf8::CountCodePoints(Storage_, Size_);
    }

    /**
   * @brief Returns code point count.
   * @pre `IsValidUtf8()`.
   */
    [[nodiscard]] constexpr usize CodePointCount() const noexcept
    {
        Option<usize> count = TryCodePointCount();
        ZCORE_CONTRACT_REQUIRE(count.HasValue(),
                               detail::ContractViolationCode::UTF8_INVALID_SEQUENCE,
                               "zcore::FixedString::CodePointCount() requires valid UTF-8");
        return count.Value();
    }

    /// @brief Returns `std::string_view` over current characters.
    [[nodiscard]] constexpr std::string_view AsStdStringView() const noexcept
    {
        return std::string_view(Storage_, Size_);
    }

    [[nodiscard]] constexpr operator StringView() const noexcept
    {
        return AsStringView();
    }

    [[nodiscard]] constexpr operator std::string_view() const noexcept
    {
        return AsStdStringView();
    }

    [[nodiscard]] constexpr bool operator==(const FixedString& other) const noexcept
    {
        if (Size_ != other.Size_) {
            return false;
        }
        for (usize index = 0; index < Size_; ++index) {
            if (Storage_[index] != other.Storage_[index]) {
                return false;
            }
        }
        return true;
    }

    /**
   * @brief Attempts construction from `StringView`.
   * @return `None` when input exceeds capacity.
   */
    [[nodiscard]] static constexpr Option<FixedString> TryFromStringView(StringView value) noexcept
    {
        FixedString out;
        if (!out.TryAssign(value)) {
            return None;
        }
        return Option<FixedString>(out);
    }

    /**
   * @brief Constructs from `StringView`.
   * @pre `value.Size() <= Capacity()`.
   */
    [[nodiscard]] static constexpr FixedString FromStringView(StringView value) noexcept
    {
        FixedString out;
        out.Assign(value);
        return out;
    }

    /**
   * @brief Attempts construction from C-string.
   * @return `None` when pointer is null, bytes are invalid UTF-8, or content exceeds capacity.
   */
    [[nodiscard]] static constexpr Option<FixedString> TryFromCString(ConstPointer value) noexcept
    {
        Option<StringView> maybe = StringView::TryFromCString(value);
        if (!maybe.HasValue()) {
            return None;
        }
        return TryFromStringView(maybe.Value());
    }

    /**
   * @brief Attempts construction from C-string that must be valid UTF-8.
   * @return `None` when pointer is null, invalid UTF-8, or overflow.
   */
    [[nodiscard]] static constexpr Option<FixedString> TryFromUtf8CString(ConstPointer value) noexcept
    {
        return TryFromCString(value);
    }

    /**
   * @brief Constructs from C-string.
   * @pre `value != nullptr`, bytes are valid UTF-8, and string length `<= Capacity()`.
   */
    [[nodiscard]] static constexpr FixedString FromCString(ConstPointer value) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(value != nullptr,
                               detail::ContractViolationCode::STRING_VIEW_INVALID_RANGE,
                               "zcore::FixedString::FromCString() requires non-null c-string");
        Option<StringView> maybe = StringView::TryFromCString(value);
        ZCORE_CONTRACT_REQUIRE(maybe.HasValue(),
                               detail::ContractViolationCode::UTF8_INVALID_SEQUENCE,
                               "zcore::FixedString::FromCString() requires valid UTF-8");
        FixedString out;
        ZCORE_CONTRACT_REQUIRE(out.TryAssign(maybe.Value()),
                               detail::ContractViolationCode::FIXED_STRING_CAPACITY_EXCEEDED,
                               "zcore::FixedString::FromCString() exceeds fixed capacity");
        return out;
    }

    /**
   * @brief Constructs from C-string that must be valid UTF-8.
   * @pre `value != nullptr`, UTF-8 valid, and length `<= Capacity()`.
   */
    [[nodiscard]] static constexpr FixedString FromUtf8CString(ConstPointer value) noexcept
    {
        return FromCString(value);
    }

private:
    char Storage_[kCapacity + 1];
    usize Size_;
};

} // namespace zcore
