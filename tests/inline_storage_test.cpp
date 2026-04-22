/**************************************************************************/
/*  inline_storage_test.cpp                                               */
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
 * @file tests/inline_storage_test.cpp
 * @brief Unit tests for inline-storage contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/inline_storage.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

namespace {

struct LifetimeProbe final {
  static inline int g_live_count = 0;
  int value;

  explicit LifetimeProbe(const int valueIn) noexcept : value(valueIn) {
    ++g_live_count;
  }

  LifetimeProbe(const LifetimeProbe& other) noexcept : value(other.value) {
    ++g_live_count;
  }

  LifetimeProbe(LifetimeProbe&& other) noexcept : value(other.value) {
    ++g_live_count;
    other.value = -1;
  }

  LifetimeProbe& operator=(const LifetimeProbe&) = delete;
  LifetimeProbe& operator=(LifetimeProbe&&) = delete;

  ~LifetimeProbe() {
    --g_live_count;
  }
};

static_assert(!std::is_copy_constructible_v<zcore::InlineStorage<int, 4>>);
static_assert(!std::is_copy_assignable_v<zcore::InlineStorage<int, 4>>);
static_assert(!std::is_move_constructible_v<zcore::InlineStorage<int, 4>>);
static_assert(!std::is_move_assignable_v<zcore::InlineStorage<int, 4>>);

TEST(InlineStorageTest, CapacityAndPointersAreDeterministic) {
  zcore::InlineStorage<int, 4> storage;
  EXPECT_EQ(storage.Capacity(), 4U);
  EXPECT_NE(storage.Data(), nullptr);
  EXPECT_NE(storage.TryPtrAt(0U), nullptr);
  EXPECT_NE(storage.TryPtrAt(3U), nullptr);
  EXPECT_EQ(storage.TryPtrAt(4U), nullptr);
}

TEST(InlineStorageTest, ZeroCapacityStorageHasNoAddressableSlots) {
  zcore::InlineStorage<int, 0> storage;
  EXPECT_EQ(storage.Capacity(), 0U);
  EXPECT_EQ(storage.Data(), nullptr);
  EXPECT_EQ(storage.TryPtrAt(0U), nullptr);
}

TEST(InlineStorageTest, ConstructRefAndDestroyManageLifetimeExplicitly) {
  LifetimeProbe::g_live_count = 0;
  zcore::InlineStorage<LifetimeProbe, 2> storage;

  const LifetimeProbe& first = storage.ConstructAt(0U, 11);
  const LifetimeProbe& second = storage.ConstructAt(1U, 22);
  EXPECT_EQ(LifetimeProbe::g_live_count, 2);
  EXPECT_EQ(first.value, 11);
  EXPECT_EQ(second.value, 22);
  EXPECT_EQ(storage.RefAt(0U).value, 11);
  EXPECT_EQ(storage.RefAt(1U).value, 22);

  storage.DestroyAt(1U);
  storage.DestroyAt(0U);
  EXPECT_EQ(LifetimeProbe::g_live_count, 0);
}

TEST(InlineStorageTest, SupportsMoveOnlyValues) {
  zcore::InlineStorage<std::unique_ptr<int>, 2> storage;
  storage.ConstructAt(0U, std::make_unique<int>(7));
  storage.ConstructAt(1U, std::make_unique<int>(9));

  ASSERT_NE(storage.RefAt(0U), nullptr);
  ASSERT_NE(storage.RefAt(1U), nullptr);
  EXPECT_EQ(*storage.RefAt(0U), 7);
  EXPECT_EQ(*storage.RefAt(1U), 9);

  storage.DestroyAt(1U);
  storage.DestroyAt(0U);
}

#if GTEST_HAS_DEATH_TEST
TEST(InlineStorageContractTest, PtrAtOutOfRangeTerminates) {
  zcore::InlineStorage<int, 2> storage;
  EXPECT_DEATH(
      {
        static_cast<void>(storage.PtrAt(2U));
      },
      "");
}
#endif

}  // namespace
