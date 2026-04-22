/**************************************************************************/
/*  vector_test.cpp                                                       */
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
 * @file tests/vector_test.cpp
 * @brief Unit tests for allocator-backed `Vector` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/vector.hpp>

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

static_assert(!std::is_copy_constructible_v<zcore::Vector<int>>);
static_assert(!std::is_copy_assignable_v<zcore::Vector<int>>);
static_assert(std::is_move_constructible_v<zcore::Vector<int>>);
static_assert(std::is_move_assignable_v<zcore::Vector<int>>);

TEST(VectorTest, DefaultConstructedIsEmptyAndUnbound) {
  zcore::Vector<int> values;

  EXPECT_TRUE(values.Empty());
  EXPECT_EQ(values.Size(), 0U);
  EXPECT_EQ(values.Capacity(), 0U);
  EXPECT_EQ(values.Data(), nullptr);
  EXPECT_FALSE(values.HasAllocator());
  EXPECT_EQ(values.AllocatorRef(), nullptr);

  const zcore::Status reserveStatus = values.TryReserve(4U);
  ASSERT_TRUE(reserveStatus.HasError());
  EXPECT_EQ(
      reserveStatus.Error().code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::UNSUPPORTED_OPERATION));
}

TEST(VectorTest, EmptyWithAllocatedStorageSupportsZeroDistanceArithmetic) {
  zcore::Vector<int> unboundValues;
  EXPECT_EQ(unboundValues.begin(), unboundValues.end());

  TrackingAllocator allocator;
  auto created = zcore::Vector<int>::TryWithCapacity(allocator, 8U);
  ASSERT_TRUE(created.HasValue());
  zcore::Vector<int> values = std::move(created.Value());

  EXPECT_EQ(values.begin(), values.end());
  EXPECT_EQ(values.end() - values.begin(), 0);

  ASSERT_TRUE(values.TryPushBack(3).HasValue());
  ASSERT_TRUE(values.TryPushBack(5).HasValue());
  EXPECT_EQ(values.end() - values.begin(), 2);

  values.Clear();
  EXPECT_EQ(values.begin(), values.end());
  EXPECT_EQ(values.end() - values.begin(), 0);
}

TEST(VectorTest, TryWithCapacityBindsAllocatorAndReservesStorage) {
  TrackingAllocator allocator;
  auto created = zcore::Vector<int>::TryWithCapacity(allocator, 4U);
  ASSERT_TRUE(created.HasValue());

  const zcore::Vector<int> values = std::move(created.Value());
  EXPECT_TRUE(values.HasAllocator());
  EXPECT_EQ(values.Size(), 0U);
  EXPECT_GE(values.Capacity(), 4U);
  EXPECT_EQ(allocator.AllocateCalls, 1);
  EXPECT_EQ(allocator.DeallocateCalls, 0);
}

TEST(VectorTest, PushBackGrowsAndPreservesOrder) {
  TrackingAllocator allocator;
  zcore::Vector<int> values(allocator);

  EXPECT_TRUE(values.TryPushBack(10).HasValue());
  EXPECT_TRUE(values.TryPushBack(20).HasValue());
  EXPECT_TRUE(values.TryPushBack(30).HasValue());

  ASSERT_EQ(values.Size(), 3U);
  EXPECT_EQ(values.Front(), 10);
  EXPECT_EQ(values.Back(), 30);
  EXPECT_EQ(values[1], 20);
  EXPECT_NE(values.TryAt(2U), nullptr);
  EXPECT_EQ(values.TryAt(3U), nullptr);
}

TEST(VectorTest, ReservePreservesExistingValues) {
  TrackingAllocator allocator;
  zcore::Vector<int> values(allocator);
  ASSERT_TRUE(values.TryPushBack(3).HasValue());
  ASSERT_TRUE(values.TryPushBack(5).HasValue());

  const zcore::usize priorCapacity = values.Capacity();
  const zcore::Status reserveStatus = values.TryReserve(priorCapacity + 8U);
  ASSERT_TRUE(reserveStatus.HasValue());
  EXPECT_GE(values.Capacity(), priorCapacity + 8U);
  EXPECT_EQ(values.Size(), 2U);
  EXPECT_EQ(values[0], 3);
  EXPECT_EQ(values[1], 5);
}

TEST(VectorTest, TryPushBackPropagatesAllocatorFailureWithoutMutation) {
  TrackingAllocator allocator;
  allocator.FailAllocation = true;
  zcore::Vector<int> values(allocator);

  const zcore::Status pushStatus = values.TryPushBack(7);
  ASSERT_TRUE(pushStatus.HasError());
  EXPECT_EQ(pushStatus.Error().code.domain.id, zcore::kAllocatorErrorDomain.id);
  EXPECT_EQ(
      pushStatus.Error().code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::OUT_OF_MEMORY));
  EXPECT_EQ(values.Size(), 0U);
  EXPECT_EQ(values.Capacity(), 0U);
}

TEST(VectorTest, PopAndSliceApisAreDeterministic) {
  TrackingAllocator allocator;
  zcore::Vector<int> values(allocator);
  ASSERT_TRUE(values.TryPushBack(1).HasValue());
  ASSERT_TRUE(values.TryPushBack(2).HasValue());
  ASSERT_TRUE(values.TryPushBack(3).HasValue());

  auto popped = values.TryPopBackValue();
  ASSERT_TRUE(popped.HasValue());
  EXPECT_EQ(popped.Value(), 3);
  EXPECT_EQ(values.Size(), 2U);

  EXPECT_TRUE(values.TryPopBack());
  EXPECT_TRUE(values.TryPopBack());
  EXPECT_FALSE(values.TryPopBack());

  ASSERT_TRUE(values.TryPushBack(11).HasValue());
  ASSERT_TRUE(values.TryPushBack(13).HasValue());
  const zcore::SliceMut<int> mutView = values.AsSliceMut();
  mutView[1] = 17;
  const zcore::Slice<const int> constView = values.AsSlice();
  ASSERT_EQ(constView.Size(), 2U);
  EXPECT_EQ(constView[0], 11);
  EXPECT_EQ(constView[1], 17);
}

TEST(VectorTest, MoveTransfersAllocationWithoutDoubleDeallocate) {
  TrackingAllocator allocator;
  zcore::Vector<int> source(allocator);
  ASSERT_TRUE(source.TryPushBack(9).HasValue());
  ASSERT_TRUE(source.TryPushBack(12).HasValue());

  zcore::Vector<int> target;
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
TEST(VectorContractTest, FrontOnEmptyTerminates) {
  EXPECT_DEATH(([]() {
                 TrackingAllocator allocator;
                 zcore::Vector<int> values(allocator);
                 static_cast<void>(values.Front());
               }()),
               "");
}

TEST(VectorContractTest, IndexOutOfBoundsTerminates) {
  EXPECT_DEATH(([]() {
                 TrackingAllocator allocator;
                 zcore::Vector<int> values(allocator);
                 static_cast<void>(values.TryPushBack(1));
                 static_cast<void>(values[1]);
               }()),
               "");
}
#endif

}  // namespace
