/**************************************************************************/
/*  byte_buffer_test.cpp                                                  */
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
 * @file tests/byte_buffer_test.cpp
 * @brief Unit tests for allocator-backed `ByteBuffer` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/byte_buffer.hpp>

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

[[nodiscard]] unsigned int ByteValue(zcore::Byte value) {
  return std::to_integer<unsigned int>(value);
}

TEST(ByteBufferTest, DefaultConstructedBufferIsEmptyAndUnbound) {
  zcore::ByteBuffer buffer;

  EXPECT_TRUE(buffer.Empty());
  EXPECT_EQ(buffer.Size(), 0U);
  EXPECT_EQ(buffer.Capacity(), 0U);
  EXPECT_FALSE(buffer.HasAllocator());
  EXPECT_EQ(buffer.AllocatorRef(), nullptr);

  const zcore::Status pushStatus = buffer.TryPushBack(static_cast<zcore::Byte>(0x7FU));
  ASSERT_TRUE(pushStatus.HasError());
  EXPECT_EQ(
      pushStatus.Error().code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::UNSUPPORTED_OPERATION));
}

TEST(ByteBufferTest, UnboundAssignRejectsNonEmptyPayload) {
  zcore::ByteBuffer buffer;
  const std::array<zcore::Byte, 2U> bytes{
      static_cast<zcore::Byte>(0x10U),
      static_cast<zcore::Byte>(0x20U),
  };

  const zcore::Status assignStatus = buffer.TryAssign(zcore::ByteSpan(bytes));
  ASSERT_TRUE(assignStatus.HasError());
  EXPECT_EQ(
      assignStatus.Error().code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::UNSUPPORTED_OPERATION));
}

TEST(ByteBufferTest, TryWithCapacityBindsAllocatorAndReservesStorage) {
  TrackingAllocator allocator;
  auto created = zcore::ByteBuffer::TryWithCapacity(allocator, 8U);
  ASSERT_TRUE(created.HasValue());

  const zcore::ByteBuffer buffer = std::move(created.Value());
  EXPECT_TRUE(buffer.HasAllocator());
  EXPECT_EQ(buffer.Size(), 0U);
  EXPECT_GE(buffer.Capacity(), 8U);
  EXPECT_EQ(allocator.AllocateCalls, 1);
}

TEST(ByteBufferTest, AppendResizeAndPopBehaviorsAreDeterministic) {
  TrackingAllocator allocator;
  zcore::ByteBuffer buffer(allocator);

  const std::array<zcore::Byte, 3U> prefix{
      static_cast<zcore::Byte>(0x11U),
      static_cast<zcore::Byte>(0x22U),
      static_cast<zcore::Byte>(0x33U),
  };
  ASSERT_TRUE(buffer.TryAppend(zcore::ByteSpan(prefix)).HasValue());
  ASSERT_EQ(buffer.Size(), 3U);
  EXPECT_EQ(ByteValue(buffer.Front()), 0x11U);
  EXPECT_EQ(ByteValue(buffer.Back()), 0x33U);

  ASSERT_TRUE(buffer.TryResize(5U, static_cast<zcore::Byte>(0xA5U)).HasValue());
  ASSERT_EQ(buffer.Size(), 5U);
  EXPECT_EQ(ByteValue(buffer[3]), 0xA5U);
  EXPECT_EQ(ByteValue(buffer[4]), 0xA5U);

  ASSERT_TRUE(buffer.TryResize(2U).HasValue());
  ASSERT_EQ(buffer.Size(), 2U);
  EXPECT_EQ(ByteValue(buffer[0]), 0x11U);
  EXPECT_EQ(ByteValue(buffer[1]), 0x22U);

  auto popped = buffer.TryPopBackValue();
  ASSERT_TRUE(popped.HasValue());
  EXPECT_EQ(ByteValue(popped.Value()), 0x22U);
  EXPECT_TRUE(buffer.TryPopBack());
  EXPECT_FALSE(buffer.TryPopBack());
}

TEST(ByteBufferTest, AsBytesViewsExposeSameBackingStorage) {
  TrackingAllocator allocator;
  zcore::ByteBuffer buffer(allocator);
  ASSERT_TRUE(buffer.TryPushBack(static_cast<zcore::Byte>(0x01U)).HasValue());
  ASSERT_TRUE(buffer.TryPushBack(static_cast<zcore::Byte>(0x02U)).HasValue());

  const zcore::ByteSpanMut mutableBytes = buffer.AsBytesMut();
  ASSERT_EQ(mutableBytes.Size(), 2U);
  mutableBytes[1] = static_cast<zcore::Byte>(0x7EU);

  const zcore::ByteSpan immutableBytes = buffer.AsBytes();
  ASSERT_EQ(immutableBytes.Size(), 2U);
  EXPECT_EQ(ByteValue(immutableBytes[0]), 0x01U);
  EXPECT_EQ(ByteValue(immutableBytes[1]), 0x7EU);
}

TEST(ByteBufferTest, ResetReleasesOwnedStorageThroughAllocator) {
  TrackingAllocator allocator;
  zcore::ByteBuffer buffer(allocator);
  ASSERT_TRUE(buffer.TryPushBack(static_cast<zcore::Byte>(0xAAU)).HasValue());

  const int deallocateBeforeReset = allocator.DeallocateCalls;
  buffer.Reset();

  EXPECT_EQ(buffer.Size(), 0U);
  EXPECT_EQ(buffer.Capacity(), 0U);
  EXPECT_EQ(allocator.DeallocateCalls, deallocateBeforeReset + 1);
}

}  // namespace
