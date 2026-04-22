/**************************************************************************/
/*  thread_id_test.cpp                                                    */
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
 * @file tests/thread_id_test.cpp
 * @brief Unit tests for `ThreadId` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/thread_id.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <thread>
#include <type_traits>
#include <unordered_set>

namespace {

static_assert(sizeof(zcore::ThreadId) == sizeof(std::uint64_t));
static_assert(std::is_trivially_copyable_v<zcore::ThreadId>);

TEST(ThreadIdTest, DefaultConstructedIsInvalid) {
  const zcore::ThreadId id;
  EXPECT_FALSE(id.IsValid());
  EXPECT_TRUE(id.IsInvalid());
  EXPECT_EQ(id.Raw(), zcore::ThreadId::kInvalidValue);
}

TEST(ThreadIdTest, RawConstructionAndResetAreDeterministic) {
  zcore::ThreadId id = zcore::ThreadId::FromRawUnchecked(42ULL);
  EXPECT_TRUE(id.IsValid());
  EXPECT_EQ(id.Raw(), 42ULL);
  EXPECT_EQ(static_cast<zcore::u64>(id), 42ULL);

  id.Reset();
  EXPECT_TRUE(id.IsInvalid());
  EXPECT_EQ(id.Raw(), zcore::ThreadId::kInvalidValue);
}

TEST(ThreadIdTest, CurrentIsValidAndStableOnSameThread) {
  const zcore::ThreadId a = zcore::ThreadId::Current();
  const zcore::ThreadId b = zcore::ThreadId::Current();
  EXPECT_TRUE(a.IsValid());
  EXPECT_TRUE(b.IsValid());
  EXPECT_EQ(a, b);
}

TEST(ThreadIdTest, CurrentDiffersAcrossConcurrentThreads) {
  const zcore::ThreadId mainId = zcore::ThreadId::Current();
  zcore::ThreadId workerId;

  std::thread worker([&workerId]() {
    workerId = zcore::ThreadId::Current();
  });
  worker.join();

  EXPECT_TRUE(workerId.IsValid());
  EXPECT_NE(mainId, workerId);
}

TEST(ThreadIdTest, HashAdaptersWorkForUnorderedContainers) {
  std::unordered_set<zcore::ThreadId> ids;
  const zcore::ThreadId current = zcore::ThreadId::Current();

  ids.insert(current);
  ids.insert(current);
  ids.insert(zcore::ThreadId::FromRawUnchecked(7ULL));

  EXPECT_EQ(ids.count(current), 1U);
  EXPECT_EQ(ids.size(), 2U);
}

}  // namespace
