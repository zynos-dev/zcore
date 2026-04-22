/**************************************************************************/
/*  function_test.cpp                                                     */
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
 * @file tests/function_test.cpp
 * @brief Unit tests for allocator-aware callable wrapper behavior.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure -R "Function"
 * @endcode
 */

#include <zcore/function.hpp>

#include <gtest/gtest.h>

#include <new>
#include <type_traits>
#include <utility>

namespace {

int g_function_destructor_calls = 0;
int g_large_function_destructor_calls = 0;

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

struct AddBiasCallable final {
  int Bias = 0;

  AddBiasCallable() = default;
  explicit AddBiasCallable(int bias) noexcept : Bias(bias) {}
  AddBiasCallable(const AddBiasCallable&) = default;
  AddBiasCallable& operator=(const AddBiasCallable&) = default;
  AddBiasCallable(AddBiasCallable&&) noexcept = default;
  AddBiasCallable& operator=(AddBiasCallable&&) noexcept = default;

  [[nodiscard]] int operator()(int value) noexcept {
    return value + Bias;
  }

  ~AddBiasCallable() {
    ++g_function_destructor_calls;
  }
};

struct alignas(64) LargeCallable final {
  int Bias = 0;
  char Padding[112]{};

  LargeCallable() = default;
  explicit LargeCallable(int bias) noexcept : Bias(bias) {}
  LargeCallable(const LargeCallable&) = default;
  LargeCallable& operator=(const LargeCallable&) = default;
  LargeCallable(LargeCallable&&) noexcept = default;
  LargeCallable& operator=(LargeCallable&&) noexcept = default;

  [[nodiscard]] int operator()(int value) noexcept {
    return value + Bias;
  }

  ~LargeCallable() {
    ++g_large_function_destructor_calls;
  }
};

struct MoveOnlyCallable final {
  int Value = 0;

  explicit MoveOnlyCallable(int value) noexcept : Value(value) {}

  MoveOnlyCallable(const MoveOnlyCallable&) = delete;
  MoveOnlyCallable& operator=(const MoveOnlyCallable&) = delete;
  MoveOnlyCallable(MoveOnlyCallable&&) noexcept = default;
  MoveOnlyCallable& operator=(MoveOnlyCallable&&) noexcept = default;
  ~MoveOnlyCallable() = default;

  [[nodiscard]] int operator()() noexcept {
    return Value;
  }
};

static_assert(!std::is_copy_constructible_v<zcore::Function<int(int)>>);
static_assert(!std::is_copy_assignable_v<zcore::Function<int(int)>>);
static_assert(std::is_move_constructible_v<zcore::Function<int(int)>>);
static_assert(std::is_move_assignable_v<zcore::Function<int(int)>>);

TEST(FunctionTest, DefaultConstructedIsEmpty) {
  const zcore::Function<int(int)> function;
  EXPECT_TRUE(function.IsEmpty());
  EXPECT_FALSE(function.HasValue());
  EXPECT_TRUE(function.TargetType().IsInvalid());
  EXPECT_FALSE(static_cast<bool>(function));
}

TEST(FunctionTest, InlineCallableInvokesWithoutAllocatorUse) {
  TrackingAllocator allocator;
  g_function_destructor_calls = 0;

  auto created = zcore::Function<int(int)>::TryMake(allocator, AddBiasCallable(3));
  ASSERT_TRUE(created.HasValue());
  zcore::Function<int(int)> function = std::move(created.Value());

  EXPECT_EQ(allocator.AllocateCalls, 0);
  EXPECT_TRUE(function.ContainsTarget<AddBiasCallable>());
  EXPECT_EQ(function(9), 12);
}

TEST(FunctionTest, TryBindReplacesTargetAndDestroysPrevious) {
  TrackingAllocator allocator;

  zcore::Function<int(int)> function;
  auto bindA = function.TryBind(allocator, AddBiasCallable(1));
  ASSERT_TRUE(bindA.HasValue());
  EXPECT_EQ(function(4), 5);

  auto bindB = function.TryBind(allocator, AddBiasCallable(5));
  ASSERT_TRUE(bindB.HasValue());
  EXPECT_EQ(function(4), 9);
}

TEST(FunctionTest, LargeCallableUsesAllocatorStorageAndReleasesOnReset) {
  TrackingAllocator allocator;
  g_large_function_destructor_calls = 0;

  auto created = zcore::Function<int(int)>::TryMake(allocator, LargeCallable(8));
  ASSERT_TRUE(created.HasValue());
  zcore::Function<int(int)> function = std::move(created.Value());

  EXPECT_EQ(allocator.AllocateCalls, 1);
  EXPECT_EQ(allocator.DeallocateCalls, 0);
  EXPECT_TRUE(function.ContainsTarget<LargeCallable>());
  EXPECT_EQ(function(10), 18);

  const int beforeReset = g_large_function_destructor_calls;
  function.Reset();
  EXPECT_TRUE(function.IsEmpty());
  EXPECT_GE(g_large_function_destructor_calls, beforeReset + 1);
  EXPECT_EQ(allocator.DeallocateCalls, 1);
}

TEST(FunctionTest, MoveTransfersTargetWithoutDoubleRelease) {
  TrackingAllocator allocator;
  g_large_function_destructor_calls = 0;

  auto created = zcore::Function<int(int)>::TryMake(allocator, LargeCallable(2));
  ASSERT_TRUE(created.HasValue());
  zcore::Function<int(int)> source = std::move(created.Value());
  zcore::Function<int(int)> target = std::move(source);

  EXPECT_TRUE(target.HasValue());
  EXPECT_EQ(target(5), 7);

  const int beforeReset = g_large_function_destructor_calls;
  target.Reset();
  EXPECT_GE(g_large_function_destructor_calls, beforeReset + 1);
  EXPECT_EQ(allocator.DeallocateCalls, 1);
}

TEST(FunctionTest, SupportsMoveOnlyCallableTargets) {
  TrackingAllocator allocator;

  auto created = zcore::Function<int()>::TryMake(allocator, MoveOnlyCallable(17));
  ASSERT_TRUE(created.HasValue());
  zcore::Function<int()> function = std::move(created.Value());

  EXPECT_EQ(function(), 17);
}

TEST(FunctionTest, TryMakePropagatesAllocatorFailureForLargeCallable) {
  TrackingAllocator allocator;
  allocator.FailAllocation = true;

  auto created = zcore::Function<int(int)>::TryMake(allocator, LargeCallable(4));
  ASSERT_TRUE(created.HasError());
  EXPECT_EQ(created.Error().code.domain.id, zcore::kAllocatorErrorDomain.id);
  EXPECT_EQ(
      created.Error().code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::OUT_OF_MEMORY));
  EXPECT_EQ(allocator.AllocateCalls, 1);
}

#if GTEST_HAS_DEATH_TEST
TEST(FunctionContractTest, InvokeOnEmptyTerminates) {
  EXPECT_DEATH(
      {
        zcore::Function<int(int)> function;
        static_cast<void>(function(1));
      },
      "");
}
#endif

}  // namespace
