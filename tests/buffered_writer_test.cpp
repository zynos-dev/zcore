/**************************************************************************/
/*  buffered_writer_test.cpp                                              */
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
 * @file tests/buffered_writer_test.cpp
 * @brief Unit tests for allocator-backed `BufferedWriter` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/buffered_writer.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <new>

namespace {

class TrackingAllocator final : public zcore::Allocator {
 public:
  bool FailAllocation = false;
  int AllocateCalls = 0;
  int DeallocateCalls = 0;

  [[nodiscard]] zcore::Result<zcore::Allocation, zcore::Error> Allocate(
      zcore::AllocationRequest request) noexcept override {
    ++AllocateCalls;
    const zcore::Status requestStatus = zcore::ValidateAllocationRequest(request);
    if (requestStatus.HasError()) {
      return zcore::Result<zcore::Allocation, zcore::Error>::Failure(requestStatus.Error());
    }
    if (request.size == 0U) {
      return zcore::Result<zcore::Allocation, zcore::Error>::Success(zcore::Allocation::Empty());
    }
    if (FailAllocation) {
      return zcore::Result<zcore::Allocation, zcore::Error>::Failure(
          zcore::MakeAllocatorError(zcore::AllocatorErrorCode::OUT_OF_MEMORY, "allocate", "forced failure"));
    }

    void* const raw =
        ::operator new(request.size, static_cast<std::align_val_t>(request.alignment), std::nothrow);
    if (raw == nullptr) {
      return zcore::Result<zcore::Allocation, zcore::Error>::Failure(
          zcore::MakeAllocatorError(zcore::AllocatorErrorCode::OUT_OF_MEMORY, "allocate", "system allocator returned null"));
    }

    return zcore::Result<zcore::Allocation, zcore::Error>::Success(zcore::Allocation{
        .data = static_cast<zcore::Byte*>(raw),
        .size = request.size,
        .alignment = request.alignment,
    });
  }

  [[nodiscard]] zcore::Status Deallocate(zcore::Allocation allocation) noexcept override {
    ++DeallocateCalls;
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

class MemoryWriter final : public zcore::Writer {
 public:
  explicit MemoryWriter(zcore::ByteSpanMut destination, zcore::usize maxPerWrite = 1024U) noexcept
      : Destination_(destination),
        MaxPerWrite_(maxPerWrite == 0U ? 1U : maxPerWrite) {}

  [[nodiscard]] zcore::Result<zcore::usize, zcore::Error> Write(zcore::ByteSpan source) noexcept override {
    ++WriteCalls;
    if (FailWrite || (FailWriteAfterCalls >= 0 && WriteCalls > FailWriteAfterCalls)) {
      return zcore::Result<zcore::usize, zcore::Error>::Failure(
          zcore::MakeIoError(zcore::IoErrorCode::UNSUPPORTED_OPERATION, "write", "forced failure"));
    }
    if (ReturnZeroOnWrite) {
      return zcore::Result<zcore::usize, zcore::Error>::Success(0U);
    }
    if (source.EmptyView()) {
      return zcore::Result<zcore::usize, zcore::Error>::Success(0U);
    }

    const zcore::usize destinationRemaining =
        Cursor_ < Destination_.Size() ? (Destination_.Size() - Cursor_) : 0U;
    const zcore::usize sourceLimit = source.Size() < MaxPerWrite_ ? source.Size() : MaxPerWrite_;
    const zcore::usize toWrite = destinationRemaining < sourceLimit ? destinationRemaining : sourceLimit;
    for (zcore::usize index = 0U; index < toWrite; ++index) {
      Destination_[Cursor_ + index] = source[index];
    }
    Cursor_ += toWrite;
    return zcore::Result<zcore::usize, zcore::Error>::Success(toWrite);
  }

  [[nodiscard]] zcore::Status Flush() noexcept override {
    ++FlushCalls;
    if (FailFlush) {
      return zcore::ErrorStatus(
          zcore::MakeIoError(zcore::IoErrorCode::UNSUPPORTED_OPERATION, "flush", "forced failure"));
    }
    return zcore::OkStatus();
  }

  bool FailWrite = false;
  int FailWriteAfterCalls = -1;
  bool FailFlush = false;
  bool ReturnZeroOnWrite = false;
  int WriteCalls = 0;
  int FlushCalls = 0;

  [[nodiscard]] zcore::usize BytesWritten() const noexcept { return Cursor_; }

 private:
  zcore::ByteSpanMut Destination_;
  zcore::usize Cursor_{0U};
  zcore::usize MaxPerWrite_;
};

[[nodiscard]] unsigned int ByteValue(zcore::Byte value) {
  return std::to_integer<unsigned int>(value);
}

TEST(BufferedWriterTest, WriteBuffersUntilExplicitFlush) {
  TrackingAllocator allocator;
  std::array<zcore::Byte, 8U> storage{};
  MemoryWriter sink{zcore::ByteSpanMut(storage)};
  zcore::BufferedWriter writer(sink, allocator, 4U);

  const std::array<zcore::Byte, 3U> source{
      static_cast<zcore::Byte>(0x10U),
      static_cast<zcore::Byte>(0x11U),
      static_cast<zcore::Byte>(0x12U),
  };
  auto writeResult = writer.Write(zcore::ByteSpan(source));
  ASSERT_TRUE(writeResult.HasValue());
  EXPECT_EQ(writeResult.Value(), 3U);
  EXPECT_EQ(writer.BufferedSize(), 3U);
  EXPECT_EQ(sink.WriteCalls, 0);

  const zcore::Status flushStatus = writer.Flush();
  EXPECT_TRUE(flushStatus.HasValue());
  EXPECT_EQ(writer.BufferedSize(), 0U);
  EXPECT_EQ(sink.WriteCalls, 1);
  EXPECT_EQ(sink.FlushCalls, 1);
  EXPECT_EQ(ByteValue(storage[0]), 0x10U);
  EXPECT_EQ(ByteValue(storage[1]), 0x11U);
  EXPECT_EQ(ByteValue(storage[2]), 0x12U);
}

TEST(BufferedWriterTest, WriteFlushesWhenBufferReachesCapacity) {
  TrackingAllocator allocator;
  std::array<zcore::Byte, 8U> storage{};
  MemoryWriter sink{zcore::ByteSpanMut(storage)};
  zcore::BufferedWriter writer(sink, allocator, 4U);

  const std::array<zcore::Byte, 2U> first{
      static_cast<zcore::Byte>(0x21U),
      static_cast<zcore::Byte>(0x22U),
  };
  const std::array<zcore::Byte, 3U> second{
      static_cast<zcore::Byte>(0x23U),
      static_cast<zcore::Byte>(0x24U),
      static_cast<zcore::Byte>(0x25U),
  };

  ASSERT_TRUE(writer.Write(zcore::ByteSpan(first)).HasValue());
  ASSERT_TRUE(writer.Write(zcore::ByteSpan(second)).HasValue());
  EXPECT_EQ(sink.WriteCalls, 1);
  EXPECT_EQ(writer.BufferedSize(), 1U);
  EXPECT_EQ(ByteValue(storage[0]), 0x21U);
  EXPECT_EQ(ByteValue(storage[3]), 0x24U);

  ASSERT_TRUE(writer.Flush().HasValue());
  EXPECT_EQ(writer.BufferedSize(), 0U);
  EXPECT_EQ(sink.WriteCalls, 2);
  EXPECT_EQ(ByteValue(storage[4]), 0x25U);
}

TEST(BufferedWriterTest, LargeWriteBypassesBufferWhenEmpty) {
  TrackingAllocator allocator;
  std::array<zcore::Byte, 16U> storage{};
  MemoryWriter sink{zcore::ByteSpanMut(storage), 16U};
  zcore::BufferedWriter writer(sink, allocator, 4U);

  const std::array<zcore::Byte, 10U> source{
      static_cast<zcore::Byte>(0x30U),
      static_cast<zcore::Byte>(0x31U),
      static_cast<zcore::Byte>(0x32U),
      static_cast<zcore::Byte>(0x33U),
      static_cast<zcore::Byte>(0x34U),
      static_cast<zcore::Byte>(0x35U),
      static_cast<zcore::Byte>(0x36U),
      static_cast<zcore::Byte>(0x37U),
      static_cast<zcore::Byte>(0x38U),
      static_cast<zcore::Byte>(0x39U),
  };
  auto writeResult = writer.Write(zcore::ByteSpan(source));
  ASSERT_TRUE(writeResult.HasValue());
  EXPECT_EQ(writeResult.Value(), 10U);
  EXPECT_EQ(writer.BufferedSize(), 0U);
  EXPECT_EQ(sink.WriteCalls, 1);
  EXPECT_EQ(ByteValue(storage[0]), 0x30U);
  EXPECT_EQ(ByteValue(storage[9]), 0x39U);
}

TEST(BufferedWriterTest, CapacityZeroUsesPassthroughWritePath) {
  TrackingAllocator allocator;
  std::array<zcore::Byte, 8U> storage{};
  MemoryWriter sink{zcore::ByteSpanMut(storage)};
  zcore::BufferedWriter writer(sink, allocator, 0U);

  const std::array<zcore::Byte, 3U> source{
      static_cast<zcore::Byte>(0x40U),
      static_cast<zcore::Byte>(0x41U),
      static_cast<zcore::Byte>(0x42U),
  };
  auto writeResult = writer.Write(zcore::ByteSpan(source));
  ASSERT_TRUE(writeResult.HasValue());
  EXPECT_EQ(writeResult.Value(), 3U);
  EXPECT_EQ(sink.WriteCalls, 1);
  EXPECT_EQ(writer.BufferedSize(), 0U);
}

TEST(BufferedWriterTest, FlushPropagatesWriteAndFlushErrors) {
  TrackingAllocator allocator;
  std::array<zcore::Byte, 8U> storage{};
  MemoryWriter sink{zcore::ByteSpanMut(storage)};
  zcore::BufferedWriter writer(sink, allocator, 4U);

  const std::array<zcore::Byte, 2U> source{
      static_cast<zcore::Byte>(0x50U),
      static_cast<zcore::Byte>(0x51U),
  };
  ASSERT_TRUE(writer.Write(zcore::ByteSpan(source)).HasValue());

  sink.FailWrite = true;
  const zcore::Status writeFlushStatus = writer.Flush();
  ASSERT_TRUE(writeFlushStatus.HasError());
  EXPECT_EQ(
      writeFlushStatus.Error().code.value,
      static_cast<zcore::i32>(zcore::IoErrorCode::UNSUPPORTED_OPERATION));

  sink.FailWrite = false;
  sink.FailFlush = true;
  const zcore::Status flushStatus = writer.Flush();
  ASSERT_TRUE(flushStatus.HasError());
  EXPECT_EQ(
      flushStatus.Error().code.value,
      static_cast<zcore::i32>(zcore::IoErrorCode::UNSUPPORTED_OPERATION));

  sink.FailFlush = false;
  sink.ReturnZeroOnWrite = true;
  ASSERT_TRUE(writer.Write(zcore::ByteSpan(source)).HasValue());
  const zcore::Status zeroWriteStatus = writer.Flush();
  ASSERT_TRUE(zeroWriteStatus.HasError());
  EXPECT_EQ(
      zeroWriteStatus.Error().code.value,
      static_cast<zcore::i32>(zcore::IoErrorCode::END_OF_STREAM));
}

TEST(BufferedWriterTest, WriteReturnsPartialProgressOnDirectPathBeforeError) {
  TrackingAllocator allocator;
  std::array<zcore::Byte, 8U> storage{};
  MemoryWriter sink{zcore::ByteSpanMut(storage), 2U};
  sink.FailWriteAfterCalls = 1;
  zcore::BufferedWriter writer(sink, allocator, 4U);

  const std::array<zcore::Byte, 6U> source{
      static_cast<zcore::Byte>(0x60U),
      static_cast<zcore::Byte>(0x61U),
      static_cast<zcore::Byte>(0x62U),
      static_cast<zcore::Byte>(0x63U),
      static_cast<zcore::Byte>(0x64U),
      static_cast<zcore::Byte>(0x65U),
  };
  auto writeResult = writer.Write(zcore::ByteSpan(source));
  ASSERT_TRUE(writeResult.HasValue());
  EXPECT_EQ(writeResult.Value(), 2U);
  EXPECT_EQ(sink.BytesWritten(), 2U);
  EXPECT_EQ(ByteValue(storage[0]), 0x60U);
  EXPECT_EQ(ByteValue(storage[1]), 0x61U);
}

TEST(BufferedWriterTest, FlushDoesNotDuplicatePrefixAfterPartialProgressError) {
  TrackingAllocator allocator;
  std::array<zcore::Byte, 16U> storage{};
  MemoryWriter sink{zcore::ByteSpanMut(storage), 2U};
  zcore::BufferedWriter writer(sink, allocator, 4U);

  const std::array<zcore::Byte, 3U> first{
      static_cast<zcore::Byte>(0x70U),
      static_cast<zcore::Byte>(0x71U),
      static_cast<zcore::Byte>(0x72U),
  };
  auto firstWrite = writer.Write(zcore::ByteSpan(first));
  ASSERT_TRUE(firstWrite.HasValue());
  EXPECT_EQ(firstWrite.Value(), 3U);
  EXPECT_EQ(writer.BufferedSize(), 3U);

  sink.FailWriteAfterCalls = 1;
  const std::array<zcore::Byte, 3U> second{
      static_cast<zcore::Byte>(0x73U),
      static_cast<zcore::Byte>(0x74U),
      static_cast<zcore::Byte>(0x75U),
  };
  auto secondWrite = writer.Write(zcore::ByteSpan(second));
  ASSERT_TRUE(secondWrite.HasValue());
  EXPECT_EQ(secondWrite.Value(), 1U);
  EXPECT_EQ(writer.BufferedSize(), 2U);
  EXPECT_EQ(sink.BytesWritten(), 2U);
  EXPECT_EQ(ByteValue(storage[0]), 0x70U);
  EXPECT_EQ(ByteValue(storage[1]), 0x71U);

  sink.FailWriteAfterCalls = -1;
  const zcore::Status flushStatus = writer.Flush();
  ASSERT_TRUE(flushStatus.HasValue());
  EXPECT_EQ(writer.BufferedSize(), 0U);
  EXPECT_EQ(sink.BytesWritten(), 4U);
  EXPECT_EQ(ByteValue(storage[2]), 0x72U);
  EXPECT_EQ(ByteValue(storage[3]), 0x73U);
}

}  // namespace
