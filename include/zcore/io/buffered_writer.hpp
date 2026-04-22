/**************************************************************************/
/*  buffered_writer.hpp                                                   */
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
 * @file include/zcore/io/buffered_writer.hpp
 * @brief Buffered byte-writer adapter over `Writer`.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/buffered_writer.hpp>
 * zcore::BufferedWriter writer(sink, allocator, 4096U);
 * @endcode
 */

#pragma once

#include <zcore/io/byte_buffer.hpp>
#include <zcore/io/io_error.hpp>
#include <zcore/io/writer.hpp>
#include <zcore/result.hpp>
#include <zcore/status.hpp>

namespace zcore {

/**
 * @brief Allocator-backed buffered adapter for `Writer`.
 *
 * `BufferedWriter` accumulates small writes in an internal byte buffer and
 * flushes to an upstream `Writer`.
 */
class BufferedWriter final : public Writer {
public:
    static constexpr usize kDefaultCapacity = 4096U;

    /**
   * @brief Constructs a buffered writer over `sink`.
   * @param sink Upstream writer.
   * @param allocator Allocator used for internal byte storage.
   * @param capacity Target internal buffer capacity in bytes.
   */
    BufferedWriter(Writer& sink, Allocator& allocator, usize capacity = kDefaultCapacity) noexcept
            : Sink_(&sink), Buffer_(allocator), Capacity_(capacity)
    {
    }

    BufferedWriter(const BufferedWriter&) = delete;
    BufferedWriter& operator=(const BufferedWriter&) = delete;
    BufferedWriter(BufferedWriter&&) = delete;
    BufferedWriter& operator=(BufferedWriter&&) = delete;
    ~BufferedWriter() override = default;

    /// @brief Returns configured internal buffer capacity.
    [[nodiscard]] usize Capacity() const noexcept
    {
        return Capacity_;
    }

    /// @brief Returns count of currently buffered unwritten bytes.
    [[nodiscard]] usize BufferedSize() const noexcept
    {
        return Buffer_.Size();
    }

    /**
   * @brief Writes bytes from source.
   * @return Number of bytes accepted from source or error.
   */
    [[nodiscard]] Result<usize, Error> Write(ByteSpan source) noexcept override
    {
        if (source.EmptyView()) {
            return Result<usize, Error>::Success(0U);
        }

        if (Capacity_ == 0U) {
            return Sink_->Write(source);
        }

        usize totalWritten = 0U;
        while (totalWritten < source.Size()) {
            const usize remaining = source.Size() - totalWritten;
            if (Buffer_.Empty() && remaining >= Capacity_) {
                const ByteSpan direct = ByteSpan::FromRawUnchecked(source.Data() + totalWritten, remaining);
                auto directResult = Sink_->Write(direct);
                if (directResult.HasError()) {
                    return Result<usize, Error>::Failure(directResult.Error());
                }
                totalWritten += directResult.Value();
                if (directResult.Value() == 0U) {
                    break;
                }
                continue;
            }

            Status ensureStatus = EnsureBufferStorage();
            if (ensureStatus.HasError()) {
                return Result<usize, Error>::Failure(ensureStatus.Error());
            }

            const usize available = Capacity_ - Buffer_.Size();
            if (available == 0U) {
                Status flushStatus = FlushBuffer();
                if (flushStatus.HasError()) {
                    return Result<usize, Error>::Failure(flushStatus.Error());
                }
                continue;
            }

            const usize toCopy = remaining < available ? remaining : available;
            const ByteSpan chunk = ByteSpan::FromRawUnchecked(source.Data() + totalWritten, toCopy);
            Status appendStatus = Buffer_.TryAppend(chunk);
            if (appendStatus.HasError()) {
                return Result<usize, Error>::Failure(appendStatus.Error());
            }
            totalWritten += toCopy;

            if (Buffer_.Size() == Capacity_) {
                Status flushStatus = FlushBuffer();
                if (flushStatus.HasError()) {
                    return Result<usize, Error>::Failure(flushStatus.Error());
                }
            }
        }

        return Result<usize, Error>::Success(totalWritten);
    }

    /**
   * @brief Flushes buffered bytes and delegates sink flush.
   * @return Success or propagated error.
   */
    [[nodiscard]] Status Flush() noexcept override
    {
        Status flushStatus = FlushBuffer();
        if (flushStatus.HasError()) {
            return flushStatus;
        }
        return Sink_->Flush();
    }

private:
    [[nodiscard]] Status EnsureBufferStorage() noexcept
    {
        if (Buffer_.Capacity() >= Capacity_) {
            return OkStatus();
        }
        return Buffer_.TryReserve(Capacity_);
    }

    [[nodiscard]] Status FlushBuffer() noexcept
    {
        if (Buffer_.Empty()) {
            return OkStatus();
        }

        usize written = 0U;
        while (written < Buffer_.Size()) {
            const usize remaining = Buffer_.Size() - written;
            const ByteSpan pending = ByteSpan::FromRawUnchecked(Buffer_.Data() + written, remaining);
            auto writeResult = Sink_->Write(pending);
            if (writeResult.HasError()) {
                return ErrorStatus(writeResult.Error());
            }
            if (writeResult.Value() == 0U) {
                return ErrorStatus(
                        MakeIoError(IoErrorCode::END_OF_STREAM, "write", "upstream writer returned zero during buffered flush"));
            }
            written += writeResult.Value();
        }

        Buffer_.Clear();
        return OkStatus();
    }

    Writer* Sink_;
    ByteBuffer Buffer_;
    usize Capacity_;
};

} // namespace zcore
