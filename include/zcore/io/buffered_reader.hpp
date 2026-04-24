/**************************************************************************/
/*  buffered_reader.hpp                                                   */
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
 * @file include/zcore/io/buffered_reader.hpp
 * @brief Buffered byte-reader adapter over `Reader`.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/buffered_reader.hpp>
 * zcore::BufferedReader reader(source, allocator, 4096U);
 * @endcode
 */

#pragma once

#include <zcore/io/byte_buffer.hpp>
#include <zcore/io/reader.hpp>
#include <zcore/result.hpp>

namespace zcore {

/**
 * @brief Allocator-backed buffered adapter for `Reader`.
 *
 * `BufferedReader` wraps a source `Reader` and serves `Read` calls from an
 * internal byte buffer to reduce upstream read call count.
 */
class BufferedReader final : public Reader {
public:
    static constexpr usize kDefaultCapacity = 4096U;

    /**
   * @brief Constructs a buffered reader over `source`.
   * @param source Upstream reader.
   * @param allocator Allocator used for internal byte storage.
   * @param capacity Target internal buffer capacity in bytes.
   */
    BufferedReader(Reader& source, Allocator& allocator, usize capacity = kDefaultCapacity) noexcept
            : Source_(&source), Buffer_(allocator), Capacity_(capacity)
    {
    }

    BufferedReader(const BufferedReader&) = delete;
    BufferedReader& operator=(const BufferedReader&) = delete;
    BufferedReader(BufferedReader&&) = delete;
    BufferedReader& operator=(BufferedReader&&) = delete;
    ~BufferedReader() override = default;

    /// @brief Returns configured internal buffer capacity.
    [[nodiscard]] usize Capacity() const noexcept
    {
        return Capacity_;
    }

    /// @brief Returns count of currently buffered unread bytes.
    [[nodiscard]] usize BufferedSize() const noexcept
    {
        return End_ >= Begin_ ? (End_ - Begin_) : 0U;
    }

    /// @brief Drops buffered unread bytes.
    void ClearBuffer() noexcept
    {
        Begin_ = 0U;
        End_ = 0U;
    }

    /**
   * @brief Reads bytes into destination.
   * @return Number of bytes read or I/O-domain/allocator-domain error.
   */
    [[nodiscard]] Result<usize, Error> Read(ByteSpanMut destination) noexcept override
    {
        if (destination.EmptyView()) {
            return Result<usize, Error>::Success(0U);
        }

        if (Capacity_ == 0U) {
            return Source_->Read(destination);
        }

        usize totalRead = 0U;
        while (totalRead < destination.Size()) {
            if (Begin_ == End_) {
                const usize remaining = destination.Size() - totalRead;
                if (remaining >= Capacity_) {
                    const ByteSpanMut direct = ByteSpanMut::FromRawUnchecked(destination.Data() + totalRead, remaining);
                    auto directResult = Source_->Read(direct);
                    if (directResult.HasError()) {
                        if (totalRead > 0U) {
                            return Result<usize, Error>::Success(totalRead);
                        }
                        return Result<usize, Error>::Failure(directResult.Error());
                    }
                    totalRead += directResult.Value();
                    if (directResult.Value() == 0U) {
                        break;
                    }
                    continue;
                }

                auto refillResult = Refill();
                if (refillResult.HasError()) {
                    if (totalRead > 0U) {
                        return Result<usize, Error>::Success(totalRead);
                    }
                    return Result<usize, Error>::Failure(refillResult.Error());
                }
                if (refillResult.Value() == 0U) {
                    break;
                }
            }

            const usize available = End_ - Begin_;
            const usize needed = destination.Size() - totalRead;
            const usize toCopy = available < needed ? available : needed;
            for (usize index = 0U; index < toCopy; ++index) {
                destination[totalRead + index] = Buffer_[Begin_ + index];
            }
            Begin_ += toCopy;
            totalRead += toCopy;
        }

        return Result<usize, Error>::Success(totalRead);
    }

private:
    [[nodiscard]] Status EnsureBufferStorage() noexcept
    {
        if (Buffer_.Capacity() < Capacity_) {
            Status reserveStatus = Buffer_.TryReserve(Capacity_);
            if (reserveStatus.HasError()) {
                return reserveStatus;
            }
        }
        if (Buffer_.Size() < Capacity_) {
            Status resizeStatus = Buffer_.TryResize(Capacity_);
            if (resizeStatus.HasError()) {
                return resizeStatus;
            }
        }
        return OkStatus();
    }

    [[nodiscard]] Result<usize, Error> Refill() noexcept
    {
        Status ensureStatus = EnsureBufferStorage();
        if (ensureStatus.HasError()) {
            return Result<usize, Error>::Failure(ensureStatus.Error());
        }

        const ByteSpanMut writable = ByteSpanMut::FromRawUnchecked(Buffer_.Data(), Capacity_);
        auto readResult = Source_->Read(writable);
        if (readResult.HasError()) {
            return Result<usize, Error>::Failure(readResult.Error());
        }

        Begin_ = 0U;
        End_ = readResult.Value();
        return Result<usize, Error>::Success(readResult.Value());
    }

    Reader* Source_;
    ByteBuffer Buffer_;
    usize Capacity_;
    usize Begin_{0U};
    usize End_{0U};
};

} // namespace zcore
