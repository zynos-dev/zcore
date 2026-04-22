/**************************************************************************/
/*  fixed_string_test.cpp                                                 */
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
 * @file tests/fixed_string_test.cpp
 * @brief Unit tests for deterministic `FixedString` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/fixed_string.hpp>

#include <gtest/gtest.h>

#include <random>
#include <type_traits>

namespace {

zcore::u32 RandomScalar(std::minstd_rand& rng) {
  const int bucket = static_cast<int>(rng() % 4U);
  if (bucket == 0) {
    return 0x21U + static_cast<zcore::u32>(rng() % 0x5EU);
  }
  if (bucket == 1) {
    return 0x80U + static_cast<zcore::u32>(rng() % (0x800U - 0x80U));
  }
  if (bucket == 2) {
    if ((rng() % 2U) == 0U) {
      return 0x800U + static_cast<zcore::u32>(rng() % (0xD800U - 0x800U));
    }
    return 0xE000U + static_cast<zcore::u32>(rng() % (0x10000U - 0xE000U));
  }
  return 0x10000U + static_cast<zcore::u32>(rng() % (0x110000U - 0x10000U));
}

static_assert(std::is_trivially_copyable_v<zcore::FixedString<8>>);

TEST(FixedStringTest, DefaultConstructedIsEmptyAndNullTerminated) {
  const zcore::FixedString<8> value;
  EXPECT_TRUE(value.Empty());
  EXPECT_EQ(value.Size(), 0U);
  EXPECT_EQ(value.CStr()[0], '\0');
  EXPECT_EQ(value.Capacity(), 8U);
}

TEST(FixedStringTest, FromCStringAndStringViewConstruction) {
  const auto value = zcore::FixedString<8>::FromCString("zcore");
  EXPECT_EQ(value.Size(), 5U);
  EXPECT_EQ(value.AsStdStringView(), "zcore");

  const auto fromView = zcore::FixedString<8>::FromStringView(zcore::StringView("core"));
  EXPECT_EQ(fromView.AsStdStringView(), "core");
}

TEST(FixedStringTest, TryFactoriesFailForNullOverflowOrInvalidUtf8) {
  const auto nullValue = zcore::FixedString<4>::TryFromCString(nullptr);
  EXPECT_FALSE(nullValue.HasValue());

  const auto overflow = zcore::FixedString<3>::TryFromCString("toolong");
  EXPECT_FALSE(overflow.HasValue());

  constexpr char kInvalidUtf8[] = {
      static_cast<char>(0xC0),
      static_cast<char>(0xAF),
      static_cast<char>(0x00),
  };
  EXPECT_FALSE(zcore::FixedString<8>::TryFromCString(kInvalidUtf8).HasValue());
}

TEST(FixedStringTest, AppendAndPushBackRespectCapacity) {
  zcore::FixedString<6> value = zcore::FixedString<6>::FromCString("ab");
  value.Append(zcore::StringView("cd"));
  value.PushBack('e');
  EXPECT_EQ(value.AsStdStringView(), "abcde");
  EXPECT_TRUE(value.TryPushBack('f'));
  EXPECT_FALSE(value.TryPushBack('g'));
  EXPECT_EQ(value.AsStdStringView(), "abcdef");
}

TEST(FixedStringTest, PushBackRejectsNonAsciiByte) {
  zcore::FixedString<8> value = zcore::FixedString<8>::FromCString("ab");
  EXPECT_FALSE(value.TryPushBack(static_cast<char>(0xC2)));
  EXPECT_EQ(value.AsStdStringView(), "ab");
}

TEST(FixedStringTest, AppendCodePointEncodesUtf8AndEnforcesCapacity) {
  zcore::FixedString<8> value;
  EXPECT_TRUE(value.TryAppendCodePoint(static_cast<zcore::u32>('A')));
  EXPECT_TRUE(value.TryAppendCodePoint(0x00A2U));  // ¢
  EXPECT_TRUE(value.TryAppendCodePoint(0x20ACU));  // €
  EXPECT_EQ(value.Size(), 6U);
  EXPECT_TRUE(value.IsValidUtf8());
  EXPECT_EQ(value.CodePointCount(), 3U);

  EXPECT_FALSE(value.TryAppendCodePoint(0x1F600U));  // needs 4 bytes
  EXPECT_FALSE(value.TryAppendCodePoint(0xD800U));   // surrogate, invalid scalar
}

TEST(FixedStringTest, TryAppendOverflowDoesNotMutateExistingState) {
  zcore::FixedString<5> value = zcore::FixedString<5>::FromCString("abc");
  EXPECT_FALSE(value.TryAppend(zcore::StringView("xyz")));
  EXPECT_EQ(value.AsStdStringView(), "abc");
}

TEST(FixedStringTest, RemovePrefixSuffixAndPopBackMutateDeterministically) {
  zcore::FixedString<8> value = zcore::FixedString<8>::FromCString("abcdef");
  EXPECT_TRUE(value.RemovePrefix(1U));
  EXPECT_TRUE(value.RemoveSuffix(2U));
  EXPECT_EQ(value.AsStdStringView(), "bcd");
  EXPECT_TRUE(value.TryPopBack());
  EXPECT_EQ(value.AsStdStringView(), "bc");
  EXPECT_FALSE(value.RemovePrefix(3U));
}

TEST(FixedStringTest, IndexTryAtAndEqualityWork) {
  zcore::FixedString<8> left = zcore::FixedString<8>::FromCString("same");
  const zcore::FixedString<8> right = zcore::FixedString<8>::FromCString("same");
  const zcore::FixedString<8> other = zcore::FixedString<8>::FromCString("diff");

  EXPECT_TRUE(left == right);
  EXPECT_FALSE(left == other);
  EXPECT_EQ(left[0], 's');

  auto* ptr = left.TryAt(2U);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 'm');
  EXPECT_EQ(left.TryAt(9U), nullptr);
}

TEST(FixedStringTest, CoreMutationApisRemainDeterministicForUtf8Bytes) {
  constexpr char kMixedUtf8[] = {
      'A',
      static_cast<char>(0xC2),
      static_cast<char>(0xA2),
      'B',
      static_cast<char>(0x00),
  };
  const zcore::StringView source = zcore::StringView::FromRawUnchecked(kMixedUtf8, 4U);

  zcore::FixedString<8> value;
  EXPECT_TRUE(value.TryAssign(source));
  EXPECT_EQ(value.Size(), 4U);
  EXPECT_EQ(value[0], 'A');
  EXPECT_EQ(static_cast<unsigned char>(value[1]), 0xC2U);
  EXPECT_EQ(static_cast<unsigned char>(value[2]), 0xA2U);
  EXPECT_EQ(value[3], 'B');

  EXPECT_TRUE(value.RemovePrefix(1U));
  EXPECT_TRUE(value.RemoveSuffix(1U));
  EXPECT_EQ(value.Size(), 2U);
  EXPECT_EQ(static_cast<unsigned char>(value[0]), 0xC2U);
  EXPECT_EQ(static_cast<unsigned char>(value[1]), 0xA2U);

  EXPECT_TRUE(value.TryAppend(zcore::StringView("X")));
  EXPECT_EQ(value.Size(), 3U);
  EXPECT_EQ(value[2], 'X');
  EXPECT_TRUE(value.TryPopBack());
  EXPECT_EQ(value.Size(), 2U);
  EXPECT_EQ(value.CStr()[2], '\0');
}

TEST(FixedStringTest, CodePointMutationsHandleMultiByteUtf8WithoutSplitting) {
  constexpr char kMultiUtf8[] = {
      'A',
      static_cast<char>(0xC2),  // ¢
      static_cast<char>(0xA2),
      static_cast<char>(0xE2),  // €
      static_cast<char>(0x82),
      static_cast<char>(0xAC),
      static_cast<char>(0xF0),  // 😀
      static_cast<char>(0x9F),
      static_cast<char>(0x98),
      static_cast<char>(0x80),
      'B',
      static_cast<char>(0x00),
  };

  zcore::FixedString<16> value;
  ASSERT_TRUE(value.TryAssign(zcore::StringView::FromCString(kMultiUtf8)));
  EXPECT_EQ(value.Size(), 11U);
  EXPECT_TRUE(value.IsValidUtf8());
  EXPECT_EQ(value.CodePointCount(), 5U);

  EXPECT_TRUE(value.RemovePrefix(2U));
  EXPECT_TRUE(value.IsValidUtf8());
  EXPECT_EQ(value.CodePointCount(), 3U);
  EXPECT_EQ(value.Size(), 8U);

  EXPECT_TRUE(value.RemoveSuffix(1U));
  EXPECT_TRUE(value.IsValidUtf8());
  EXPECT_EQ(value.CodePointCount(), 2U);
  EXPECT_EQ(value.Size(), 7U);

  EXPECT_TRUE(value.TryPopBack());
  EXPECT_TRUE(value.IsValidUtf8());
  EXPECT_EQ(value.CodePointCount(), 1U);
  EXPECT_EQ(value.Size(), 3U);
}

TEST(FixedStringTest, ByteRemovalApisAllowLegacyByteMutation) {
  constexpr char kMixedUtf8[] = {
      'A',
      static_cast<char>(0xC2),  // ¢
      static_cast<char>(0xA2),
      'B',
      static_cast<char>(0x00),
  };
  zcore::FixedString<8> value = zcore::FixedString<8>::FromCString(kMixedUtf8);
  EXPECT_TRUE(value.RemovePrefixBytes(2U));
  EXPECT_FALSE(value.IsValidUtf8());
  EXPECT_TRUE(value.RemoveSuffixBytes(2U));
  EXPECT_TRUE(value.Empty());
}

TEST(FixedStringTest, RandomizedCodePointOpsPreserveUtf8Validity) {
  std::minstd_rand rng(0xA531U);
  for (zcore::usize round = 0U; round < 128U; ++round) {
    zcore::FixedString<64> value;
    const zcore::usize targetCodePoints = 1U + static_cast<zcore::usize>(rng() % 12U);
    for (zcore::usize index = 0U; index < targetCodePoints; ++index) {
      if (!value.TryAppendCodePoint(RandomScalar(rng))) {
        break;
      }
    }

    ASSERT_TRUE(value.IsValidUtf8());
    ASSERT_TRUE(value.TryCodePointCount().HasValue());

    zcore::usize remaining = value.CodePointCount();
    while (remaining > 0U) {
      const zcore::usize drop = 1U + static_cast<zcore::usize>(rng() % remaining);
      const zcore::usize op = static_cast<zcore::usize>(rng() % 3U);
      if (op == 0U) {
        EXPECT_TRUE(value.RemovePrefix(drop));
      } else if (op == 1U) {
        EXPECT_TRUE(value.RemoveSuffix(drop));
      } else {
        for (zcore::usize count = 0U; count < drop; ++count) {
          EXPECT_TRUE(value.TryPopBack());
        }
      }
      EXPECT_TRUE(value.IsValidUtf8());
      remaining = value.Empty() ? 0U : value.CodePointCount();
    }
  }
}

TEST(FixedStringTest, ZeroCapacityBehaviorIsDeterministic) {
  zcore::FixedString<0> value;
  EXPECT_TRUE(value.Empty());
  EXPECT_TRUE(value.Full());
  EXPECT_EQ(value.CStr()[0], '\0');
  EXPECT_FALSE(value.TryPushBack('x'));
  EXPECT_FALSE(value.TryAppend(zcore::StringView("x")));
}

TEST(FixedStringTest, Utf8HelpersValidateAssignAppendAndCount) {
  zcore::FixedString<12> value;
  EXPECT_TRUE(value.TryAssignUtf8(zcore::StringView("hi")));

  constexpr char kCent[] = {
      static_cast<char>(0xC2),
      static_cast<char>(0xA2),
      static_cast<char>(0x00),
  };
  EXPECT_TRUE(value.TryAppendUtf8(zcore::StringView::FromRawUnchecked(kCent, 2U)));
  EXPECT_TRUE(value.IsValidUtf8());
  ASSERT_TRUE(value.TryCodePointCount().HasValue());
  EXPECT_EQ(value.CodePointCount(), 3U);

  constexpr char kInvalid[] = {
      static_cast<char>(0xC0),
      static_cast<char>(0xAF),
      static_cast<char>(0x00),
  };
  EXPECT_FALSE(value.TryAssignUtf8(zcore::StringView::FromRawUnchecked(kInvalid, 2U)));
}

#if GTEST_HAS_DEATH_TEST
TEST(FixedStringContractTest, AppendOverflowTerminates) {
  EXPECT_DEATH(([]() {
                 auto value = zcore::FixedString<3>::FromCString("ab");
                 value.Append(zcore::StringView("cd"));
               }()),
               "");
}

TEST(FixedStringContractTest, IndexOutOfBoundsTerminates) {
  EXPECT_DEATH(([]() {
                 const auto value = zcore::FixedString<3>::FromCString("ab");
                 static_cast<void>(value[2]);
               }()),
               "");
}

TEST(FixedStringContractTest, FromUtf8CStringTerminatesOnInvalidUtf8) {
  EXPECT_DEATH(([]() {
                 constexpr char kInvalid[] = {
                     static_cast<char>(0xC0),
                     static_cast<char>(0xAF),
                     static_cast<char>(0x00),
                 };
                 const auto value = zcore::FixedString<8>::FromUtf8CString(kInvalid);
                 static_cast<void>(value);
               }()),
               "");
}

TEST(FixedStringContractTest, FromCStringTerminatesOnInvalidUtf8) {
  EXPECT_DEATH(([]() {
                 constexpr char kInvalid[] = {
                     static_cast<char>(0xC0),
                     static_cast<char>(0xAF),
                     static_cast<char>(0x00),
                 };
                 const auto value = zcore::FixedString<8>::FromCString(kInvalid);
                 static_cast<void>(value);
               }()),
               "");
}

TEST(FixedStringContractTest, AppendCodePointTerminatesOnInvalidScalar) {
  EXPECT_DEATH(([]() {
                 zcore::FixedString<8> value;
                 value.AppendCodePoint(0xD800U);
               }()),
               "");
}
#endif

}  // namespace
