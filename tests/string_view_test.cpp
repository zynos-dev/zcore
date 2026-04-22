/**************************************************************************/
/*  string_view_test.cpp                                                  */
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
 * @file tests/string_view_test.cpp
 * @brief Unit tests for `zcore::StringView` contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/string_view.hpp>

#include <gtest/gtest.h>

#include <random>
#include <string>
#include <string_view>
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

std::string BuildRandomUtf8Text(std::minstd_rand& rng, zcore::usize maxCodePoints) {
  std::string out;
  const zcore::usize count = 1U + static_cast<zcore::usize>(rng() % maxCodePoints);
  for (zcore::usize index = 0U; index < count; ++index) {
    const zcore::u32 scalar = RandomScalar(rng);
    char encoded[4] = {};
    const auto width = zcore::utf8::EncodeCodePoint(scalar, encoded, sizeof(encoded));
    if (!width.HasValue()) {
      continue;
    }
    out.append(encoded, encoded + width.Value());
  }
  return out;
}

static_assert(std::is_trivially_copyable_v<zcore::StringView>);

TEST(StringViewTest, DefaultConstructedIsEmpty) {
  const zcore::StringView value;
  EXPECT_TRUE(value.EmptyView());
  EXPECT_EQ(value.Size(), 0U);
  EXPECT_EQ(value.Data(), nullptr);
}

TEST(StringViewTest, ConstructsFromCStringAndArray) {
  const zcore::StringView cstr = zcore::StringView::FromCString("zcore");
  EXPECT_EQ(cstr.Size(), 5U);
  EXPECT_EQ(cstr[0], 'z');
  EXPECT_EQ(cstr[4], 'e');

  const zcore::StringView array("core");
  EXPECT_EQ(array.Size(), 4U);
  EXPECT_EQ(array[0], 'c');
  EXPECT_EQ(array[3], 'e');
}

TEST(StringViewTest, ConstructsFromStdStringView) {
  const std::string_view source = "alpha";
  const zcore::StringView value(source);
  EXPECT_EQ(value.Size(), 5U);
  EXPECT_EQ(value.AsStdStringView(), source);
}

TEST(StringViewTest, TryFromRawAndTryFromCStringAreFallible) {
  const auto invalidRange = zcore::StringView::TryFromRaw(nullptr, 2U);
  EXPECT_FALSE(invalidRange.HasValue());

  const auto validEmpty = zcore::StringView::TryFromRaw(nullptr, 0U);
  ASSERT_TRUE(validEmpty.HasValue());
  EXPECT_TRUE(validEmpty.Value().EmptyView());

  const auto nullCString = zcore::StringView::TryFromCString(nullptr);
  EXPECT_FALSE(nullCString.HasValue());

  constexpr char kInvalidUtf8[] = {
      static_cast<char>(0xC0),
      static_cast<char>(0xAF),
      static_cast<char>(0x00),
  };
  EXPECT_FALSE(zcore::StringView::TryFromRaw(kInvalidUtf8, 2U).HasValue());
  EXPECT_FALSE(zcore::StringView::TryFromCString(kInvalidUtf8).HasValue());
}

TEST(StringViewTest, FirstLastAndSubstrValidateRanges) {
  const zcore::StringView value = zcore::StringView::FromCString("abcdef");

  const auto first = value.First(2U);
  ASSERT_TRUE(first.HasValue());
  EXPECT_EQ(first.Value().AsStdStringView(), "ab");

  const auto last = value.Last(2U);
  ASSERT_TRUE(last.HasValue());
  EXPECT_EQ(last.Value().AsStdStringView(), "ef");

  const auto mid = value.Substr(2U, 3U);
  ASSERT_TRUE(mid.HasValue());
  EXPECT_EQ(mid.Value().AsStdStringView(), "cde");

  EXPECT_FALSE(value.Substr(5U, 2U).HasValue());
}

TEST(StringViewTest, RemovePrefixAndSuffixMutateViewWindow) {
  zcore::StringView value = zcore::StringView::FromCString("abcdef");
  EXPECT_TRUE(value.RemovePrefix(1U));
  EXPECT_TRUE(value.RemoveSuffix(2U));
  EXPECT_EQ(value.AsStdStringView(), "bcd");
  EXPECT_FALSE(value.RemovePrefix(4U));
}

TEST(StringViewTest, TryAtAndConversionsAreStable) {
  const zcore::StringView value = zcore::StringView::FromCString("xyz");
  ASSERT_NE(value.TryAt(1U), nullptr);
  EXPECT_EQ(*value.TryAt(1U), 'y');
  EXPECT_EQ(value.TryAt(3U), nullptr);

  const zcore::Slice<const char> slice = value.AsSlice();
  EXPECT_EQ(slice.Size(), 3U);
  EXPECT_EQ(slice[0], 'x');
  EXPECT_EQ(slice[2], 'z');
}

TEST(StringViewTest, EqualityUsesContentNotPointerIdentity) {
  const zcore::StringView a("same");
  const zcore::StringView b = zcore::StringView::FromCString("same");
  const zcore::StringView c("diff");
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);
}

TEST(StringViewTest, CoreWindowApisRemainDeterministicForUtf8Bytes) {
  constexpr char kMixedUtf8[] = {
      'A',
      static_cast<char>(0xC2),
      static_cast<char>(0xA2),
      'B',
      static_cast<char>(0x00),
  };
  const zcore::StringView value = zcore::StringView::FromCString(kMixedUtf8);
  EXPECT_EQ(value.Size(), 4U);
  EXPECT_EQ(value[0], 'A');
  EXPECT_EQ(static_cast<unsigned char>(value[1]), 0xC2U);
  EXPECT_EQ(static_cast<unsigned char>(value[2]), 0xA2U);
  EXPECT_EQ(value[3], 'B');

  const auto mid = value.Substr(1U, 1U);
  ASSERT_TRUE(mid.HasValue());
  EXPECT_EQ(static_cast<unsigned char>(mid.Value()[0]), 0xC2U);
  EXPECT_EQ(static_cast<unsigned char>(mid.Value()[1]), 0xA2U);

  zcore::StringView mutableView = value;
  EXPECT_TRUE(mutableView.RemovePrefix(1U));
  EXPECT_TRUE(mutableView.RemoveSuffix(1U));
  EXPECT_EQ(mutableView.Size(), 2U);
  EXPECT_TRUE(mutableView == mid.Value());
}

TEST(StringViewTest, CodePointWindowOpsHandleMultiByteUtf8WithoutSplitting) {
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

  zcore::StringView value = zcore::StringView::FromCString(kMultiUtf8);
  ASSERT_TRUE(value.IsValidUtf8());
  EXPECT_EQ(value.CodePointCount(), 5U);

  const auto middle = value.Substr(1U, 3U);
  ASSERT_TRUE(middle.HasValue());
  EXPECT_TRUE(middle.Value().IsValidUtf8());
  EXPECT_EQ(middle.Value().CodePointCount(), 3U);
  EXPECT_EQ(middle.Value().Size(), 9U);

  EXPECT_TRUE(value.RemovePrefix(2U));
  EXPECT_TRUE(value.IsValidUtf8());
  EXPECT_EQ(value.CodePointCount(), 3U);

  EXPECT_TRUE(value.RemoveSuffix(1U));
  EXPECT_TRUE(value.IsValidUtf8());
  EXPECT_EQ(value.CodePointCount(), 2U);
  EXPECT_FALSE(value.Substr(3U, 1U).HasValue());
}

TEST(StringViewTest, ByteApisAllowLegacyByteWindowing) {
  constexpr char kMultiUtf8[] = {
      'A',
      static_cast<char>(0xC2),  // ¢
      static_cast<char>(0xA2),
      static_cast<char>(0x00),
  };

  const zcore::StringView value = zcore::StringView::FromCString(kMultiUtf8);
  const auto split = value.SubstrBytes(2U, 1U);
  ASSERT_TRUE(split.HasValue());
  EXPECT_EQ(split.Value().Size(), 1U);
  EXPECT_FALSE(split.Value().IsValidUtf8());

  zcore::StringView mutated = value;
  EXPECT_TRUE(mutated.RemovePrefixBytes(2U));
  EXPECT_FALSE(mutated.IsValidUtf8());
  EXPECT_TRUE(mutated.RemoveSuffixBytes(1U));
  EXPECT_TRUE(mutated.EmptyView());
}

TEST(StringViewTest, RandomizedCodePointOpsPreserveUtf8Validity) {
  std::minstd_rand rng(0x5A17U);
  for (zcore::usize round = 0U; round < 128U; ++round) {
    const std::string text = BuildRandomUtf8Text(rng, 12U);
    const zcore::StringView base = zcore::StringView::FromCString(text.c_str());
    ASSERT_TRUE(base.IsValidUtf8());
    const zcore::usize total = base.CodePointCount();

    const zcore::usize offset = static_cast<zcore::usize>(rng() % (total + 1U));
    const zcore::usize count = static_cast<zcore::usize>(rng() % ((total - offset) + 1U));
    const auto sub = base.Substr(offset, count);
    ASSERT_TRUE(sub.HasValue());
    EXPECT_TRUE(sub.Value().IsValidUtf8());
    EXPECT_EQ(sub.Value().CodePointCount(), count);

    zcore::StringView mutableView = base;
    zcore::usize remaining = mutableView.CodePointCount();
    while (remaining > 0U) {
      const zcore::usize drop = 1U + static_cast<zcore::usize>(rng() % remaining);
      if ((rng() % 2U) == 0U) {
        EXPECT_TRUE(mutableView.RemovePrefix(drop));
      } else {
        EXPECT_TRUE(mutableView.RemoveSuffix(drop));
      }
      EXPECT_TRUE(mutableView.IsValidUtf8());
      remaining = mutableView.EmptyView() ? 0U : mutableView.CodePointCount();
    }
  }
}

TEST(StringViewTest, Utf8HelpersValidateAndCountCodePoints) {
  constexpr char kValidUtf8[] = {
      static_cast<char>(0xF0),  // 😀
      static_cast<char>(0x9F),
      static_cast<char>(0x98),
      static_cast<char>(0x80),
      static_cast<char>(0x00),
  };
  const zcore::StringView valid = zcore::StringView::FromRawUnchecked(kValidUtf8, 4U);
  EXPECT_TRUE(valid.IsValidUtf8());
  ASSERT_TRUE(valid.TryCodePointCount().HasValue());
  EXPECT_EQ(valid.CodePointCount(), 1U);

  constexpr char kInvalidUtf8[] = {
      static_cast<char>(0xC0),
      static_cast<char>(0xAF),
      static_cast<char>(0x00),
  };
  const zcore::StringView invalid = zcore::StringView::FromRawUnchecked(kInvalidUtf8, 2U);
  EXPECT_FALSE(invalid.IsValidUtf8());
  EXPECT_FALSE(invalid.TryCodePointCount().HasValue());
  EXPECT_FALSE(zcore::StringView::TryFromUtf8CString(kInvalidUtf8).HasValue());
}

#if GTEST_HAS_DEATH_TEST
TEST(StringViewContractTest, CheckedRawConstructorTerminatesOnNullNonZeroRange) {
  EXPECT_DEATH(([]() {
                 const zcore::StringView invalid(nullptr, 1U);
                 static_cast<void>(invalid);
               }()),
               "");
}

TEST(StringViewContractTest, FromCStringTerminatesOnNull) {
  EXPECT_DEATH(([]() {
                 const zcore::StringView invalid = zcore::StringView::FromCString(nullptr);
                 static_cast<void>(invalid);
               }()),
               "");
}

TEST(StringViewContractTest, FromCStringTerminatesOnInvalidUtf8) {
  EXPECT_DEATH(([]() {
                 constexpr char kInvalidUtf8[] = {
                     static_cast<char>(0xC0),
                     static_cast<char>(0xAF),
                     static_cast<char>(0x00),
                 };
                 const zcore::StringView invalid = zcore::StringView::FromCString(kInvalidUtf8);
                 static_cast<void>(invalid);
               }()),
               "");
}

TEST(StringViewContractTest, IndexOutOfBoundsTerminates) {
  EXPECT_DEATH(([]() {
                 const zcore::StringView value = zcore::StringView::FromCString("ab");
                 static_cast<void>(value[2]);
               }()),
               "");
}

TEST(StringViewContractTest, FromUtf8CStringTerminatesOnInvalidUtf8) {
  EXPECT_DEATH(([]() {
                 constexpr char kInvalidUtf8[] = {
                     static_cast<char>(0xC0),
                     static_cast<char>(0xAF),
                     static_cast<char>(0x00),
                 };
                 const zcore::StringView value = zcore::StringView::FromUtf8CString(kInvalidUtf8);
                 static_cast<void>(value);
               }()),
               "");
}
#endif

}  // namespace
