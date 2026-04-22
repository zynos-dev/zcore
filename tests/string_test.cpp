/**************************************************************************/
/*  string_test.cpp                                                       */
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
 * @file tests/string_test.cpp
 * @brief Unit tests for allocator-backed UTF-8 string contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/string.hpp>

#include <gtest/gtest.h>

#include <new>
#include <string_view>
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
    zcore::Status requestStatus = zcore::ValidateAllocationRequest(request);
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

static_assert(!std::is_copy_constructible_v<zcore::String>);
static_assert(!std::is_copy_assignable_v<zcore::String>);
static_assert(std::is_move_constructible_v<zcore::String>);
static_assert(std::is_move_assignable_v<zcore::String>);

TEST(StringTest, DefaultConstructedIsEmptyAndUnbound) {
  zcore::String value;

  EXPECT_TRUE(value.Empty());
  EXPECT_EQ(value.Size(), 0U);
  EXPECT_EQ(value.Capacity(), 0U);
  EXPECT_EQ(value.Data(), nullptr);
  EXPECT_STREQ(value.CStr(), "");
  EXPECT_FALSE(value.HasAllocator());
  EXPECT_EQ(value.AllocatorRef(), nullptr);

  const zcore::Status reserveStatus = value.TryReserve(8U);
  ASSERT_TRUE(reserveStatus.HasError());
  EXPECT_EQ(
      reserveStatus.Error().code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::UNSUPPORTED_OPERATION));
}

TEST(StringTest, TryWithCapacityBindsAllocatorAndReservesSpace) {
  TrackingAllocator allocator;
  auto created = zcore::String::TryWithCapacity(allocator, 12U);
  ASSERT_TRUE(created.HasValue());

  const zcore::String value = std::move(created.Value());
  EXPECT_TRUE(value.HasAllocator());
  EXPECT_EQ(value.Size(), 0U);
  EXPECT_GE(value.Capacity(), 12U);
  EXPECT_STREQ(value.CStr(), "");
}

TEST(StringTest, TryFromCStringValidatesUtf8AndNullPointer) {
  TrackingAllocator allocator;

  auto ok = zcore::String::TryFromCString(allocator, "zcore");
  ASSERT_TRUE(ok.HasValue());
  EXPECT_EQ(std::string_view(ok.Value().CStr(), ok.Value().Size()), std::string_view("zcore"));

  const char invalidUtf8[] = {static_cast<char>(0xC3), static_cast<char>(0x28), '\0'};
  auto invalid = zcore::String::TryFromCString(allocator, invalidUtf8);
  ASSERT_TRUE(invalid.HasError());
  EXPECT_EQ(invalid.Error().code.domain.id, zcore::kStringErrorDomain.id);
  EXPECT_EQ(
      invalid.Error().code.value,
      static_cast<zcore::i32>(zcore::StringErrorCode::INVALID_UTF8));

  auto nullInput = zcore::String::TryFromCString(allocator, nullptr);
  ASSERT_TRUE(nullInput.HasError());
  EXPECT_EQ(
      nullInput.Error().code.value,
      static_cast<zcore::i32>(zcore::StringErrorCode::INVALID_ARGUMENT));
}

TEST(StringTest, TryAssignAppendAndCodePointOpsAreUtf8Aware) {
  TrackingAllocator allocator;
  zcore::String value(allocator);

  ASSERT_TRUE(value.TryAssignCString("a").HasValue());
  ASSERT_TRUE(value.TryAppend(zcore::StringView::FromUtf8CString("bc")).HasValue());
  ASSERT_TRUE(value.TryAppendCodePoint(0x1F642U).HasValue());

  EXPECT_TRUE(value.IsValidUtf8());
  EXPECT_EQ(value.CodePointCount(), 4U);
  EXPECT_EQ(std::string_view(value.CStr(), value.Size()), std::string_view("abc\xF0\x9F\x99\x82", 7U));

  EXPECT_TRUE(value.RemoveSuffix(1U));
  EXPECT_EQ(std::string_view(value.CStr(), value.Size()), std::string_view("abc"));
  EXPECT_TRUE(value.RemovePrefix(1U));
  EXPECT_EQ(std::string_view(value.CStr(), value.Size()), std::string_view("bc"));
  EXPECT_TRUE(value.TryPopBack());
  EXPECT_EQ(std::string_view(value.CStr(), value.Size()), std::string_view("b"));
  EXPECT_EQ(value.CStr()[value.Size()], '\0');
}

TEST(StringTest, ByteCompatibilityApisCanInvalidateUtf8) {
  TrackingAllocator allocator;
  zcore::String value(allocator);
  ASSERT_TRUE(value.TryAssign(zcore::StringView::FromUtf8CString("A\xF0\x9F\x99\x82")).HasValue());
  ASSERT_TRUE(value.IsValidUtf8());

  EXPECT_TRUE(value.RemoveSuffixBytes(1U));
  EXPECT_FALSE(value.IsValidUtf8());
  EXPECT_FALSE(value.TryCodePointCount().HasValue());
  EXPECT_FALSE(value.RemoveSuffix(1U));
}

TEST(StringTest, TryPushBackRejectsNonAsciiByte) {
  TrackingAllocator allocator;
  zcore::String value(allocator);

  ASSERT_TRUE(value.TryPushBack('x').HasValue());
  const zcore::Status invalidStatus = value.TryPushBack(static_cast<char>(0xC3));
  ASSERT_TRUE(invalidStatus.HasError());
  EXPECT_EQ(invalidStatus.Error().code.domain.id, zcore::kStringErrorDomain.id);
  EXPECT_EQ(
      invalidStatus.Error().code.value,
      static_cast<zcore::i32>(zcore::StringErrorCode::INVALID_UTF8));
  EXPECT_EQ(value.Size(), 1U);
  EXPECT_EQ(value[0], 'x');
}

TEST(StringTest, AssignmentAndAppendPropagateAllocatorFailure) {
  TrackingAllocator allocator;
  zcore::String value(allocator);
  allocator.FailAllocation = true;

  const zcore::Status assignStatus = value.TryAssign(zcore::StringView::FromUtf8CString("abc"));
  ASSERT_TRUE(assignStatus.HasError());
  EXPECT_EQ(assignStatus.Error().code.domain.id, zcore::kAllocatorErrorDomain.id);
  EXPECT_EQ(value.Size(), 0U);

  allocator.FailAllocation = false;
  ASSERT_TRUE(value.TryAssignCString("ok").HasValue());
  allocator.FailAllocation = true;
  const zcore::Status appendStatus = value.TryAppend(zcore::StringView::FromUtf8CString("!"));
  ASSERT_TRUE(appendStatus.HasError());
  EXPECT_EQ(appendStatus.Error().code.domain.id, zcore::kAllocatorErrorDomain.id);
  EXPECT_EQ(std::string_view(value.CStr(), value.Size()), std::string_view("ok"));
}

TEST(StringTest, MoveAndResetReleaseOwnedStorage) {
  TrackingAllocator allocator;
  zcore::String source(allocator);
  ASSERT_TRUE(source.TryAssignCString("hello").HasValue());
  ASSERT_TRUE(source.TryAppend(zcore::StringView::FromUtf8CString(" world")).HasValue());

  zcore::String target;
  target = std::move(source);
  EXPECT_EQ(std::string_view(target.CStr(), target.Size()), std::string_view("hello world"));

  const int deallocateBeforeReset = allocator.DeallocateCalls;
  target.Reset();
  EXPECT_EQ(target.Size(), 0U);
  EXPECT_STREQ(target.CStr(), "");
  EXPECT_EQ(allocator.DeallocateCalls, deallocateBeforeReset + 1);
}

#if GTEST_HAS_DEATH_TEST
TEST(StringContractTest, IndexOutOfBoundsTerminates) {
  EXPECT_DEATH(([]() {
                 TrackingAllocator allocator;
                 zcore::String value(allocator);
                 static_cast<void>(value.TryAssignCString("z"));
                 static_cast<void>(value[1]);
               }()),
               "");
}

TEST(StringContractTest, CodePointCountOnInvalidUtf8Terminates) {
  EXPECT_DEATH(([]() {
                 TrackingAllocator allocator;
                 zcore::String value(allocator);
                 static_cast<void>(value.TryAssign(zcore::StringView::FromUtf8CString("A\xF0\x9F\x99\x82")));
                 static_cast<void>(value.RemoveSuffixBytes(1U));
                 static_cast<void>(value.CodePointCount());
               }()),
               "");
}
#endif

}  // namespace
