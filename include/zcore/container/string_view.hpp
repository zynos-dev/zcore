/**************************************************************************/
/*  string_view.hpp                                                       */
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
 * @file include/zcore/container/string_view.hpp
 * @brief Non-owning bounds-aware UTF-8 text view.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/string_view.hpp>
 * const zcore::StringView view = zcore::StringView::FromCString("zcore");
 * @endcode
 */

#pragma once

#include <string_view>
#include <zcore/contract_violation.hpp>
#include <zcore/foundation.hpp>
#include <zcore/option.hpp>
#include <zcore/slice.hpp>
#include <zcore/utf8.hpp>

namespace zcore {

/**
 * @brief Non-owning text view over validated UTF-8 bytes.
 *
 * `StringView` never owns memory and validates UTF-8 on checked construction.
 */
class [[nodiscard("StringView must be handled explicitly.")]] StringView final {
public:
    using ValueType = char;
    using Pointer = const char*;
    using Iterator = const char*;

    /// @brief Constructs an empty view.
    constexpr StringView() noexcept : Ptr_(nullptr), Size_(0)
    {
    }

    /**
   * @brief Constructs from pointer and size.
   * @param pointer Text start pointer.
   * @param size Text byte length.
   *
   * Aborts when range is invalid or bytes are not valid UTF-8.
   */
    constexpr StringView(Pointer pointer, usize size) noexcept : Ptr_(pointer), Size_(size)
    {
        EnsureValidUtf8Range(pointer, size);
    }

    /**
   * @brief Constructs from `std::string_view`.
   * @param value Source text view.
   */
    constexpr explicit StringView(std::string_view value) noexcept : Ptr_(value.data()), Size_(value.size())
    {
        EnsureValidUtf8Range(Ptr_, Size_);
    }

    /**
   * @brief Constructs from char array and excludes trailing null terminator.
   * @tparam SizeV Char array extent.
   * @param value Source char array.
   */
    template <usize SizeV>
    constexpr StringView(const char (&value)[SizeV]) noexcept : Ptr_(value), Size_(SizeV == 0 ? 0 : SizeV - 1)
    {
        EnsureValidUtf8Range(Ptr_, Size_);
    }

    constexpr StringView(const StringView&) noexcept = default;
    constexpr StringView& operator=(const StringView&) noexcept = default;
    constexpr StringView(StringView&&) noexcept = default;
    constexpr StringView& operator=(StringView&&) noexcept = default;
    ~StringView() = default;

    /// @brief Returns an empty string view.
    [[nodiscard]] static constexpr StringView Empty() noexcept
    {
        return StringView();
    }

    /**
   * @brief Attempts construction from pointer and size.
   * @return `None` when range is invalid or bytes are not valid UTF-8.
   */
    [[nodiscard]] static constexpr Option<StringView> TryFromRaw(Pointer pointer, usize size) noexcept
    {
        if (size > 0 && pointer == nullptr) {
            return None;
        }
        if (!utf8::IsValid(pointer, size)) {
            return None;
        }
        return Option<StringView>(StringView(pointer, size, UncheckedTag{}));
    }

    /**
   * @brief Constructs from pointer and size without validation.
   * @warning Caller must ensure range validity.
   */
    [[nodiscard]] static constexpr StringView FromRawUnchecked(Pointer pointer, usize size) noexcept
    {
        return StringView(pointer, size, UncheckedTag{});
    }

    /**
   * @brief Attempts construction from null-terminated C string.
   * @param text Source C string.
   * @return `None` when `text == nullptr` or bytes are not valid UTF-8.
   */
    [[nodiscard]] static constexpr Option<StringView> TryFromCString(Pointer text) noexcept
    {
        if (text == nullptr) {
            return None;
        }
        const usize size = std::char_traits<char>::length(text);
        if (!utf8::IsValid(text, size)) {
            return None;
        }
        return Option<StringView>(StringView(text, size, UncheckedTag{}));
    }

    /**
   * @brief Constructs from null-terminated C string.
   * @param text Source C string.
   * @pre `text != nullptr` and bytes are valid UTF-8.
   */
    [[nodiscard]] static constexpr StringView FromCString(Pointer text) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(text != nullptr,
                               detail::ContractViolationCode::STRING_VIEW_INVALID_RANGE,
                               "zcore::StringView::FromCString() requires non-null pointer");
        const usize size = std::char_traits<char>::length(text);
        ZCORE_CONTRACT_REQUIRE(utf8::IsValid(text, size),
                               detail::ContractViolationCode::UTF8_INVALID_SEQUENCE,
                               "zcore::StringView::FromCString() requires valid UTF-8");
        return StringView(text, size, UncheckedTag{});
    }

    /**
   * @brief Attempts construction from C-string that must be valid UTF-8.
   * @param text Source C string.
   * @return `None` when `text == nullptr` or invalid UTF-8.
   */
    [[nodiscard]] static constexpr Option<StringView> TryFromUtf8CString(Pointer text) noexcept
    {
        return TryFromCString(text);
    }

    /**
   * @brief Constructs from C-string that must be valid UTF-8.
   * @param text Source C string.
   * @pre `text != nullptr` and UTF-8 sequence is valid.
   */
    [[nodiscard]] static constexpr StringView FromUtf8CString(Pointer text) noexcept
    {
        return FromCString(text);
    }

    /// @brief Returns raw pointer to text start.
    [[nodiscard]] constexpr Pointer Data() const noexcept
    {
        return Ptr_;
    }

    [[nodiscard]] constexpr Pointer data() const noexcept
    {
        return Data();
    }

    /// @brief Returns view byte length.
    [[nodiscard]] constexpr usize Size() const noexcept
    {
        return Size_;
    }

    [[nodiscard]] constexpr usize size() const noexcept
    {
        return Size();
    }

    /// @brief Returns `true` when empty.
    [[nodiscard]] constexpr bool EmptyView() const noexcept
    {
        return Size_ == 0;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return EmptyView();
    }

    /// @brief Returns iterator to first character.
    [[nodiscard]] constexpr Iterator begin() const noexcept
    {
        return Ptr_;
    }

    /// @brief Returns iterator past last character.
    [[nodiscard]] constexpr Iterator end() const noexcept
    {
        return Size_ == 0 ? Ptr_ : Ptr_ + Size_;
    }

    /**
   * @brief Checked indexed access.
   * @pre `index < Size()`.
   */
    [[nodiscard]] constexpr char operator[](usize index) const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(index < Size_,
                               detail::ContractViolationCode::STRING_VIEW_INDEX_OUT_OF_BOUNDS,
                               "zcore::StringView::operator[] index out of bounds");
        return Ptr_[index];
    }

    /// @brief Returns pointer to character at `index` or `nullptr` when out-of-range.
    [[nodiscard]] constexpr Pointer TryAt(usize index) const noexcept
    {
        return index < Size_ ? Ptr_ + index : nullptr;
    }

    /**
   * @brief Returns first `count` UTF-8 code points.
   * @return `None` when count exceeds available code points or view is invalid UTF-8.
   */
    [[nodiscard]] constexpr Option<StringView> First(usize count) const noexcept
    {
        Option<usize> end = utf8::AdvanceCodePoints(Ptr_, Size_, 0U, count);
        if (!end.HasValue()) {
            return None;
        }
        if (end.Value() == 0U) {
            return Option<StringView>(StringView());
        }
        return Option<StringView>(StringView(Ptr_, end.Value(), UncheckedTag{}));
    }

    /**
   * @brief Returns last `count` UTF-8 code points.
   * @return `None` when count exceeds available code points or view is invalid UTF-8.
   */
    [[nodiscard]] constexpr Option<StringView> Last(usize count) const noexcept
    {
        Option<usize> totalCodePoints = utf8::CountCodePoints(Ptr_, Size_);
        if (!totalCodePoints.HasValue() || count > totalCodePoints.Value()) {
            return None;
        }
        if (count == 0) {
            return Option<StringView>(StringView());
        }
        const usize startCodePoint = totalCodePoints.Value() - count;
        Option<usize> startOffset = utf8::AdvanceCodePoints(Ptr_, Size_, 0U, startCodePoint);
        if (!startOffset.HasValue()) {
            return None;
        }
        return Option<StringView>(StringView(Ptr_ + startOffset.Value(), Size_ - startOffset.Value(), UncheckedTag{}));
    }

    /**
   * @brief Returns subview at code point `offset` with `count` code points.
   * @return `None` for invalid ranges or invalid UTF-8.
   */
    [[nodiscard]] constexpr Option<StringView> Substr(usize offset, usize count) const noexcept
    {
        Option<usize> byteOffset = utf8::AdvanceCodePoints(Ptr_, Size_, 0U, offset);
        if (!byteOffset.HasValue()) {
            return None;
        }
        Option<usize> byteEnd = utf8::AdvanceCodePoints(Ptr_, Size_, byteOffset.Value(), count);
        if (!byteEnd.HasValue()) {
            return None;
        }
        if (count == 0) {
            return Option<StringView>(StringView());
        }
        return Option<StringView>(StringView(Ptr_ + byteOffset.Value(), byteEnd.Value() - byteOffset.Value(), UncheckedTag{}));
    }

    /**
   * @brief Returns subview at byte `offset` with `count` bytes.
   * @return `None` for invalid byte ranges.
   */
    [[nodiscard]] constexpr Option<StringView> SubstrBytes(usize offset, usize count) const noexcept
    {
        if (offset > Size_ || count > (Size_ - offset)) {
            return None;
        }
        if (count == 0U) {
            return Option<StringView>(StringView());
        }
        return Option<StringView>(StringView(Ptr_ + offset, count, UncheckedTag{}));
    }

    /// @brief Returns `true` when contents are valid UTF-8.
    [[nodiscard]] constexpr bool IsValidUtf8() const noexcept
    {
        return utf8::IsValid(Ptr_, Size_);
    }

    /**
   * @brief Counts Unicode scalar values in UTF-8 text.
   * @return `None` when contents are invalid UTF-8.
   */
    [[nodiscard]] constexpr Option<usize> TryCodePointCount() const noexcept
    {
        return utf8::CountCodePoints(Ptr_, Size_);
    }

    /**
   * @brief Returns code point count for UTF-8 text.
   * @pre `IsValidUtf8()`.
   */
    [[nodiscard]] constexpr usize CodePointCount() const noexcept
    {
        Option<usize> count = TryCodePointCount();
        ZCORE_CONTRACT_REQUIRE(count.HasValue(),
                               detail::ContractViolationCode::UTF8_INVALID_SEQUENCE,
                               "zcore::StringView::CodePointCount() requires valid UTF-8");
        return count.Value();
    }

    /**
   * @brief Removes `count` UTF-8 code points from front.
   * @return `true` on success, `false` when count exceeds available code points or UTF-8 is invalid.
   */
    constexpr bool RemovePrefix(usize count) noexcept
    {
        Option<usize> bytesToDrop = utf8::AdvanceCodePoints(Ptr_, Size_, 0U, count);
        if (!bytesToDrop.HasValue()) {
            return false;
        }
        if (bytesToDrop.Value() == Size_) {
            Clear();
            return true;
        }
        if (bytesToDrop.Value() > 0U) {
            Ptr_ += bytesToDrop.Value();
            Size_ -= bytesToDrop.Value();
        }
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
        if (count == Size_) {
            Clear();
            return true;
        }
        if (count > 0U) {
            Ptr_ += count;
            Size_ -= count;
        }
        return true;
    }

    /**
   * @brief Removes `count` UTF-8 code points from back.
   * @return `true` on success, `false` when count exceeds available code points or UTF-8 is invalid.
   */
    constexpr bool RemoveSuffix(usize count) noexcept
    {
        Option<usize> totalCodePoints = utf8::CountCodePoints(Ptr_, Size_);
        if (!totalCodePoints.HasValue() || count > totalCodePoints.Value()) {
            return false;
        }
        const usize remainingCodePoints = totalCodePoints.Value() - count;
        Option<usize> newSize = utf8::AdvanceCodePoints(Ptr_, Size_, 0U, remainingCodePoints);
        if (!newSize.HasValue()) {
            return false;
        }
        if (newSize.Value() == 0U) {
            Clear();
            return true;
        }
        Size_ = newSize.Value();
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
        if (count == Size_) {
            Clear();
            return true;
        }
        Size_ -= count;
        return true;
    }

    /// @brief Resets to empty view.
    constexpr void Clear() noexcept
    {
        Ptr_ = nullptr;
        Size_ = 0;
    }

    /// @brief Converts to `zcore::Slice<const char>`.
    [[nodiscard]] constexpr Slice<const char> AsSlice() const noexcept
    {
        if (EmptyView()) {
            return Slice<const char>::Empty();
        }
        return Slice<const char>::FromRawUnchecked(Ptr_, Size_);
    }

    /// @brief Converts to `std::string_view`.
    [[nodiscard]] constexpr std::string_view AsStdStringView() const noexcept
    {
        return std::string_view(Ptr_, Size_);
    }

    [[nodiscard]] constexpr operator Slice<const char>() const noexcept
    {
        return AsSlice();
    }

    [[nodiscard]] constexpr operator std::string_view() const noexcept
    {
        return AsStdStringView();
    }

    [[nodiscard]] constexpr bool operator==(StringView other) const noexcept
    {
        if (Size_ != other.Size_) {
            return false;
        }
        for (usize index = 0; index < Size_; ++index) {
            if (Ptr_[index] != other.Ptr_[index]) {
                return false;
            }
        }
        return true;
    }

private:
    struct UncheckedTag final {};

    constexpr StringView(Pointer pointer, usize size, UncheckedTag) noexcept : Ptr_(pointer), Size_(size)
    {
    }

    static constexpr void EnsureValidUtf8Range(Pointer pointer, usize size) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(size == 0 || pointer != nullptr,
                               detail::ContractViolationCode::STRING_VIEW_INVALID_RANGE,
                               "zcore::StringView requires non-null pointer when size is non-zero");
        ZCORE_CONTRACT_REQUIRE(utf8::IsValid(pointer, size),
                               detail::ContractViolationCode::UTF8_INVALID_SEQUENCE,
                               "zcore::StringView requires valid UTF-8 bytes");
    }

    Pointer Ptr_;
    usize Size_;
};

} // namespace zcore
