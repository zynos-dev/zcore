/**************************************************************************/
/*  btree_set_test.cpp                                                    */
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
 * @file tests/btree_set_test.cpp
 * @brief Unit tests for allocator-backed `BTreeSet` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/btree_set.hpp>

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

static_assert(!std::is_copy_constructible_v<zcore::BTreeSet<int>>);
static_assert(!std::is_copy_assignable_v<zcore::BTreeSet<int>>);
static_assert(std::is_move_constructible_v<zcore::BTreeSet<int>>);
static_assert(std::is_move_assignable_v<zcore::BTreeSet<int>>);

TEST(BTreeSetTest, DefaultConstructedIsEmptyAndUnbound) {
  zcore::BTreeSet<int> values;

  EXPECT_TRUE(values.Empty());
  EXPECT_EQ(values.Size(), 0U);
  EXPECT_EQ(values.Capacity(), 0U);
  EXPECT_FALSE(values.HasAllocator());
  EXPECT_EQ(values.AllocatorRef(), nullptr);

  auto insertResult = values.TryInsert(1);
  ASSERT_TRUE(insertResult.HasError());
  EXPECT_EQ(
      insertResult.Error().code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::UNSUPPORTED_OPERATION));
}

TEST(BTreeSetTest, TryWithCapacityBindsAllocatorAndReservesStorage) {
  TrackingAllocator allocator;
  auto created = zcore::BTreeSet<int>::TryWithCapacity(allocator, 12U);
  ASSERT_TRUE(created.HasValue());

  const zcore::BTreeSet<int> values = std::move(created.Value());
  EXPECT_TRUE(values.HasAllocator());
  EXPECT_EQ(values.Size(), 0U);
  EXPECT_GE(values.Capacity(), 12U);
  EXPECT_EQ(allocator.AllocateCalls, 1);
}

TEST(BTreeSetTest, InsertContainsAndDuplicateSemanticsAreDeterministic) {
  TrackingAllocator allocator;
  zcore::BTreeSet<int> values(allocator);

  auto first = values.TryInsert(3);
  auto second = values.TryInsert(7);
  auto duplicate = values.TryInsert(3);
  ASSERT_TRUE(first.HasValue());
  ASSERT_TRUE(second.HasValue());
  ASSERT_TRUE(duplicate.HasValue());
  EXPECT_TRUE(first.Value());
  EXPECT_TRUE(second.Value());
  EXPECT_FALSE(duplicate.Value());

  EXPECT_EQ(values.Size(), 2U);
  EXPECT_TRUE(values.Contains(3));
  EXPECT_TRUE(values.Contains(7));
  EXPECT_FALSE(values.Contains(9));
}

TEST(BTreeSetTest, RemoveAndGrowthBehaviorsAreDeterministic) {
  TrackingAllocator allocator;
  zcore::BTreeSet<int> values(allocator);

  for (int index = 63; index >= 0; --index) {
    auto inserted = values.TryInsert(index);
    ASSERT_TRUE(inserted.HasValue());
    ASSERT_TRUE(inserted.Value());
  }
  EXPECT_EQ(values.Size(), 64U);
  EXPECT_GE(values.Capacity(), 64U);

  for (int index = 0; index < 32; ++index) {
    EXPECT_TRUE(values.TryRemove(index));
  }
  EXPECT_EQ(values.Size(), 32U);
  for (int index = 0; index < 32; ++index) {
    EXPECT_FALSE(values.Contains(index));
  }
  for (int index = 32; index < 64; ++index) {
    EXPECT_TRUE(values.Contains(index));
  }
}

TEST(BTreeSetTest, ResetReleasesOwnedStorageThroughAllocator) {
  TrackingAllocator allocator;
  zcore::BTreeSet<int> values(allocator);
  ASSERT_TRUE(values.TryInsert(1).HasValue());
  ASSERT_TRUE(values.TryInsert(2).HasValue());

  const int deallocateBeforeReset = allocator.DeallocateCalls;
  values.Reset();

  EXPECT_TRUE(values.Empty());
  EXPECT_EQ(values.Size(), 0U);
  EXPECT_EQ(values.Capacity(), 0U);
  EXPECT_EQ(allocator.DeallocateCalls, deallocateBeforeReset + 1);
}

}  // namespace
