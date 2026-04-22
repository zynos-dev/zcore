/**************************************************************************/
/*  any_test.cpp                                                          */
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
 * @file tests/any_test.cpp
 * @brief Unit tests for allocator-aware type-erased value behavior.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure -R "Any"
 * @endcode
 */

#include <zcore/any.hpp>

#include <gtest/gtest.h>

#include <new>
#include <type_traits>
#include <utility>

namespace {

int g_any_destructor_calls = 0;
int g_large_any_destructor_calls = 0;

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

  explicit TrackedValue(int value) noexcept : Value(value) {}

  TrackedValue(const TrackedValue&) = default;
  TrackedValue& operator=(const TrackedValue&) = default;
  TrackedValue(TrackedValue&&) noexcept = default;
  TrackedValue& operator=(TrackedValue&&) noexcept = default;

  ~TrackedValue() {
    ++g_any_destructor_calls;
  }
};

struct alignas(64) LargeTrackedValue final {
  int Value = 0;
  char Padding[96]{};

  explicit LargeTrackedValue(int value) noexcept : Value(value) {}

  LargeTrackedValue(const LargeTrackedValue&) = default;
  LargeTrackedValue& operator=(const LargeTrackedValue&) = default;
  LargeTrackedValue(LargeTrackedValue&&) noexcept = default;
  LargeTrackedValue& operator=(LargeTrackedValue&&) noexcept = default;

  ~LargeTrackedValue() {
    ++g_large_any_destructor_calls;
  }
};

static_assert(!std::is_copy_constructible_v<zcore::Any>);
static_assert(!std::is_copy_assignable_v<zcore::Any>);
static_assert(std::is_move_constructible_v<zcore::Any>);
static_assert(std::is_move_assignable_v<zcore::Any>);

TEST(AnyTest, DefaultConstructedIsEmpty) {
  const zcore::Any value;
  EXPECT_TRUE(value.IsEmpty());
  EXPECT_FALSE(value.HasValue());
  EXPECT_TRUE(value.Type().IsInvalid());
  EXPECT_FALSE(static_cast<bool>(value));
}

TEST(AnyTest, InlineValueStoresWithoutAllocatorUse) {
  TrackingAllocator allocator;

  auto created = zcore::Any::TryMake<int>(allocator, 42);
  ASSERT_TRUE(created.HasValue());
  zcore::Any value = std::move(created.Value());

  EXPECT_EQ(allocator.AllocateCalls, 0);
  EXPECT_TRUE(value.Contains<int>());
  ASSERT_NE(value.TryGet<int>(), nullptr);
  EXPECT_EQ(*value.TryGet<int>(), 42);
  EXPECT_EQ(value.Get<int>(), 42);
  EXPECT_FALSE(value.Contains<float>());
  EXPECT_EQ(value.TryGet<float>(), nullptr);
}

TEST(AnyTest, EmplaceReplacesValueAndDestroysPrevious) {
  TrackingAllocator allocator;
  g_any_destructor_calls = 0;

  auto created = zcore::Any::TryMake<TrackedValue>(allocator, 7);
  ASSERT_TRUE(created.HasValue());
  zcore::Any value = std::move(created.Value());

  const int beforeReplace = g_any_destructor_calls;
  const zcore::Status emplaceStatus = value.TryEmplace<int>(allocator, 11);
  ASSERT_TRUE(emplaceStatus.HasValue());
  EXPECT_GE(g_any_destructor_calls, beforeReplace + 1);
  EXPECT_TRUE(value.Contains<int>());
  EXPECT_EQ(value.Get<int>(), 11);
}

TEST(AnyTest, LargeValueUsesAllocatorStorageAndReleasesOnReset) {
  TrackingAllocator allocator;
  g_large_any_destructor_calls = 0;

  auto created = zcore::Any::TryMake<LargeTrackedValue>(allocator, 9);
  ASSERT_TRUE(created.HasValue());
  zcore::Any value = std::move(created.Value());

  EXPECT_EQ(allocator.AllocateCalls, 1);
  EXPECT_EQ(allocator.DeallocateCalls, 0);
  EXPECT_TRUE(value.Contains<LargeTrackedValue>());
  EXPECT_EQ(value.Get<LargeTrackedValue>().Value, 9);

  value.Reset();
  EXPECT_TRUE(value.IsEmpty());
  EXPECT_EQ(g_large_any_destructor_calls, 1);
  EXPECT_EQ(allocator.DeallocateCalls, 1);
}

TEST(AnyTest, MoveTransfersStorageOwnershipWithoutDoubleRelease) {
  TrackingAllocator allocator;
  g_large_any_destructor_calls = 0;

  auto created = zcore::Any::TryMake<LargeTrackedValue>(allocator, 13);
  ASSERT_TRUE(created.HasValue());
  zcore::Any source = std::move(created.Value());
  zcore::Any target = std::move(source);

  EXPECT_TRUE(target.Contains<LargeTrackedValue>());
  EXPECT_EQ(target.Get<LargeTrackedValue>().Value, 13);

  target.Reset();
  EXPECT_EQ(g_large_any_destructor_calls, 1);
  EXPECT_EQ(allocator.DeallocateCalls, 1);
}

TEST(AnyTest, TryMakePropagatesAllocatorFailureForLargeValue) {
  TrackingAllocator allocator;
  allocator.FailAllocation = true;

  auto created = zcore::Any::TryMake<LargeTrackedValue>(allocator, 5);
  ASSERT_TRUE(created.HasError());
  EXPECT_EQ(created.Error().code.domain.id, zcore::kAllocatorErrorDomain.id);
  EXPECT_EQ(
      created.Error().code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::OUT_OF_MEMORY));
  EXPECT_EQ(allocator.AllocateCalls, 1);
}

#if GTEST_HAS_DEATH_TEST
TEST(AnyContractTest, GetOnWrongTypeTerminates) {
  TrackingAllocator allocator;
  auto created = zcore::Any::TryMake<int>(allocator, 99);
  ASSERT_TRUE(created.HasValue());
  zcore::Any value = std::move(created.Value());

  EXPECT_DEATH(
      {
        static_cast<void>(value.Get<float>());
      },
      "");
}
#endif

}  // namespace
