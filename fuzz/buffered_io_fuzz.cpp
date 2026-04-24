/**************************************************************************/
/*  buffered_io_fuzz.cpp                                                  */
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
 * @file fuzz/buffered_io_fuzz.cpp
 * @brief Local libFuzzer harness for buffered reader/writer invariants.
 */

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <vector>
#include <zcore/buffered_reader.hpp>
#include <zcore/buffered_writer.hpp>

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

class FuzzSourceReader final : public zcore::Reader {
public:
    FuzzSourceReader(std::vector<std::uint8_t> source, zcore::usize maxChunk, zcore::usize failEveryRead) noexcept
            : Source_(std::move(source)), MaxChunk_(maxChunk == 0U ? 1U : maxChunk), FailEveryRead_(failEveryRead)
    {
    }

    [[nodiscard]] zcore::Result<zcore::usize, zcore::Error> Read(zcore::ByteSpanMut destination) noexcept override
    {
        ++ReadCalls_;
        if (FailEveryRead_ > 0U && (ReadCalls_ % FailEveryRead_) == 0U) {
            return zcore::Result<zcore::usize, zcore::Error>::Failure(
                    zcore::MakeIoError(zcore::IoErrorCode::UNSUPPORTED_OPERATION, "read", "fuzz reader injected failure"));
        }
        if (destination.EmptyView()) {
            return zcore::Result<zcore::usize, zcore::Error>::Success(0U);
        }

        const zcore::usize remaining = Cursor_ < Source_.size() ? static_cast<zcore::usize>(Source_.size() - Cursor_) : 0U;
        const zcore::usize toRead = std::min(remaining, std::min(destination.Size(), MaxChunk_));
        for (zcore::usize index = 0U; index < toRead; ++index) {
            destination[index] = static_cast<zcore::Byte>(Source_[Cursor_ + index]);
        }
        Cursor_ += toRead;
        return zcore::Result<zcore::usize, zcore::Error>::Success(toRead);
    }

    void DisableFailures() noexcept
    {
        FailEveryRead_ = 0U;
    }

    [[nodiscard]] const std::vector<std::uint8_t>& Source() const noexcept
    {
        return Source_;
    }

private:
    std::vector<std::uint8_t> Source_;
    zcore::usize Cursor_{0U};
    zcore::usize MaxChunk_{1U};
    zcore::usize FailEveryRead_{0U};
    zcore::usize ReadCalls_{0U};
};

class FuzzSinkWriter final : public zcore::Writer {
public:
    FuzzSinkWriter(zcore::usize maxChunk, zcore::usize failEveryFlush) noexcept
            : MaxChunk_(maxChunk == 0U ? 1U : maxChunk), FailEveryFlush_(failEveryFlush)
    {
    }

    [[nodiscard]] zcore::Result<zcore::usize, zcore::Error> Write(zcore::ByteSpan source) noexcept override
    {
        if (source.EmptyView()) {
            return zcore::Result<zcore::usize, zcore::Error>::Success(0U);
        }

        const zcore::usize toWrite = std::min(source.Size(), MaxChunk_);
        if ((OutputSize_ + toWrite) > kMaxOutputBytes) {
            return zcore::Result<zcore::usize, zcore::Error>::Failure(
                    zcore::MakeIoError(zcore::IoErrorCode::UNSUPPORTED_OPERATION, "write", "fuzz sink output capacity exceeded"));
        }
        for (zcore::usize index = 0U; index < toWrite; ++index) {
            Output_[OutputSize_ + index] = std::to_integer<std::uint8_t>(source[index]);
        }
        OutputSize_ += toWrite;
        return zcore::Result<zcore::usize, zcore::Error>::Success(toWrite);
    }

    [[nodiscard]] zcore::Status Flush() noexcept override
    {
        ++FlushCalls_;
        if (FailEveryFlush_ > 0U && (FlushCalls_ % FailEveryFlush_) == 0U) {
            return zcore::ErrorStatus(
                    zcore::MakeIoError(zcore::IoErrorCode::UNSUPPORTED_OPERATION, "flush", "fuzz writer injected failure"));
        }
        return zcore::OkStatus();
    }

    void DisableFailures() noexcept
    {
        FailEveryFlush_ = 0U;
    }

    [[nodiscard]] zcore::usize OutputSize() const noexcept
    {
        return OutputSize_;
    }

    [[nodiscard]] std::uint8_t OutputAt(zcore::usize index) const noexcept
    {
        return Output_[index];
    }

private:
    static constexpr zcore::usize kMaxOutputBytes = 1U << 15U;

    std::array<std::uint8_t, kMaxOutputBytes> Output_{};
    zcore::usize OutputSize_{0U};
    zcore::usize MaxChunk_{1U};
    zcore::usize FailEveryFlush_{0U};
    zcore::usize FlushCalls_{0U};
};

zcore::ByteSpan BytesAsSpan(const std::vector<zcore::Byte>& buffer) noexcept
{
    if (buffer.empty()) {
        return zcore::ByteSpan();
    }
    return zcore::ByteSpan::FromRawUnchecked(buffer.data(), static_cast<zcore::usize>(buffer.size()));
}

zcore::ByteSpanMut BytesAsSpanMut(std::vector<zcore::Byte>& buffer) noexcept
{
    if (buffer.empty()) {
        return zcore::ByteSpanMut();
    }
    return zcore::ByteSpanMut::FromRawUnchecked(buffer.data(), static_cast<zcore::usize>(buffer.size()));
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    FuzzCursor cursor(data, size);
    FuzzAllocator allocator;

    const zcore::usize sourceLength = static_cast<zcore::usize>((cursor.TakeByte() % 160U) + 32U);
    std::vector<std::uint8_t> source;
    source.reserve(sourceLength);
    for (zcore::usize index = 0U; index < sourceLength; ++index) {
        source.push_back(cursor.TakeByte());
    }

    const zcore::usize readerCapacity = static_cast<zcore::usize>(cursor.TakeByte() % 17U);
    const zcore::usize readerChunk = static_cast<zcore::usize>((cursor.TakeByte() % 16U) + 1U);
    const zcore::usize readerFailEvery =
            (cursor.TakeByte() % 4U == 0U) ? static_cast<zcore::usize>((cursor.TakeByte() % 5U) + 2U) : 0U;
    FuzzSourceReader sourceReader(std::move(source), readerChunk, readerFailEvery);
    zcore::BufferedReader reader(sourceReader, allocator, readerCapacity);

    std::vector<std::uint8_t> observed;
    const zcore::usize readerSteps = static_cast<zcore::usize>((cursor.TakeByte() % 192U) + 64U);
    for (zcore::usize step = 0U; step < readerSteps; ++step) {
        std::vector<zcore::Byte> destination(static_cast<zcore::usize>(cursor.TakeByte() % 32U));
        const auto readResult = reader.Read(BytesAsSpanMut(destination));
        if (readResult.HasError()) {
            continue;
        }
        if (readResult.Value() > destination.size()) {
            FuzzAbort();
        }
        for (zcore::usize index = 0U; index < readResult.Value(); ++index) {
            observed.push_back(std::to_integer<std::uint8_t>(destination[index]));
        }
    }

    sourceReader.DisableFailures();
    for (zcore::usize guard = 0U; guard < 512U; ++guard) {
        std::vector<zcore::Byte> destination(32U);
        const auto readResult = reader.Read(BytesAsSpanMut(destination));
        if (readResult.HasError()) {
            FuzzAbort();
        }
        if (readResult.Value() == 0U) {
            break;
        }
        for (zcore::usize index = 0U; index < readResult.Value(); ++index) {
            observed.push_back(std::to_integer<std::uint8_t>(destination[index]));
        }
    }

    const auto& expectedSource = sourceReader.Source();
    if (observed != expectedSource) {
        FuzzAbort();
    }

    const zcore::usize writerCapacity = static_cast<zcore::usize>(cursor.TakeByte() % 24U);
    const zcore::usize writerChunk = static_cast<zcore::usize>((cursor.TakeByte() % 16U) + 1U);
    const zcore::usize flushFailEvery =
            (cursor.TakeByte() % 5U == 0U) ? static_cast<zcore::usize>((cursor.TakeByte() % 7U) + 2U) : 0U;
    FuzzSinkWriter sink(writerChunk, flushFailEvery);
    zcore::BufferedWriter writer(sink, allocator, writerCapacity);

    std::vector<std::uint8_t> expectedOutput;
    const zcore::usize writerSteps = static_cast<zcore::usize>((cursor.TakeByte() % 192U) + 64U);
    for (zcore::usize step = 0U; step < writerSteps; ++step) {
        if ((cursor.TakeByte() % 3U) == 0U) {
            (void) writer.Flush();
            continue;
        }

        std::vector<zcore::Byte> payload(static_cast<zcore::usize>(cursor.TakeByte() % 32U));
        for (zcore::usize index = 0U; index < payload.size(); ++index) {
            payload[index] = static_cast<zcore::Byte>(cursor.TakeByte());
        }

        const auto writeResult = writer.Write(BytesAsSpan(payload));
        if (writeResult.HasError()) {
            continue;
        }
        if (writeResult.Value() > payload.size()) {
            FuzzAbort();
        }
        for (zcore::usize index = 0U; index < writeResult.Value(); ++index) {
            expectedOutput.push_back(std::to_integer<std::uint8_t>(payload[index]));
        }
    }

    sink.DisableFailures();
    if (writer.Flush().HasError()) {
        FuzzAbort();
    }
    if (sink.OutputSize() != expectedOutput.size()) {
        FuzzAbort();
    }
    for (zcore::usize index = 0U; index < sink.OutputSize(); ++index) {
        if (sink.OutputAt(index) != expectedOutput[index]) {
            FuzzAbort();
        }
    }

    if (writer.BufferedSize() > writer.Capacity()) {
        FuzzAbort();
    }

    return 0;
}
