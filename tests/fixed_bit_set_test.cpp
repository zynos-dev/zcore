/**************************************************************************/
/*  fixed_bit_set_test.cpp                                                */
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
 * @file tests/fixed_bit_set_test.cpp
 * @brief Unit tests for deterministic `FixedBitSet` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/fixed_bit_set.hpp>

#include <gtest/gtest.h>

namespace {

TEST(FixedBitSetTest, DefaultConstructedIsEmptyWithKnownShape) {
  const zcore::FixedBitSet<130> bits;

  EXPECT_EQ(bits.BitCount(), 130U);
  EXPECT_EQ(bits.WordCount(), 3U);
  EXPECT_TRUE(bits.Empty());
  EXPECT_TRUE(bits.NoneSet());
  EXPECT_FALSE(bits.Any());
  EXPECT_FALSE(bits.All());
  EXPECT_EQ(bits.CountSet(), 0U);
}

TEST(FixedBitSetTest, SetClearToggleAssignAndTryApisAreDeterministic) {
  zcore::FixedBitSet<16> bits;

  EXPECT_TRUE(bits.TrySet(0U));
  EXPECT_TRUE(bits.TrySet(9U));
  EXPECT_TRUE(bits.Test(0U));
  EXPECT_TRUE(bits.Test(9U));

  auto value = bits.TryTest(9U);
  ASSERT_TRUE(value.HasValue());
  EXPECT_TRUE(value.Value());

  EXPECT_TRUE(bits.TryToggle(9U));
  EXPECT_FALSE(bits.Test(9U));
  EXPECT_TRUE(bits.TryAssign(5U, true));
  EXPECT_TRUE(bits.Test(5U));
  EXPECT_TRUE(bits.TryAssign(5U, false));
  EXPECT_FALSE(bits.Test(5U));

  EXPECT_TRUE(bits.TryClear(0U));
  EXPECT_FALSE(bits.Test(0U));
  EXPECT_FALSE(bits.TrySet(16U));
  EXPECT_FALSE(bits.TryClear(16U));
  EXPECT_FALSE(bits.TryToggle(16U));
  EXPECT_FALSE(bits.TryAssign(16U, true));
  EXPECT_FALSE(bits.TryTest(16U).HasValue());
}

TEST(FixedBitSetTest, SetAllFlipAllAndClearAllRespectTailMask) {
  zcore::FixedBitSet<70> bits;
  bits.SetAll();

  EXPECT_TRUE(bits.All());
  EXPECT_EQ(bits.CountSet(), 70U);
  EXPECT_EQ(bits.RawWord(1U), 0x3FULL);

  bits.FlipAll();
  EXPECT_TRUE(bits.Empty());
  EXPECT_EQ(bits.CountSet(), 0U);

  bits.Set(69U);
  EXPECT_TRUE(bits.Test(69U));
  EXPECT_FALSE(bits.Test(0U));
  bits.ClearAll();
  EXPECT_TRUE(bits.Empty());
}

TEST(FixedBitSetTest, BitwiseOperatorsComposeDeterministically) {
  zcore::FixedBitSet<80> lhs;
  zcore::FixedBitSet<80> rhs;
  lhs.Set(0U);
  lhs.Set(64U);
  rhs.Set(1U);
  rhs.Set(64U);

  const zcore::FixedBitSet<80> orBits = lhs | rhs;
  EXPECT_TRUE(orBits.Test(0U));
  EXPECT_TRUE(orBits.Test(1U));
  EXPECT_TRUE(orBits.Test(64U));

  const zcore::FixedBitSet<80> andBits = lhs & rhs;
  EXPECT_FALSE(andBits.Test(0U));
  EXPECT_FALSE(andBits.Test(1U));
  EXPECT_TRUE(andBits.Test(64U));

  const zcore::FixedBitSet<80> xorBits = lhs ^ rhs;
  EXPECT_TRUE(xorBits.Test(0U));
  EXPECT_TRUE(xorBits.Test(1U));
  EXPECT_FALSE(xorBits.Test(64U));

  const zcore::FixedBitSet<80> notBits = ~lhs;
  EXPECT_FALSE(notBits.Test(0U));
  EXPECT_TRUE(notBits.Test(1U));
  EXPECT_FALSE(notBits.Test(64U));
  EXPECT_EQ(notBits.CountSet(), 78U);
}

TEST(FixedBitSetTest, ZeroBitCountBehaviorIsDeterministic) {
  zcore::FixedBitSet<0> bits;

  EXPECT_EQ(bits.BitCount(), 0U);
  EXPECT_EQ(bits.WordCount(), 0U);
  EXPECT_TRUE(bits.Empty());
  EXPECT_TRUE(bits.All());
  EXPECT_FALSE(bits.Any());
  EXPECT_FALSE(bits.TrySet(0U));
  EXPECT_FALSE(bits.TryTest(0U).HasValue());
}

#if GTEST_HAS_DEATH_TEST
TEST(FixedBitSetContractTest, TestOutOfBoundsTerminates) {
  EXPECT_DEATH(([]() {
                 const zcore::FixedBitSet<8> bits;
                 static_cast<void>(bits.Test(8U));
               }()),
               "");
}

TEST(FixedBitSetContractTest, SetOutOfBoundsTerminates) {
  EXPECT_DEATH(([]() {
                 zcore::FixedBitSet<8> bits;
                 bits.Set(9U);
               }()),
               "");
}
#endif

}  // namespace
