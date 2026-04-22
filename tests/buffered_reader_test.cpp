/**************************************************************************/
/*  buffered_reader_test.cpp                                              */
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
 * @file tests/buffered_reader_test.cpp
 * @brief Unit tests for allocator-backed `BufferedReader` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/buffered_reader.hpp>

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

class ChunkedReader final : public zcore::Reader {
 public:
  ChunkedReader(
      zcore::ByteSpan source,
      zcore::usize maxChunk,
      bool failRead,
      int failAfterCalls) noexcept
      : Source_(source),
        MaxChunk_(maxChunk == 0U ? 1U : maxChunk),
        FailRead_(failRead),
        FailAfterCalls_(failAfterCalls) {}

  [[nodiscard]] zcore::Result<zcore::usize, zcore::Error> Read(zcore::ByteSpanMut destination) noexcept override {
    ++ReadCalls;
    if (FailRead_ && ReadCalls > FailAfterCalls_) {
      return zcore::Result<zcore::usize, zcore::Error>::Failure(
          zcore::MakeIoError(zcore::IoErrorCode::UNSUPPORTED_OPERATION, "read", "forced failure"));
    }
    if (destination.EmptyView()) {
      return zcore::Result<zcore::usize, zcore::Error>::Success(0U);
    }

    const zcore::usize remaining = Cursor_ < Source_.Size() ? (Source_.Size() - Cursor_) : 0U;
    const zcore::usize destinationLimit = destination.Size() < MaxChunk_ ? destination.Size() : MaxChunk_;
    const zcore::usize toRead = remaining < destinationLimit ? remaining : destinationLimit;
    for (zcore::usize index = 0U; index < toRead; ++index) {
      destination[index] = Source_[Cursor_ + index];
    }
    Cursor_ += toRead;
    return zcore::Result<zcore::usize, zcore::Error>::Success(toRead);
  }

  int ReadCalls = 0;

 private:
  zcore::ByteSpan Source_;
  zcore::usize Cursor_{0U};
  zcore::usize MaxChunk_;
  bool FailRead_;
  int FailAfterCalls_;
};

[[nodiscard]] unsigned int ByteValue(zcore::Byte value) {
  return std::to_integer<unsigned int>(value);
}

TEST(BufferedReaderTest, ReadsAcrossInternalRefillsAndPreservesOrder) {
  TrackingAllocator allocator;
  const std::array<zcore::Byte, 10U> source{
      static_cast<zcore::Byte>(0x10U),
      static_cast<zcore::Byte>(0x11U),
      static_cast<zcore::Byte>(0x12U),
      static_cast<zcore::Byte>(0x13U),
      static_cast<zcore::Byte>(0x14U),
      static_cast<zcore::Byte>(0x15U),
      static_cast<zcore::Byte>(0x16U),
      static_cast<zcore::Byte>(0x17U),
      static_cast<zcore::Byte>(0x18U),
      static_cast<zcore::Byte>(0x19U),
  };
  ChunkedReader sourceReader(zcore::ByteSpan(source), 2U, false, 0);
  zcore::BufferedReader reader(sourceReader, allocator, 4U);

  std::array<zcore::Byte, 7U> first{};
  auto firstRead = reader.Read(zcore::ByteSpanMut(first));
  ASSERT_TRUE(firstRead.HasValue());
  EXPECT_EQ(firstRead.Value(), 7U);
  EXPECT_EQ(ByteValue(first[0]), 0x10U);
  EXPECT_EQ(ByteValue(first[6]), 0x16U);

  std::array<zcore::Byte, 7U> second{};
  auto secondRead = reader.Read(zcore::ByteSpanMut(second));
  ASSERT_TRUE(secondRead.HasValue());
  EXPECT_EQ(secondRead.Value(), 3U);
  EXPECT_EQ(ByteValue(second[0]), 0x17U);
  EXPECT_EQ(ByteValue(second[1]), 0x18U);
  EXPECT_EQ(ByteValue(second[2]), 0x19U);

  auto thirdRead = reader.Read(zcore::ByteSpanMut(second));
  ASSERT_TRUE(thirdRead.HasValue());
  EXPECT_EQ(thirdRead.Value(), 0U);
}

TEST(BufferedReaderTest, ClearBufferDropsPrefetchedBytes) {
  TrackingAllocator allocator;
  const std::array<zcore::Byte, 10U> source{
      static_cast<zcore::Byte>(0x20U),
      static_cast<zcore::Byte>(0x21U),
      static_cast<zcore::Byte>(0x22U),
      static_cast<zcore::Byte>(0x23U),
      static_cast<zcore::Byte>(0x24U),
      static_cast<zcore::Byte>(0x25U),
      static_cast<zcore::Byte>(0x26U),
      static_cast<zcore::Byte>(0x27U),
      static_cast<zcore::Byte>(0x28U),
      static_cast<zcore::Byte>(0x29U),
  };
  ChunkedReader sourceReader(zcore::ByteSpan(source), 8U, false, 0);
  zcore::BufferedReader reader(sourceReader, allocator, 8U);

  std::array<zcore::Byte, 3U> first{};
  auto firstRead = reader.Read(zcore::ByteSpanMut(first));
  ASSERT_TRUE(firstRead.HasValue());
  EXPECT_EQ(firstRead.Value(), 3U);
  EXPECT_EQ(reader.BufferedSize(), 5U);

  reader.ClearBuffer();
  EXPECT_EQ(reader.BufferedSize(), 0U);

  std::array<zcore::Byte, 8U> second{};
  auto secondRead = reader.Read(zcore::ByteSpanMut(second));
  ASSERT_TRUE(secondRead.HasValue());
  EXPECT_EQ(secondRead.Value(), 2U);
  EXPECT_EQ(ByteValue(second[0]), 0x28U);
  EXPECT_EQ(ByteValue(second[1]), 0x29U);
}

TEST(BufferedReaderTest, CapacityZeroUsesPassthroughReadPath) {
  TrackingAllocator allocator;
  const std::array<zcore::Byte, 4U> source{
      static_cast<zcore::Byte>(0xA0U),
      static_cast<zcore::Byte>(0xA1U),
      static_cast<zcore::Byte>(0xA2U),
      static_cast<zcore::Byte>(0xA3U),
  };
  ChunkedReader sourceReader(zcore::ByteSpan(source), 4U, false, 0);
  zcore::BufferedReader reader(sourceReader, allocator, 0U);

  std::array<zcore::Byte, 4U> destination{};
  auto readResult = reader.Read(zcore::ByteSpanMut(destination));
  ASSERT_TRUE(readResult.HasValue());
  EXPECT_EQ(readResult.Value(), 4U);
  EXPECT_EQ(sourceReader.ReadCalls, 1);
  EXPECT_EQ(ByteValue(destination[0]), 0xA0U);
  EXPECT_EQ(ByteValue(destination[3]), 0xA3U);
}

TEST(BufferedReaderTest, ReadPropagatesUpstreamErrors) {
  TrackingAllocator allocator;
  const std::array<zcore::Byte, 3U> source{
      static_cast<zcore::Byte>(0x01U),
      static_cast<zcore::Byte>(0x02U),
      static_cast<zcore::Byte>(0x03U),
  };
  ChunkedReader sourceReader(zcore::ByteSpan(source), 3U, true, 0);
  zcore::BufferedReader reader(sourceReader, allocator, 4U);

  std::array<zcore::Byte, 3U> destination{};
  auto readResult = reader.Read(zcore::ByteSpanMut(destination));
  ASSERT_TRUE(readResult.HasError());
  EXPECT_EQ(readResult.Error().code.domain.id, zcore::kIoErrorDomain.id);
  EXPECT_EQ(
      readResult.Error().code.value,
      static_cast<zcore::i32>(zcore::IoErrorCode::UNSUPPORTED_OPERATION));
}

}  // namespace
