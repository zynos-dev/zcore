/**************************************************************************/
/*  defer_test.cpp                                                        */
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
 * @file tests/defer_test.cpp
 * @brief Unit tests for `Defer` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure -R "Defer"
 * @endcode
 */

#include <zcore/defer.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>
#include <utility>

namespace {

struct NoThrowCleanup final {
  void operator()() noexcept {}
};

static_assert(std::is_same_v<zcore::Defer<NoThrowCleanup>, zcore::ScopeGuard<NoThrowCleanup>>);
static_assert(!std::is_copy_constructible_v<zcore::Defer<NoThrowCleanup>>);
static_assert(std::is_move_constructible_v<zcore::Defer<NoThrowCleanup>>);

TEST(DeferTest, ExecutesCleanupOnScopeExitWhenActive) {
  int calls = 0;
  {
    auto defer = zcore::MakeDefer([&calls]() noexcept {
      ++calls;
    });
    EXPECT_TRUE(defer.Active());
  }
  EXPECT_EQ(calls, 1);
}

TEST(DeferTest, DismissAndArmControlFinalExecution) {
  int calls = 0;
  {
    auto defer = zcore::MakeDefer([&calls]() noexcept {
      ++calls;
    });
    defer.Dismiss();
    EXPECT_FALSE(defer.Active());
    defer.Arm();
    EXPECT_TRUE(defer.Active());
  }
  EXPECT_EQ(calls, 1);
}

TEST(DeferTest, ExecuteNowRunsCleanupOnce) {
  int calls = 0;
  {
    auto defer = zcore::MakeDefer([&calls]() noexcept {
      ++calls;
    });
    defer.ExecuteNow();
    EXPECT_FALSE(defer.Active());
  }
  EXPECT_EQ(calls, 1);
}

TEST(DeferTest, SupportsMoveOnlyCleanupCallable) {
  int value = 0;
  auto payload = std::make_unique<int>(9);
  {
    auto defer = zcore::MakeDefer([owned = std::move(payload), &value]() noexcept {
      value = *owned;
    });
    EXPECT_TRUE(defer.Active());
  }
  EXPECT_EQ(value, 9);
  EXPECT_EQ(payload, nullptr);
}

}  // namespace
