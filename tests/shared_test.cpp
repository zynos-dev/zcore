/**************************************************************************/
/*  shared_test.cpp                                                       */
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
 * @file tests/shared_test.cpp
 * @brief Unit tests for allocator-owned shared value wrapper behavior.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/shared.hpp>

#include <gtest/gtest.h>

#include <new>
#include <type_traits>
#include <utility>

namespace {

int g_shared_constructor_calls = 0;
int g_shared_destructor_calls = 0;

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

struct TrackedValue final {
  int Value;

  explicit TrackedValue(int value) noexcept : Value(value) {
    ++g_shared_constructor_calls;
  }

  TrackedValue(const TrackedValue&) = default;
  TrackedValue& operator=(const TrackedValue&) = default;
  TrackedValue(TrackedValue&&) noexcept = default;
  TrackedValue& operator=(TrackedValue&&) noexcept = default;

  ~TrackedValue() {
    ++g_shared_destructor_calls;
  }
};

struct NothrowDefaultValue final {
  int Value = 11;
};

static_assert(std::is_copy_constructible_v<zcore::Shared<int>>);
static_assert(std::is_copy_assignable_v<zcore::Shared<int>>);
static_assert(std::is_move_constructible_v<zcore::Shared<int>>);
static_assert(std::is_move_assignable_v<zcore::Shared<int>>);

TEST(SharedTest, DefaultConstructedIsEmpty) {
  const zcore::Shared<NothrowDefaultValue> shared;
  EXPECT_TRUE(shared.IsEmpty());
  EXPECT_FALSE(shared.HasValue());
  EXPECT_EQ(shared.Get(), nullptr);
  EXPECT_EQ(shared.AllocatorRef(), nullptr);
  EXPECT_EQ(shared.UseCount(), 0U);
  EXPECT_FALSE(shared.IsUnique());
  EXPECT_FALSE(static_cast<bool>(shared));
}

TEST(SharedTest, TryMakeConstructsValueWithUseCountOne) {
  TrackingAllocator allocator;
  g_shared_constructor_calls = 0;
  g_shared_destructor_calls = 0;

  auto created = zcore::Shared<TrackedValue>::TryMake(allocator, 42);
  ASSERT_TRUE(created.HasValue());
  zcore::Shared<TrackedValue> shared = std::move(created.Value());

  EXPECT_TRUE(shared.HasValue());
  EXPECT_EQ(shared->Value, 42);
  EXPECT_EQ(shared.UseCount(), 1U);
  EXPECT_TRUE(shared.IsUnique());
  EXPECT_EQ(g_shared_constructor_calls, 1);
  EXPECT_EQ(g_shared_destructor_calls, 0);
  EXPECT_EQ(allocator.AllocateCalls, 1);
  EXPECT_EQ(allocator.DeallocateCalls, 0);
}

TEST(SharedTest, CopySharesOwnershipAndReleasesOnLastOwner) {
  TrackingAllocator allocator;
  g_shared_constructor_calls = 0;
  g_shared_destructor_calls = 0;

  auto created = zcore::Shared<TrackedValue>::TryMake(allocator, 7);
  ASSERT_TRUE(created.HasValue());
  zcore::Shared<TrackedValue> first = std::move(created.Value());

  zcore::Shared<TrackedValue> second = first;
  EXPECT_EQ(first.UseCount(), 2U);
  EXPECT_EQ(second.UseCount(), 2U);
  EXPECT_FALSE(first.IsUnique());
  EXPECT_EQ(g_shared_destructor_calls, 0);

  second.Reset();
  EXPECT_TRUE(second.IsEmpty());

  EXPECT_EQ(first.UseCount(), 1U);
  EXPECT_TRUE(first.IsUnique());
  EXPECT_EQ(g_shared_destructor_calls, 0);

  first.Reset();
  EXPECT_TRUE(first.IsEmpty());
  EXPECT_EQ(g_shared_destructor_calls, 1);
  EXPECT_EQ(allocator.DeallocateCalls, 1);
}

TEST(SharedTest, MoveTransfersControlBlockWithoutIncrementingCount) {
  TrackingAllocator allocator;
  g_shared_constructor_calls = 0;
  g_shared_destructor_calls = 0;

  auto created = zcore::Shared<TrackedValue>::TryMake(allocator, 99);
  ASSERT_TRUE(created.HasValue());
  zcore::Shared<TrackedValue> source = std::move(created.Value());
  zcore::Shared<TrackedValue> target;
  target = std::move(source);

  EXPECT_TRUE(target.HasValue());
  EXPECT_EQ(target.UseCount(), 1U);
  EXPECT_EQ(target->Value, 99);

  target.Reset();
  EXPECT_EQ(g_shared_destructor_calls, 1);
  EXPECT_EQ(allocator.DeallocateCalls, 1);
}

TEST(SharedTest, TryMakePropagatesAllocatorFailure) {
  TrackingAllocator allocator;
  allocator.FailAllocation = true;

  auto created = zcore::Shared<NothrowDefaultValue>::TryMake(allocator);
  ASSERT_TRUE(created.HasError());
  EXPECT_EQ(created.Error().code.domain.id, zcore::kAllocatorErrorDomain.id);
  EXPECT_EQ(
      created.Error().code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::OUT_OF_MEMORY));
  EXPECT_EQ(allocator.AllocateCalls, 1);
  EXPECT_EQ(allocator.DeallocateCalls, 0);
}

#if GTEST_HAS_DEATH_TEST
TEST(SharedContractTest, ValueAccessOnEmptyTerminates) {
  EXPECT_DEATH(
      {
        zcore::Shared<NothrowDefaultValue> shared;
        static_cast<void>(shared.Value());
      },
      "");
}
#endif

}  // namespace
