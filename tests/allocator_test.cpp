/**************************************************************************/
/*  allocator_test.cpp                                                    */
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
 * @file tests/allocator_test.cpp
 * @brief Unit tests for allocator interface contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/allocator.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <new>

namespace {

class SystemAllocator final : public zcore::Allocator {
 public:
  [[nodiscard]] zcore::Result<zcore::Allocation, zcore::Error> Allocate(
      zcore::AllocationRequest request) noexcept override {
    const zcore::Status requestStatus = zcore::ValidateAllocationRequest(request);
    if (requestStatus.HasError()) {
      return zcore::Result<zcore::Allocation, zcore::Error>::Failure(requestStatus.Error());
    }

    if (request.size == 0U) {
      return zcore::Result<zcore::Allocation, zcore::Error>::Success(zcore::Allocation::Empty());
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

TEST(AllocatorTest, PowerOfTwoAndAlignmentHelpersAreDeterministic) {
  EXPECT_FALSE(zcore::IsPowerOfTwo(0U));
  EXPECT_TRUE(zcore::IsPowerOfTwo(1U));
  EXPECT_TRUE(zcore::IsPowerOfTwo(8U));
  EXPECT_FALSE(zcore::IsPowerOfTwo(12U));

  EXPECT_TRUE(zcore::IsValidAllocationAlignment(alignof(std::max_align_t)));
  EXPECT_FALSE(zcore::IsValidAllocationAlignment(3U));
}

TEST(AllocatorTest, AllocationRequestValidationRejectsInvalidAlignment) {
  const zcore::AllocationRequest validRequest{.size = 64U, .alignment = 16U};
  const zcore::AllocationRequest invalidRequest{.size = 64U, .alignment = 3U};

  EXPECT_TRUE(validRequest.IsValid());
  EXPECT_FALSE(invalidRequest.IsValid());
  EXPECT_TRUE(zcore::ValidateAllocationRequest(validRequest).HasValue());
  EXPECT_TRUE(zcore::ValidateAllocationRequest(invalidRequest).HasError());
}

TEST(AllocatorTest, AllocationValidationAcceptsCanonicalShapes) {
  const zcore::Allocation empty = zcore::Allocation::Empty();
  EXPECT_TRUE(empty.IsValid());
  EXPECT_TRUE(zcore::ValidateAllocation(empty).HasValue());

  std::array<zcore::Byte, 32U> storage{};
  const zcore::Allocation aligned{
      .data = storage.data(),
      .size = storage.size(),
      .alignment = 8U,
  };
  EXPECT_TRUE(aligned.IsValid());
  EXPECT_TRUE(zcore::ValidateAllocation(aligned).HasValue());
}

TEST(AllocatorTest, AllocationValidationRejectsMisalignedOrMalformedBlocks) {
  std::array<zcore::Byte, 32U> storage{};
  const zcore::Allocation misaligned{
      .data = storage.data() + 1U,
      .size = 8U,
      .alignment = 8U,
  };
  const zcore::Allocation nullNonZero{
      .data = nullptr,
      .size = 8U,
      .alignment = 8U,
  };
  const zcore::Allocation nonNullZero{
      .data = storage.data(),
      .size = 0U,
      .alignment = 8U,
  };

  EXPECT_FALSE(misaligned.IsValid());
  EXPECT_FALSE(nullNonZero.IsValid());
  EXPECT_FALSE(nonNullZero.IsValid());
  EXPECT_TRUE(zcore::ValidateAllocation(misaligned).HasError());
  EXPECT_TRUE(zcore::ValidateAllocation(nullNonZero).HasError());
  EXPECT_TRUE(zcore::ValidateAllocation(nonNullZero).HasError());
}

TEST(AllocatorTest, AllocateBytesAndDeallocateRoundTrip) {
  SystemAllocator allocator;
  auto allocationResult = allocator.AllocateBytes(128U, 16U);

  ASSERT_TRUE(allocationResult.HasValue());
  const zcore::Allocation allocation = allocationResult.Value();
  EXPECT_TRUE(allocation.IsValid());
  EXPECT_EQ(allocation.size, 128U);
  EXPECT_EQ(allocation.alignment, 16U);

  const zcore::ByteSpanMut bytes = allocation.AsBytes();
  ASSERT_EQ(bytes.Size(), 128U);
  bytes[0] = zcore::Byte{0x5AU};
  bytes[127] = zcore::Byte{0xA5U};
  EXPECT_EQ(std::to_integer<unsigned int>(bytes[0]), 0x5AU);
  EXPECT_EQ(std::to_integer<unsigned int>(bytes[127]), 0xA5U);

  const zcore::Status deallocateStatus = allocator.Deallocate(allocation);
  EXPECT_TRUE(deallocateStatus.HasValue());
}

TEST(AllocatorTest, ZeroSizeAllocationReturnsEmptyDescriptor) {
  SystemAllocator allocator;
  auto allocationResult = allocator.Allocate(zcore::AllocationRequest::WithDefaultAlignment(0U));

  ASSERT_TRUE(allocationResult.HasValue());
  const zcore::Allocation allocation = allocationResult.Value();
  EXPECT_TRUE(allocation.IsEmpty());
  EXPECT_TRUE(allocation.IsValid());
  EXPECT_EQ(allocation.AsBytes().Size(), 0U);
}

TEST(AllocatorTest, AllocateRejectsInvalidRequest) {
  SystemAllocator allocator;
  auto allocationResult = allocator.Allocate(zcore::AllocationRequest{
      .size = 32U,
      .alignment = 3U,
  });

  ASSERT_TRUE(allocationResult.HasError());
  EXPECT_EQ(allocationResult.Error().code.domain.id, zcore::kAllocatorErrorDomain.id);
  EXPECT_EQ(
      allocationResult.Error().code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::INVALID_REQUEST));
}

TEST(AllocatorTest, DeallocateRejectsMalformedDescriptor) {
  SystemAllocator allocator;
  std::array<zcore::Byte, 16U> storage{};

  const zcore::Allocation malformed{
      .data = storage.data(),
      .size = 0U,
      .alignment = 8U,
  };
  const zcore::Status deallocateStatus = allocator.Deallocate(malformed);

  ASSERT_TRUE(deallocateStatus.HasError());
  EXPECT_EQ(
      deallocateStatus.Error().code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::INVALID_ALLOCATION));
}

}  // namespace
