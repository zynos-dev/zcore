/**************************************************************************/
/*  rw_lock_test.cpp                                                      */
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
 * @file tests/rw_lock_test.cpp
 * @brief Unit tests for `RwLock` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/rw_lock.hpp>

#include <gtest/gtest.h>

#include <mutex>
#include <shared_mutex>
#include <type_traits>

namespace {

static_assert(!std::is_copy_constructible_v<zcore::RwLock>);
static_assert(!std::is_copy_assignable_v<zcore::RwLock>);
static_assert(!std::is_move_constructible_v<zcore::RwLock>);
static_assert(!std::is_move_assignable_v<zcore::RwLock>);

TEST(RwLockTest, ExclusiveTryLockReflectsOwnership) {
  zcore::RwLock lock;

  EXPECT_TRUE(lock.TryLock());
  EXPECT_FALSE(lock.TryLock());
  EXPECT_FALSE(lock.TryLockShared());
  lock.Unlock();
  EXPECT_TRUE(lock.TryLock());
  lock.Unlock();
}

TEST(RwLockTest, SharedTryLockAllowsMultipleReaders) {
  zcore::RwLock lock;

  EXPECT_TRUE(lock.TryLockShared());
  EXPECT_TRUE(lock.TryLockShared());
  EXPECT_FALSE(lock.TryLock());
  lock.UnlockShared();
  lock.UnlockShared();
}

TEST(RwLockTest, ExclusiveLockBlocksSharedAcquisition) {
  zcore::RwLock lock;

  lock.Lock();
  EXPECT_FALSE(lock.TryLockShared());
  lock.Unlock();
}

TEST(RwLockTest, SharedAndExclusiveAdaptersMatchStdUtilities) {
  zcore::RwLock lock;
  int value = 0;

  {
    const std::unique_lock<zcore::RwLock> writer(lock);
    value = 5;
  }

  {
    const std::shared_lock<zcore::RwLock> reader(lock);
    EXPECT_EQ(value, 5);
  }
}

}  // namespace
