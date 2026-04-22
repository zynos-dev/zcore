/**************************************************************************/
/*  scope_guard_test.cpp                                                  */
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
 * @file tests/scope_guard_test.cpp
 * @brief Unit tests for `ScopeGuard` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/scope_guard.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>
#include <utility>

namespace {

struct NoThrowCleanup final {
  void operator()() noexcept {}
};

static_assert(!std::is_copy_constructible_v<zcore::ScopeGuard<NoThrowCleanup>>);
static_assert(!std::is_copy_assignable_v<zcore::ScopeGuard<NoThrowCleanup>>);
static_assert(std::is_move_constructible_v<zcore::ScopeGuard<NoThrowCleanup>>);
static_assert(!std::is_move_assignable_v<zcore::ScopeGuard<NoThrowCleanup>>);

TEST(ScopeGuardTest, ExecutesCleanupOnScopeExitWhenActive) {
  int calls = 0;
  {
    auto guard = zcore::MakeScopeGuard([&calls]() noexcept {
      ++calls;
    });
    EXPECT_TRUE(guard.Active());
  }
  EXPECT_EQ(calls, 1);
}

TEST(ScopeGuardTest, DismissPreventsCleanupExecution) {
  int calls = 0;
  {
    auto guard = zcore::MakeScopeGuard([&calls]() noexcept {
      ++calls;
    });
    guard.Dismiss();
    EXPECT_FALSE(guard.Active());
  }
  EXPECT_EQ(calls, 0);
}

TEST(ScopeGuardTest, ArmReenablesCleanupAfterDismiss) {
  int calls = 0;
  {
    auto guard = zcore::MakeScopeGuard([&calls]() noexcept {
      ++calls;
    });
    guard.Dismiss();
    guard.Arm();
    EXPECT_TRUE(guard.Active());
  }
  EXPECT_EQ(calls, 1);
}

TEST(ScopeGuardTest, ExecuteNowRunsCleanupOnlyOnce) {
  int calls = 0;
  {
    auto guard = zcore::MakeScopeGuard([&calls]() noexcept {
      ++calls;
    });
    guard.ExecuteNow();
    EXPECT_FALSE(guard.Active());
  }
  EXPECT_EQ(calls, 1);
}

TEST(ScopeGuardTest, MoveTransfersExecutionResponsibility) {
  int calls = 0;
  {
    auto first = zcore::MakeScopeGuard([&calls]() noexcept {
      ++calls;
    });
    EXPECT_TRUE(first.Active());
    {
      auto second = std::move(first);
      EXPECT_TRUE(second.Active());
    }
    EXPECT_EQ(calls, 1);
  }
  EXPECT_EQ(calls, 1);
}

TEST(ScopeGuardTest, SupportsMoveOnlyCleanupCallable) {
  int value = 0;
  auto payload = std::make_unique<int>(7);
  {
    auto guard = zcore::MakeScopeGuard([owned = std::move(payload), &value]() noexcept {
      value = *owned;
    });
    EXPECT_TRUE(guard.Active());
  }
  EXPECT_EQ(value, 7);
  EXPECT_EQ(payload, nullptr);
}

}  // namespace
