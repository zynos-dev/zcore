/**************************************************************************/
/*  error_handler_test.cpp                                                */
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
 * @file tests/error_handler_test.cpp
 * @brief Unit tests for allocator/arena error propagation into handlers.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/arena.hpp>
#include <zcore/error_handler.hpp>

#include <gtest/gtest.h>

#include <array>

namespace {

struct CapturedError final {
  int calls;
  zcore::Error last;
};

void CaptureError(const zcore::Error& error, void* userData) noexcept {
  auto* const captured = static_cast<CapturedError*>(userData);
  if (captured == nullptr) {
    return;
  }
  captured->calls += 1;
  captured->last = error;
}

TEST(ErrorHandlerTest, ArenaInvalidRequestPropagatesOnceToHandler) {
  std::array<zcore::Byte, 64U> storage{};
  zcore::Arena arena(storage);
  CapturedError captured{
      .calls = 0,
      .last = zcore::Error::Ok(),
  };

  const auto result = arena.Allocate(zcore::AllocationRequest{
      .size = 8U,
      .alignment = 3U,
  });

  ASSERT_TRUE(result.HasError());
  const bool handled =
      zcore::HandleIfError(result, zcore::MakeErrorHandler(&CaptureError, &captured));

  EXPECT_TRUE(handled);
  EXPECT_EQ(captured.calls, 1);
  EXPECT_EQ(captured.last.code.domain.id, zcore::kAllocatorErrorDomain.id);
  EXPECT_EQ(
      captured.last.code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::INVALID_REQUEST));
  EXPECT_STREQ(captured.last.context.operation, "allocate");
}

TEST(ErrorHandlerTest, ArenaOutOfMemoryPropagatesOnceToHandler) {
  std::array<zcore::Byte, 8U> storage{};
  zcore::Arena arena(storage);
  CapturedError captured{
      .calls = 0,
      .last = zcore::Error::Ok(),
  };

  const auto result = arena.Allocate(zcore::AllocationRequest{
      .size = 16U,
      .alignment = 1U,
  });

  ASSERT_TRUE(result.HasError());
  const bool handled =
      zcore::HandleIfError(result, zcore::MakeErrorHandler(&CaptureError, &captured));

  EXPECT_TRUE(handled);
  EXPECT_EQ(captured.calls, 1);
  EXPECT_EQ(captured.last.code.domain.id, zcore::kAllocatorErrorDomain.id);
  EXPECT_EQ(
      captured.last.code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::OUT_OF_MEMORY));
  EXPECT_STREQ(captured.last.context.operation, "allocate");
}

TEST(ErrorHandlerTest, ArenaForeignDeallocatePropagatesOnceToHandler) {
  std::array<zcore::Byte, 64U> storage{};
  zcore::Arena arena(storage);
  alignas(8) std::array<zcore::Byte, 16U> foreignStorage{};
  CapturedError captured{
      .calls = 0,
      .last = zcore::Error::Ok(),
  };

  const zcore::Status status = arena.Deallocate(zcore::Allocation{
      .data = foreignStorage.data(),
      .size = 8U,
      .alignment = 8U,
  });

  ASSERT_TRUE(status.HasError());
  const bool handled =
      zcore::HandleIfError(status, zcore::MakeErrorHandler(&CaptureError, &captured));

  EXPECT_TRUE(handled);
  EXPECT_EQ(captured.calls, 1);
  EXPECT_EQ(captured.last.code.domain.id, zcore::kAllocatorErrorDomain.id);
  EXPECT_EQ(
      captured.last.code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::INVALID_ALLOCATION));
  EXPECT_STREQ(captured.last.context.operation, "deallocate");
}

}  // namespace
