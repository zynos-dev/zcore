/**************************************************************************/
/*  deque_test.cpp                                                        */
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
 * @file tests/deque_test.cpp
 * @brief Unit tests for allocator-backed `Deque` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/deque.hpp>

#include <gtest/gtest.h>

#include <new>
#include <type_traits>
#include <utility>

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

static_assert(!std::is_copy_constructible_v<zcore::Deque<int>>);
static_assert(!std::is_copy_assignable_v<zcore::Deque<int>>);
static_assert(std::is_move_constructible_v<zcore::Deque<int>>);
static_assert(std::is_move_assignable_v<zcore::Deque<int>>);

TEST(DequeTest, DefaultConstructedIsEmptyAndUnbound) {
  zcore::Deque<int> values;

  EXPECT_TRUE(values.Empty());
  EXPECT_EQ(values.Size(), 0U);
  EXPECT_EQ(values.Capacity(), 0U);
  EXPECT_FALSE(values.HasAllocator());
  EXPECT_EQ(values.AllocatorRef(), nullptr);

  const zcore::Status reserveStatus = values.TryReserve(4U);
  ASSERT_TRUE(reserveStatus.HasError());
  EXPECT_EQ(
      reserveStatus.Error().code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::UNSUPPORTED_OPERATION));
}

TEST(DequeTest, TryWithCapacityBindsAllocatorAndReservesStorage) {
  TrackingAllocator allocator;
  auto created = zcore::Deque<int>::TryWithCapacity(allocator, 4U);
  ASSERT_TRUE(created.HasValue());

  const zcore::Deque<int> values = std::move(created.Value());
  EXPECT_TRUE(values.HasAllocator());
  EXPECT_EQ(values.Size(), 0U);
  EXPECT_GE(values.Capacity(), 4U);
  EXPECT_EQ(allocator.AllocateCalls, 1);
}

TEST(DequeTest, PushBackAndPushFrontPreserveLogicalOrder) {
  TrackingAllocator allocator;
  zcore::Deque<int> values(allocator);

  ASSERT_TRUE(values.TryPushBack(20).HasValue());
  ASSERT_TRUE(values.TryPushFront(10).HasValue());
  ASSERT_TRUE(values.TryPushBack(30).HasValue());
  ASSERT_TRUE(values.TryPushFront(5).HasValue());

  ASSERT_EQ(values.Size(), 4U);
  EXPECT_EQ(values.Front(), 5);
  EXPECT_EQ(values.Back(), 30);
  EXPECT_EQ(values[0], 5);
  EXPECT_EQ(values[1], 10);
  EXPECT_EQ(values[2], 20);
  EXPECT_EQ(values[3], 30);
}

TEST(DequeTest, WrapAroundAndReservePreserveOrder) {
  TrackingAllocator allocator;
  zcore::Deque<int> values(allocator);
  ASSERT_TRUE(values.TryReserve(4U).HasValue());

  ASSERT_TRUE(values.TryPushBack(1).HasValue());
  ASSERT_TRUE(values.TryPushBack(2).HasValue());
  ASSERT_TRUE(values.TryPushBack(3).HasValue());
  ASSERT_TRUE(values.TryPushBack(4).HasValue());
  ASSERT_TRUE(values.TryPopFront());
  ASSERT_TRUE(values.TryPopFront());
  ASSERT_TRUE(values.TryPushBack(5).HasValue());
  ASSERT_TRUE(values.TryPushBack(6).HasValue());

  ASSERT_EQ(values.Size(), 4U);
  EXPECT_EQ(values[0], 3);
  EXPECT_EQ(values[1], 4);
  EXPECT_EQ(values[2], 5);
  EXPECT_EQ(values[3], 6);

  ASSERT_TRUE(values.TryReserve(12U).HasValue());
  EXPECT_GE(values.Capacity(), 12U);
  EXPECT_EQ(values[0], 3);
  EXPECT_EQ(values[1], 4);
  EXPECT_EQ(values[2], 5);
  EXPECT_EQ(values[3], 6);
}

TEST(DequeTest, PopFrontAndPopBackValueAreDeterministic) {
  TrackingAllocator allocator;
  zcore::Deque<int> values(allocator);
  ASSERT_TRUE(values.TryPushBack(7).HasValue());
  ASSERT_TRUE(values.TryPushBack(9).HasValue());
  ASSERT_TRUE(values.TryPushBack(11).HasValue());

  auto first = values.TryPopFrontValue();
  ASSERT_TRUE(first.HasValue());
  EXPECT_EQ(first.Value(), 7);

  auto last = values.TryPopBackValue();
  ASSERT_TRUE(last.HasValue());
  EXPECT_EQ(last.Value(), 11);

  ASSERT_EQ(values.Size(), 1U);
  EXPECT_EQ(values.Front(), 9);
  EXPECT_EQ(values.Back(), 9);
  EXPECT_TRUE(values.TryPopBack());
  EXPECT_FALSE(values.TryPopBack());
  EXPECT_FALSE(values.TryPopFront());
}

TEST(DequeTest, IteratorsTraverseLogicalOrder) {
  TrackingAllocator allocator;
  zcore::Deque<int> values(allocator);
  ASSERT_TRUE(values.TryPushBack(2).HasValue());
  ASSERT_TRUE(values.TryPushBack(4).HasValue());
  ASSERT_TRUE(values.TryPushFront(1).HasValue());

  int sum = 0;
  for (const int value : values) {
    sum += value;
  }
  EXPECT_EQ(sum, 7);
}

TEST(DequeTest, TryPushFrontPropagatesAllocatorFailureWithoutMutation) {
  TrackingAllocator allocator;
  allocator.FailAllocation = true;
  zcore::Deque<int> values(allocator);

  const zcore::Status pushStatus = values.TryPushFront(42);
  ASSERT_TRUE(pushStatus.HasError());
  EXPECT_EQ(pushStatus.Error().code.domain.id, zcore::kAllocatorErrorDomain.id);
  EXPECT_EQ(
      pushStatus.Error().code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::OUT_OF_MEMORY));
  EXPECT_EQ(values.Size(), 0U);
  EXPECT_EQ(values.Capacity(), 0U);
}

TEST(DequeTest, MoveTransfersAllocationWithoutDoubleDeallocate) {
  TrackingAllocator allocator;
  zcore::Deque<int> source(allocator);
  ASSERT_TRUE(source.TryPushBack(9).HasValue());
  ASSERT_TRUE(source.TryPushBack(12).HasValue());

  zcore::Deque<int> target;
  target = std::move(source);

  EXPECT_EQ(target.Size(), 2U);
  EXPECT_EQ(target[0], 9);
  EXPECT_EQ(target[1], 12);

  const int deallocateBeforeReset = allocator.DeallocateCalls;
  target.Reset();
  EXPECT_EQ(target.Size(), 0U);
  EXPECT_EQ(target.Capacity(), 0U);
  EXPECT_EQ(allocator.DeallocateCalls, deallocateBeforeReset + 1);
}

#if GTEST_HAS_DEATH_TEST
TEST(DequeContractTest, FrontOnEmptyTerminates) {
  EXPECT_DEATH(([]() {
                 TrackingAllocator allocator;
                 zcore::Deque<int> values(allocator);
                 static_cast<void>(values.Front());
               }()),
               "");
}

TEST(DequeContractTest, IndexOutOfBoundsTerminates) {
  EXPECT_DEATH(([]() {
                 TrackingAllocator allocator;
                 zcore::Deque<int> values(allocator);
                 static_cast<void>(values.TryPushBack(1));
                 static_cast<void>(values[1]);
               }()),
               "");
}
#endif

}  // namespace

