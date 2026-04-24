/**************************************************************************/
/*  string_fuzz.cpp                                                       */
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
 * @file fuzz/string_fuzz.cpp
 * @brief Local libFuzzer harness for String/FixedString mutation invariants.
 */

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <vector>
#include <zcore/fixed_string.hpp>
#include <zcore/string.hpp>
#include <zcore/utf8.hpp>

namespace {

class FuzzCursor final {
public:
    FuzzCursor(const std::uint8_t* data, std::size_t size) noexcept : Data_(data), Size_(size)
    {
    }

    [[nodiscard]] std::uint8_t TakeByte() noexcept
    {
        if (Position_ >= Size_) {
            return 0U;
        }
        return Data_[Position_++];
    }

    [[nodiscard]] std::uint32_t TakeU32() noexcept
    {
        std::uint32_t value = 0U;
        for (std::uint32_t index = 0U; index < 4U; ++index) {
            value |= static_cast<std::uint32_t>(TakeByte()) << (index * 8U);
        }
        return value;
    }

private:
    const std::uint8_t* Data_{nullptr};
    std::size_t Size_{0U};
    std::size_t Position_{0U};
};

[[noreturn]] void FuzzAbort() noexcept
{
    std::abort();
}

class FuzzAllocator final : public zcore::Allocator {
public:
    [[nodiscard]] zcore::Result<zcore::Allocation, zcore::Error> Allocate(zcore::AllocationRequest request) noexcept override
    {
        const zcore::Status requestStatus = zcore::ValidateAllocationRequest(request);
        if (requestStatus.HasError()) {
            return zcore::Result<zcore::Allocation, zcore::Error>::Failure(requestStatus.Error());
        }
        if (request.size == 0U) {
            return zcore::Result<zcore::Allocation, zcore::Error>::Success(zcore::Allocation::Empty());
        }

        void* const raw = ::operator new(request.size, static_cast<std::align_val_t>(request.alignment), std::nothrow);
        if (raw == nullptr) {
            return zcore::Result<zcore::Allocation, zcore::Error>::Failure(
                    zcore::MakeAllocatorError(zcore::AllocatorErrorCode::OUT_OF_MEMORY,
                                              "allocate",
                                              "system allocator returned null"));
        }
        return zcore::Result<zcore::Allocation, zcore::Error>::Success(zcore::Allocation{
                .data = static_cast<zcore::Byte*>(raw),
                .size = request.size,
                .alignment = request.alignment,
        });
    }

    [[nodiscard]] zcore::Status Deallocate(zcore::Allocation allocation) noexcept override
    {
        zcore::Status allocationStatus = zcore::ValidateAllocation(allocation);
        if (allocationStatus.HasError()) {
            return allocationStatus;
        }
        if (allocation.IsEmpty()) {
            return zcore::OkStatus();
        }
        ::operator delete(static_cast<void*>(allocation.data), static_cast<std::align_val_t>(allocation.alignment));
        return zcore::OkStatus();
    }
};

class Utf8Model final {
public:
    explicit Utf8Model(zcore::usize capacityLimit) noexcept : CapacityLimit_(capacityLimit)
    {
    }

    [[nodiscard]] bool TryAssignCString(const char* text)
    {
        if (text == nullptr) {
            return false;
        }
        const zcore::usize length = static_cast<zcore::usize>(std::strlen(text));
        if (length > CapacityLimit_) {
            return false;
        }
        if (!zcore::utf8::IsValid(text, length)) {
            return false;
        }
        Bytes_.assign(text, text + length);
        return true;
    }

    [[nodiscard]] bool TryAssignView(zcore::StringView value)
    {
        if (!value.IsValidUtf8()) {
            return false;
        }
        if (value.Size() > CapacityLimit_) {
            return false;
        }
        Bytes_.clear();
        Bytes_.reserve(value.Size());
        for (zcore::usize index = 0U; index < value.Size(); ++index) {
            Bytes_.push_back(value[index]);
        }
        return true;
    }

    [[nodiscard]] bool TryAppendView(zcore::StringView value)
    {
        if (!value.IsValidUtf8()) {
            return false;
        }
        if (value.Size() > RemainingCapacity()) {
            return false;
        }
        const zcore::usize oldSize = Size();
        Bytes_.resize(oldSize + value.Size());
        for (zcore::usize index = 0U; index < value.Size(); ++index) {
            Bytes_[oldSize + index] = value[index];
        }
        return true;
    }

    [[nodiscard]] bool TryAppendCodePoint(zcore::u32 value)
    {
        char encoded[4] = {};
        const zcore::Option<zcore::usize> width = zcore::utf8::EncodeCodePoint(value, encoded, 4U);
        if (!width.HasValue()) {
            return false;
        }
        if (width.Value() > RemainingCapacity()) {
            return false;
        }
        Bytes_.insert(Bytes_.end(), encoded, encoded + width.Value());
        return true;
    }

    [[nodiscard]] bool TryPushBack(char value)
    {
        if (static_cast<unsigned char>(value) > 0x7FU) {
            return false;
        }
        if (RemainingCapacity() == 0U) {
            return false;
        }
        Bytes_.push_back(value);
        return true;
    }

    [[nodiscard]] bool TryPopBack()
    {
        if (Bytes_.empty()) {
            return false;
        }
        const zcore::Option<zcore::usize> totalCodePoints = zcore::utf8::CountCodePoints(DataOrNull(), Size());
        if (!totalCodePoints.HasValue() || totalCodePoints.Value() == 0U) {
            return false;
        }
        const zcore::Option<zcore::usize> newSize =
                zcore::utf8::AdvanceCodePoints(DataOrNull(), Size(), 0U, totalCodePoints.Value() - 1U);
        if (!newSize.HasValue()) {
            return false;
        }
        return RemoveSuffixBytes(Size() - newSize.Value());
    }

    [[nodiscard]] bool RemovePrefix(zcore::usize count)
    {
        const zcore::Option<zcore::usize> bytesToDrop = zcore::utf8::AdvanceCodePoints(DataOrNull(), Size(), 0U, count);
        if (!bytesToDrop.HasValue()) {
            return false;
        }
        return RemovePrefixBytes(bytesToDrop.Value());
    }

    [[nodiscard]] bool RemoveSuffix(zcore::usize count)
    {
        const zcore::Option<zcore::usize> totalCodePoints = zcore::utf8::CountCodePoints(DataOrNull(), Size());
        if (!totalCodePoints.HasValue() || count > totalCodePoints.Value()) {
            return false;
        }
        const zcore::Option<zcore::usize> newSize =
                zcore::utf8::AdvanceCodePoints(DataOrNull(), Size(), 0U, totalCodePoints.Value() - count);
        if (!newSize.HasValue()) {
            return false;
        }
        return RemoveSuffixBytes(Size() - newSize.Value());
    }

    [[nodiscard]] bool RemovePrefixBytes(zcore::usize count)
    {
        if (count > Size()) {
            return false;
        }
        Bytes_.erase(Bytes_.begin(), Bytes_.begin() + static_cast<std::ptrdiff_t>(count));
        return true;
    }

    [[nodiscard]] bool RemoveSuffixBytes(zcore::usize count)
    {
        if (count > Size()) {
            return false;
        }
        Bytes_.resize(Size() - count);
        return true;
    }

    void Clear()
    {
        Bytes_.clear();
    }

    [[nodiscard]] bool IsValidUtf8() const
    {
        return zcore::utf8::IsValid(DataOrNull(), Size());
    }

    [[nodiscard]] zcore::Option<zcore::usize> TryCodePointCount() const
    {
        return zcore::utf8::CountCodePoints(DataOrNull(), Size());
    }

    [[nodiscard]] zcore::usize Size() const noexcept
    {
        return static_cast<zcore::usize>(Bytes_.size());
    }

    [[nodiscard]] zcore::usize RemainingCapacity() const noexcept
    {
        if (CapacityLimit_ < Size()) {
            return 0U;
        }
        return CapacityLimit_ - Size();
    }

    [[nodiscard]] const std::vector<char>& Bytes() const noexcept
    {
        return Bytes_;
    }

private:
    [[nodiscard]] const char* DataOrNull() const noexcept
    {
        return Bytes_.empty() ? nullptr : Bytes_.data();
    }

    zcore::usize CapacityLimit_{0U};
    std::vector<char> Bytes_;
};

zcore::u32 TakeRangeValue(FuzzCursor& cursor, zcore::u32 lo, zcore::u32 hi) noexcept
{
    if (lo >= hi) {
        return lo;
    }
    const std::uint64_t span = static_cast<std::uint64_t>(hi) - static_cast<std::uint64_t>(lo) + 1U;
    return static_cast<zcore::u32>(static_cast<std::uint64_t>(lo) + (static_cast<std::uint64_t>(cursor.TakeU32()) % span));
}

zcore::u32 TakeValidScalar(FuzzCursor& cursor) noexcept
{
    const std::uint8_t bucket = static_cast<std::uint8_t>(cursor.TakeByte() % 4U);
    if (bucket == 0U) {
        return TakeRangeValue(cursor, 0x20U, 0x7EU);
    }
    if (bucket == 1U) {
        return TakeRangeValue(cursor, 0x80U, 0x7FFU);
    }
    if (bucket == 2U) {
        return (cursor.TakeByte() & 1U) == 0U ? TakeRangeValue(cursor, 0x800U, 0xD7FFU) : TakeRangeValue(cursor, 0xE000U, 0xFFFFU);
    }
    return TakeRangeValue(cursor, 0x10000U, 0x10FFFFU);
}

zcore::u32 TakePossiblyInvalidScalar(FuzzCursor& cursor) noexcept
{
    return static_cast<zcore::u32>(cursor.TakeU32() % 0x120000U);
}

std::vector<char> BuildCStringInput(FuzzCursor& cursor, zcore::usize maxLen)
{
    const zcore::usize length = static_cast<zcore::usize>(cursor.TakeByte()) % (maxLen + 1U);
    std::vector<char> out(length + 1U, '\0');
    for (zcore::usize index = 0U; index < length; ++index) {
        out[index] = static_cast<char>(cursor.TakeByte());
    }
    return out;
}

std::vector<char> BuildValidUtf8Chunk(FuzzCursor& cursor, zcore::usize maxCodePoints, zcore::usize maxBytes)
{
    std::vector<char> chunk;
    const zcore::usize codePoints = static_cast<zcore::usize>(cursor.TakeByte()) % (maxCodePoints + 1U);
    for (zcore::usize index = 0U; index < codePoints; ++index) {
        char encoded[4] = {};
        const zcore::Option<zcore::usize> width = zcore::utf8::EncodeCodePoint(TakeValidScalar(cursor), encoded, 4U);
        if (!width.HasValue()) {
            break;
        }
        if (chunk.size() + width.Value() > maxBytes) {
            break;
        }
        chunk.insert(chunk.end(), encoded, encoded + width.Value());
    }
    return chunk;
}

zcore::StringView AsStringView(const std::vector<char>& bytes)
{
    if (bytes.empty()) {
        return zcore::StringView::Empty();
    }
    return zcore::StringView::FromRawUnchecked(bytes.data(), static_cast<zcore::usize>(bytes.size()));
}

void CompareDynamic(const zcore::String& value, const Utf8Model& model)
{
    if (value.Size() != model.Size()) {
        FuzzAbort();
    }
    if (value.CStr()[value.Size()] != '\0') {
        FuzzAbort();
    }
    if (static_cast<zcore::usize>(value.AsStdStringView().size()) != model.Size()) {
        FuzzAbort();
    }
    if (value.IsValidUtf8() != model.IsValidUtf8()) {
        FuzzAbort();
    }

    const zcore::Option<zcore::usize> count = value.TryCodePointCount();
    const zcore::Option<zcore::usize> expectedCount = model.TryCodePointCount();
    if (count.HasValue() != expectedCount.HasValue()) {
        FuzzAbort();
    }
    if (count.HasValue() && count.Value() != expectedCount.Value()) {
        FuzzAbort();
    }

    const auto& bytes = model.Bytes();
    if (bytes.empty()) {
        return;
    }
    const char* const data = value.Data();
    if (data == nullptr) {
        FuzzAbort();
    }
    for (zcore::usize index = 0U; index < model.Size(); ++index) {
        if (data[index] != bytes[index]) {
            FuzzAbort();
        }
    }
}

void CompareFixed(const zcore::FixedString<64U>& value, const Utf8Model& model)
{
    if (value.Size() != model.Size()) {
        FuzzAbort();
    }
    if (value.CStr()[value.Size()] != '\0') {
        FuzzAbort();
    }
    if (value.IsValidUtf8() != model.IsValidUtf8()) {
        FuzzAbort();
    }

    const zcore::Option<zcore::usize> count = value.TryCodePointCount();
    const zcore::Option<zcore::usize> expectedCount = model.TryCodePointCount();
    if (count.HasValue() != expectedCount.HasValue()) {
        FuzzAbort();
    }
    if (count.HasValue() && count.Value() != expectedCount.Value()) {
        FuzzAbort();
    }

    const auto& bytes = model.Bytes();
    for (zcore::usize index = 0U; index < model.Size(); ++index) {
        if (value[index] != bytes[index]) {
            FuzzAbort();
        }
    }
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    FuzzCursor cursor(data, size);
    FuzzAllocator allocator;

    zcore::String dynamicValue(allocator);
    zcore::FixedString<64U> fixedValue;
    Utf8Model dynamicModel(std::numeric_limits<zcore::usize>::max());
    Utf8Model fixedModel(64U);

    const zcore::usize steps = static_cast<zcore::usize>((cursor.TakeByte() % 224U) + 96U);
    for (zcore::usize step = 0U; step < steps; ++step) {
        const std::uint8_t op = static_cast<std::uint8_t>(cursor.TakeByte() % 14U);

        if (op == 0U) {
            const std::vector<char> cstring = BuildCStringInput(cursor, 24U);
            const bool dynamicExpected = dynamicModel.TryAssignCString(cstring.data());
            const zcore::Status dynamicStatus = dynamicValue.TryAssignCString(cstring.data());
            if (dynamicStatus.HasValue() != dynamicExpected) {
                FuzzAbort();
            }

            const bool fixedExpected = fixedModel.TryAssignCString(cstring.data());
            const bool fixedResult = fixedValue.TryAssignCString(cstring.data());
            if (fixedResult != fixedExpected) {
                FuzzAbort();
            }
        }
        else if (op == 1U) {
            if (dynamicValue.TryAssignCString(nullptr).HasValue()) {
                FuzzAbort();
            }
            if (fixedValue.TryAssignCString(nullptr)) {
                FuzzAbort();
            }
        }
        else if (op == 2U) {
            const zcore::u32 scalar = TakePossiblyInvalidScalar(cursor);
            const bool dynamicExpected = dynamicModel.TryAppendCodePoint(scalar);
            const zcore::Status dynamicStatus = dynamicValue.TryAppendCodePoint(scalar);
            if (dynamicStatus.HasValue() != dynamicExpected) {
                FuzzAbort();
            }

            const bool fixedExpected = fixedModel.TryAppendCodePoint(scalar);
            const bool fixedResult = fixedValue.TryAppendCodePoint(scalar);
            if (fixedResult != fixedExpected) {
                FuzzAbort();
            }
        }
        else if (op == 3U) {
            const char value = static_cast<char>(cursor.TakeByte());
            const bool dynamicExpected = dynamicModel.TryPushBack(value);
            const zcore::Status dynamicStatus = dynamicValue.TryPushBack(value);
            if (dynamicStatus.HasValue() != dynamicExpected) {
                FuzzAbort();
            }

            const bool fixedExpected = fixedModel.TryPushBack(value);
            const bool fixedResult = fixedValue.TryPushBack(value);
            if (fixedResult != fixedExpected) {
                FuzzAbort();
            }
        }
        else if (op == 4U) {
            const zcore::usize count = static_cast<zcore::usize>(cursor.TakeByte() % 8U);
            if (dynamicValue.RemovePrefix(count) != dynamicModel.RemovePrefix(count)) {
                FuzzAbort();
            }
            if (fixedValue.RemovePrefix(count) != fixedModel.RemovePrefix(count)) {
                FuzzAbort();
            }
        }
        else if (op == 5U) {
            const zcore::usize count = static_cast<zcore::usize>(cursor.TakeByte() % 8U);
            if (dynamicValue.RemoveSuffix(count) != dynamicModel.RemoveSuffix(count)) {
                FuzzAbort();
            }
            if (fixedValue.RemoveSuffix(count) != fixedModel.RemoveSuffix(count)) {
                FuzzAbort();
            }
        }
        else if (op == 6U) {
            const zcore::usize count = static_cast<zcore::usize>(cursor.TakeByte() % 16U);
            if (dynamicValue.RemovePrefixBytes(count) != dynamicModel.RemovePrefixBytes(count)) {
                FuzzAbort();
            }
            if (fixedValue.RemovePrefixBytes(count) != fixedModel.RemovePrefixBytes(count)) {
                FuzzAbort();
            }
        }
        else if (op == 7U) {
            const zcore::usize count = static_cast<zcore::usize>(cursor.TakeByte() % 16U);
            if (dynamicValue.RemoveSuffixBytes(count) != dynamicModel.RemoveSuffixBytes(count)) {
                FuzzAbort();
            }
            if (fixedValue.RemoveSuffixBytes(count) != fixedModel.RemoveSuffixBytes(count)) {
                FuzzAbort();
            }
        }
        else if (op == 8U) {
            if (dynamicValue.TryPopBack() != dynamicModel.TryPopBack()) {
                FuzzAbort();
            }
            if (fixedValue.TryPopBack() != fixedModel.TryPopBack()) {
                FuzzAbort();
            }
        }
        else if (op == 9U) {
            const std::vector<char> chunk = BuildValidUtf8Chunk(cursor, 6U, 24U);
            const zcore::StringView view = AsStringView(chunk);

            const bool dynamicExpected = dynamicModel.TryAppendView(view);
            const zcore::Status dynamicStatus = dynamicValue.TryAppend(view);
            if (dynamicStatus.HasValue() != dynamicExpected) {
                FuzzAbort();
            }

            const bool fixedExpected = fixedModel.TryAppendView(view);
            const bool fixedResult = fixedValue.TryAppend(view);
            if (fixedResult != fixedExpected) {
                FuzzAbort();
            }
        }
        else if (op == 10U) {
            const std::vector<char> chunk = BuildValidUtf8Chunk(cursor, 6U, 24U);
            const zcore::StringView view = AsStringView(chunk);

            const bool dynamicExpected = dynamicModel.TryAssignView(view);
            const zcore::Status dynamicStatus = dynamicValue.TryAssignUtf8(view);
            if (dynamicStatus.HasValue() != dynamicExpected) {
                FuzzAbort();
            }

            const bool fixedExpected = fixedModel.TryAssignView(view);
            const bool fixedResult = fixedValue.TryAssignUtf8(view);
            if (fixedResult != fixedExpected) {
                FuzzAbort();
            }
        }
        else if (op == 11U) {
            dynamicValue.Clear();
            fixedValue.Clear();
            dynamicModel.Clear();
            fixedModel.Clear();
        }
        else if (op == 12U) {
            dynamicValue.Reset();
            fixedValue.Clear();
            dynamicModel.Clear();
            fixedModel.Clear();
        }
        else {
            const zcore::usize capacity = static_cast<zcore::usize>(cursor.TakeByte() % 96U);
            if (dynamicValue.TryReserve(capacity).HasError()) {
                FuzzAbort();
            }
        }

        CompareDynamic(dynamicValue, dynamicModel);
        CompareFixed(fixedValue, fixedModel);
    }

    return 0;
}
