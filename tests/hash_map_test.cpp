/**************************************************************************/
/*  hash_map_test.cpp                                                     */
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
 * @file tests/hash_map_test.cpp
 * @brief Unit tests for allocator-backed `HashMap` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/hash_map.hpp>

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

static_assert(!std::is_copy_constructible_v<zcore::HashMap<int, int>>);
static_assert(!std::is_copy_assignable_v<zcore::HashMap<int, int>>);
static_assert(std::is_move_constructible_v<zcore::HashMap<int, int>>);
static_assert(std::is_move_assignable_v<zcore::HashMap<int, int>>);

TEST(HashMapTest, DefaultConstructedIsEmptyAndUnbound) {
  zcore::HashMap<int, int> values;

  EXPECT_TRUE(values.Empty());
  EXPECT_EQ(values.Size(), 0U);
  EXPECT_EQ(values.Capacity(), 0U);
  EXPECT_FALSE(values.HasAllocator());
  EXPECT_EQ(values.AllocatorRef(), nullptr);

  auto insertResult = values.TryInsert(1, 7);
  ASSERT_TRUE(insertResult.HasError());
  EXPECT_EQ(
      insertResult.Error().code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::UNSUPPORTED_OPERATION));
}

TEST(HashMapTest, TryWithCapacityBindsAllocatorAndReservesStorage) {
  TrackingAllocator allocator;
  auto created = zcore::HashMap<int, int>::TryWithCapacity(allocator, 8U);
  ASSERT_TRUE(created.HasValue());

  const zcore::HashMap<int, int> values = std::move(created.Value());
  EXPECT_TRUE(values.HasAllocator());
  EXPECT_EQ(values.Size(), 0U);
  EXPECT_GE(values.Capacity(), 8U);
  EXPECT_EQ(allocator.AllocateCalls, 2);
}

TEST(HashMapTest, InsertLookupAndDuplicateSemanticsAreDeterministic) {
  TrackingAllocator allocator;
  zcore::HashMap<int, int> values(allocator);

  auto first = values.TryInsert(7, 10);
  auto second = values.TryInsert(9, 20);
  auto duplicate = values.TryInsert(7, 99);

  ASSERT_TRUE(first.HasValue());
  ASSERT_TRUE(second.HasValue());
  ASSERT_TRUE(duplicate.HasValue());
  EXPECT_TRUE(first.Value());
  EXPECT_TRUE(second.Value());
  EXPECT_FALSE(duplicate.Value());
  EXPECT_EQ(values.Size(), 2U);
  EXPECT_TRUE(values.Contains(7));
  EXPECT_TRUE(values.Contains(9));
  EXPECT_FALSE(values.Contains(11));
  ASSERT_NE(values.TryGet(7), nullptr);
  ASSERT_NE(values.TryGet(9), nullptr);
  EXPECT_EQ(*values.TryGet(7), 10);
  EXPECT_EQ(*values.TryGet(9), 20);
}

TEST(HashMapTest, InsertOrAssignAndRemoveValueWork) {
  TrackingAllocator allocator;
  zcore::HashMap<int, int> values(allocator);

  ASSERT_TRUE(values.TryInsert(1, 11).HasValue());
  ASSERT_TRUE(values.TryInsert(2, 22).HasValue());
  ASSERT_TRUE(values.TryInsertOrAssign(2, 33).HasValue());
  ASSERT_NE(values.TryGet(2), nullptr);
  EXPECT_EQ(*values.TryGet(2), 33);

  auto removed = values.TryRemoveValue(2);
  ASSERT_TRUE(removed.HasValue());
  EXPECT_EQ(removed.Value(), 33);
  EXPECT_FALSE(values.Contains(2));
  EXPECT_FALSE(values.TryRemove(2));
  EXPECT_TRUE(values.TryRemove(1));
  EXPECT_TRUE(values.Empty());
}

TEST(HashMapTest, GrowthPreservesAllEntries) {
  TrackingAllocator allocator;
  zcore::HashMap<int, int> values(allocator);

  for (int index = 0; index < 64; ++index) {
    auto inserted = values.TryInsert(index, index * 3);
    ASSERT_TRUE(inserted.HasValue());
    ASSERT_TRUE(inserted.Value());
  }
  EXPECT_GE(values.Capacity(), 64U);
  EXPECT_EQ(values.Size(), 64U);

  for (int index = 0; index < 64; ++index) {
    ASSERT_TRUE(values.Contains(index));
    const int* const found = values.TryGet(index);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(*found, index * 3);
  }
}

TEST(HashMapTest, MoveTransfersAllocationWithoutDoubleDeallocate) {
  TrackingAllocator allocator;
  zcore::HashMap<int, int> source(allocator);
  ASSERT_TRUE(source.TryInsert(9, 12).HasValue());
  ASSERT_TRUE(source.TryInsert(7, 3).HasValue());

  zcore::HashMap<int, int> target;
  target = std::move(source);

  EXPECT_EQ(target.Size(), 2U);
  EXPECT_EQ(target.Get(9), 12);
  EXPECT_EQ(target.Get(7), 3);

  const int deallocateBeforeReset = allocator.DeallocateCalls;
  target.Reset();
  EXPECT_EQ(target.Size(), 0U);
  EXPECT_EQ(target.Capacity(), 0U);
  EXPECT_EQ(allocator.DeallocateCalls, deallocateBeforeReset + 2);
}

#if GTEST_HAS_DEATH_TEST
TEST(HashMapContractTest, GetMissingKeyTerminates) {
  EXPECT_DEATH(([]() {
                 TrackingAllocator allocator;
                 zcore::HashMap<int, int> values(allocator);
                 static_cast<void>(values.Get(99));
               }()),
               "");
}
#endif

}  // namespace

