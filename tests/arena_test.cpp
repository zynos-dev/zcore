/**************************************************************************/
/*  arena_test.cpp                                                        */
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
 * @file tests/arena_test.cpp
 * @brief Unit tests for arena allocator contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/arena.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace {

static_assert(!std::is_copy_constructible_v<zcore::Arena>);
static_assert(!std::is_copy_assignable_v<zcore::Arena>);
static_assert(std::is_move_constructible_v<zcore::Arena>);
static_assert(std::is_move_assignable_v<zcore::Arena>);

TEST(ArenaTest, DefaultConstructedHasZeroCapacity) {
  zcore::Arena arena;
  EXPECT_EQ(arena.Capacity(), 0U);
  EXPECT_EQ(arena.Used(), 0U);
  EXPECT_EQ(arena.Remaining(), 0U);
  EXPECT_TRUE(arena.IsEmpty());

  auto zeroAlloc = arena.Allocate(zcore::AllocationRequest::WithDefaultAlignment(0U));
  ASSERT_TRUE(zeroAlloc.HasValue());
  EXPECT_TRUE(zeroAlloc.Value().IsEmpty());

  auto nonZeroAlloc = arena.Allocate(zcore::AllocationRequest{
      .size = 1U,
      .alignment = 1U,
  });
  ASSERT_TRUE(nonZeroAlloc.HasError());
  EXPECT_EQ(
      nonZeroAlloc.Error().code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::OUT_OF_MEMORY));
}

TEST(ArenaTest, AllocateRespectsAlignmentAndUpdatesUsage) {
  std::array<zcore::Byte, 128U> storage{};
  zcore::Arena arena(storage);

  auto first = arena.Allocate(zcore::AllocationRequest{
      .size = 3U,
      .alignment = 1U,
  });
  ASSERT_TRUE(first.HasValue());
  EXPECT_EQ(first.Value().data, storage.data());
  EXPECT_EQ(arena.Used(), 3U);

  auto second = arena.Allocate(zcore::AllocationRequest{
      .size = 8U,
      .alignment = 16U,
  });
  ASSERT_TRUE(second.HasValue());
  EXPECT_TRUE(zcore::IsAddressAligned(second.Value().data, 16U));

  const std::uintptr_t baseAddress = reinterpret_cast<std::uintptr_t>(storage.data());
  const zcore::usize expectedPadding = static_cast<zcore::usize>((16U - ((baseAddress + 3U) % 16U)) % 16U);
  const zcore::usize expectedSecondOffset = 3U + expectedPadding;
  EXPECT_EQ(second.Value().data, storage.data() + expectedSecondOffset);
  EXPECT_EQ(arena.Used(), expectedSecondOffset + 8U);
  EXPECT_EQ(arena.Remaining(), arena.Capacity() - arena.Used());
}

TEST(ArenaTest, AllocateOutOfMemoryDoesNotMutateUsage) {
  std::array<zcore::Byte, 16U> storage{};
  zcore::Arena arena(storage);

  auto first = arena.Allocate(zcore::AllocationRequest{
      .size = 12U,
      .alignment = 1U,
  });
  ASSERT_TRUE(first.HasValue());
  const zcore::usize usedBeforeFailure = arena.Used();

  auto second = arena.Allocate(zcore::AllocationRequest{
      .size = 8U,
      .alignment = 1U,
  });
  ASSERT_TRUE(second.HasError());
  EXPECT_EQ(
      second.Error().code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::OUT_OF_MEMORY));
  EXPECT_EQ(arena.Used(), usedBeforeFailure);
}

TEST(ArenaTest, ResetRewindsArenaForReuse) {
  std::array<zcore::Byte, 64U> storage{};
  zcore::Arena arena(storage);

  auto first = arena.Allocate(zcore::AllocationRequest{
      .size = 16U,
      .alignment = 1U,
  });
  ASSERT_TRUE(first.HasValue());
  ASSERT_GT(arena.Used(), 0U);

  arena.Reset();
  EXPECT_EQ(arena.Used(), 0U);
  EXPECT_EQ(arena.Remaining(), arena.Capacity());

  auto second = arena.Allocate(zcore::AllocationRequest{
      .size = 16U,
      .alignment = 1U,
  });
  ASSERT_TRUE(second.HasValue());
  EXPECT_EQ(second.Value().data, storage.data());
}

TEST(ArenaTest, DeallocateAcceptsOwnedBlocksAsNoOp) {
  std::array<zcore::Byte, 64U> storage{};
  zcore::Arena arena(storage);

  auto allocated = arena.Allocate(zcore::AllocationRequest{
      .size = 8U,
      .alignment = 8U,
  });
  ASSERT_TRUE(allocated.HasValue());
  const zcore::usize usedBeforeDeallocate = arena.Used();

  const zcore::Status status = arena.Deallocate(allocated.Value());
  EXPECT_TRUE(status.HasValue());
  EXPECT_EQ(arena.Used(), usedBeforeDeallocate);
}

TEST(ArenaTest, DeallocateRejectsMalformedOrForeignAllocations) {
  std::array<zcore::Byte, 64U> storage{};
  zcore::Arena arena(storage);

  const zcore::Allocation malformed{
      .data = nullptr,
      .size = 4U,
      .alignment = 4U,
  };
  const zcore::Status malformedStatus = arena.Deallocate(malformed);
  ASSERT_TRUE(malformedStatus.HasError());
  EXPECT_EQ(
      malformedStatus.Error().code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::INVALID_ALLOCATION));

  alignas(16) std::array<zcore::Byte, 16U> foreignStorage{};
  const zcore::Allocation foreign{
      .data = foreignStorage.data(),
      .size = 8U,
      .alignment = 8U,
  };
  const zcore::Status foreignStatus = arena.Deallocate(foreign);
  ASSERT_TRUE(foreignStatus.HasError());
  EXPECT_EQ(
      foreignStatus.Error().code.value,
      static_cast<zcore::i32>(zcore::AllocatorErrorCode::INVALID_ALLOCATION));
}

TEST(ArenaTest, OwnsReflectsBackingStorageRange) {
  std::array<zcore::Byte, 32U> storage{};
  const zcore::Arena arena(storage);
  EXPECT_TRUE(arena.Owns(storage.data()));
  EXPECT_TRUE(arena.Owns(storage.data() + 31U));

  alignas(16) std::array<zcore::Byte, 32U> foreignStorage{};
  EXPECT_FALSE(arena.Owns(foreignStorage.data()));
  EXPECT_FALSE(arena.Owns(nullptr));
}

TEST(ArenaTest, MoveConstructionTransfersOwnershipAndInvalidatesSource) {
  std::array<zcore::Byte, 64U> storage{};
  zcore::Arena source(storage);

  auto first = source.Allocate(zcore::AllocationRequest{
      .size = 8U,
      .alignment = 8U,
  });
  ASSERT_TRUE(first.HasValue());

  zcore::Arena moved(std::move(source));

  auto second = moved.Allocate(zcore::AllocationRequest{
      .size = 8U,
      .alignment = 8U,
  });
  ASSERT_TRUE(second.HasValue());
  EXPECT_NE(second.Value().data, first.Value().data);
}

}  // namespace
