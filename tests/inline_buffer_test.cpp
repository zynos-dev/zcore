/**************************************************************************/
/*  inline_buffer_test.cpp                                                */
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
 * @file tests/inline_buffer_test.cpp
 * @brief Unit tests for fixed-capacity `InlineBuffer` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/inline_buffer.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstddef>

namespace {

[[nodiscard]] unsigned int ByteValue(const zcore::Byte value) noexcept {
  return std::to_integer<unsigned int>(value);
}

TEST(InlineBufferTest, DefaultConstructedBufferIsEmptyWithFixedCapacity) {
  zcore::InlineBuffer<4> buffer;

  EXPECT_TRUE(buffer.Empty());
  EXPECT_EQ(buffer.Size(), 0U);
  EXPECT_EQ(buffer.Capacity(), 4U);
  EXPECT_EQ(buffer.RemainingCapacity(), 4U);
  EXPECT_TRUE(buffer.TryPushBack(static_cast<zcore::Byte>(0x10U)));
}

TEST(InlineBufferTest, AssignAppendResizeAndPopBehaviorsAreDeterministic) {
  zcore::InlineBuffer<6> buffer;
  const std::array<zcore::Byte, 3U> prefix{
      static_cast<zcore::Byte>(0x11U),
      static_cast<zcore::Byte>(0x22U),
      static_cast<zcore::Byte>(0x33U),
  };
  const std::array<zcore::Byte, 2U> suffix{
      static_cast<zcore::Byte>(0x44U),
      static_cast<zcore::Byte>(0x55U),
  };

  ASSERT_TRUE(buffer.TryAssign(zcore::ByteSpan(prefix)));
  ASSERT_TRUE(buffer.TryAppend(zcore::ByteSpan(suffix)));
  ASSERT_EQ(buffer.Size(), 5U);
  EXPECT_EQ(ByteValue(buffer.Front()), 0x11U);
  EXPECT_EQ(ByteValue(buffer.Back()), 0x55U);

  ASSERT_TRUE(buffer.TryResize(6U, static_cast<zcore::Byte>(0xA5U)));
  EXPECT_EQ(ByteValue(buffer[5]), 0xA5U);
  ASSERT_TRUE(buffer.TryResize(2U));
  EXPECT_EQ(buffer.Size(), 2U);
  EXPECT_EQ(ByteValue(buffer[0]), 0x11U);
  EXPECT_EQ(ByteValue(buffer[1]), 0x22U);

  auto last = buffer.TryPopBackValue();
  ASSERT_TRUE(last.HasValue());
  EXPECT_EQ(ByteValue(last.Value()), 0x22U);
  EXPECT_TRUE(buffer.TryPopBack());
  EXPECT_FALSE(buffer.TryPopBack());
}

TEST(InlineBufferTest, OverflowOperationsDoNotMutateExistingBytes) {
  zcore::InlineBuffer<3> buffer;
  const std::array<zcore::Byte, 2U> initial{
      static_cast<zcore::Byte>(0x01U),
      static_cast<zcore::Byte>(0x02U),
  };
  const std::array<zcore::Byte, 2U> overflow{
      static_cast<zcore::Byte>(0x03U),
      static_cast<zcore::Byte>(0x04U),
  };

  ASSERT_TRUE(buffer.TryAssign(zcore::ByteSpan(initial)));
  EXPECT_FALSE(buffer.TryAppend(zcore::ByteSpan(overflow)));
  ASSERT_EQ(buffer.Size(), 2U);
  EXPECT_EQ(ByteValue(buffer[0]), 0x01U);
  EXPECT_EQ(ByteValue(buffer[1]), 0x02U);
}

TEST(InlineBufferTest, AsBytesViewsExposeSameBackingStorage) {
  zcore::InlineBuffer<4> buffer;
  ASSERT_TRUE(buffer.TryPushBack(static_cast<zcore::Byte>(0x7AU)));
  ASSERT_TRUE(buffer.TryPushBack(static_cast<zcore::Byte>(0x7BU)));

  const zcore::ByteSpanMut mutableBytes = buffer.AsBytesMut();
  ASSERT_EQ(mutableBytes.Size(), 2U);
  mutableBytes[1] = static_cast<zcore::Byte>(0x70U);

  const zcore::ByteSpan immutableBytes = buffer.AsBytes();
  ASSERT_EQ(immutableBytes.Size(), 2U);
  EXPECT_EQ(ByteValue(immutableBytes[0]), 0x7AU);
  EXPECT_EQ(ByteValue(immutableBytes[1]), 0x70U);
}

TEST(InlineBufferTest, ZeroCapacityBufferAcceptsOnlyEmptyOperations) {
  zcore::InlineBuffer<0> buffer;

  EXPECT_TRUE(buffer.Empty());
  EXPECT_TRUE(buffer.Full());
  EXPECT_TRUE(buffer.TryAssign(zcore::ByteSpan::Empty()));
  EXPECT_TRUE(buffer.TryAppend(zcore::ByteSpan::Empty()));
  EXPECT_TRUE(buffer.TryResize(0U));
  EXPECT_FALSE(buffer.TryPushBack(static_cast<zcore::Byte>(0x99U)));
  EXPECT_FALSE(buffer.TryResize(1U));
}

#if GTEST_HAS_DEATH_TEST
TEST(InlineBufferContractTest, PushBackOnFullTerminates) {
  EXPECT_DEATH(([]() {
                 zcore::InlineBuffer<1> buffer;
                 buffer.PushBack(static_cast<zcore::Byte>(0x01U));
                 buffer.PushBack(static_cast<zcore::Byte>(0x02U));
               }()),
               "");
}

TEST(InlineBufferContractTest, IndexOutOfBoundsTerminates) {
  EXPECT_DEATH(([]() {
                 zcore::InlineBuffer<2> buffer;
                 buffer.PushBack(static_cast<zcore::Byte>(0x01U));
                 static_cast<void>(buffer[1]);
               }()),
               "");
}
#endif

}  // namespace
