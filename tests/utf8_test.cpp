/**************************************************************************/
/*  utf8_test.cpp                                                         */
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
 * @file tests/utf8_test.cpp
 * @brief Unit tests for UTF-8 validation and decoding utilities.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/utf8.hpp>

#include <gtest/gtest.h>

#include <array>

namespace {

TEST(Utf8Test, ValidSequencesAreAccepted) {
  constexpr char kAscii[] = "abc";
  EXPECT_TRUE(zcore::utf8::IsValid(kAscii, 3U));

  constexpr std::array<char, 7> kMixed{
      static_cast<char>(0x24),      // $
      static_cast<char>(0xC2),      // ¢
      static_cast<char>(0xA2),
      static_cast<char>(0xE2),      // €
      static_cast<char>(0x82),
      static_cast<char>(0xAC),
      static_cast<char>(0x00),
  };
  EXPECT_TRUE(zcore::utf8::IsValid(kMixed.data(), 6U));
}

TEST(Utf8Test, InvalidSequencesAreRejected) {
  constexpr std::array<char, 2> kOverlong{
      static_cast<char>(0xC0),
      static_cast<char>(0xAF),
  };
  EXPECT_FALSE(zcore::utf8::IsValid(kOverlong.data(), kOverlong.size()));

  constexpr std::array<char, 3> kSurrogate{
      static_cast<char>(0xED),
      static_cast<char>(0xA0),
      static_cast<char>(0x80),
  };
  EXPECT_FALSE(zcore::utf8::IsValid(kSurrogate.data(), kSurrogate.size()));

  constexpr std::array<char, 2> kTruncated{
      static_cast<char>(0xE2),
      static_cast<char>(0x82),
  };
  EXPECT_FALSE(zcore::utf8::IsValid(kTruncated.data(), kTruncated.size()));
}

TEST(Utf8Test, CountCodePointsReturnsExpectedValue) {
  constexpr std::array<char, 10> kPayload{
      static_cast<char>(0x48),      // H
      static_cast<char>(0x69),      // i
      static_cast<char>(0x20),      // space
      static_cast<char>(0xF0),      // 😀
      static_cast<char>(0x9F),
      static_cast<char>(0x98),
      static_cast<char>(0x80),
      static_cast<char>(0x21),      // !
      static_cast<char>(0x00),
      static_cast<char>(0x00),
  };

  const auto count = zcore::utf8::CountCodePoints(kPayload.data(), 8U);
  ASSERT_TRUE(count.HasValue());
  EXPECT_EQ(count.Value(), 5U);
}

TEST(Utf8Test, DecodeAtReturnsCodePointAndWidth) {
  constexpr std::array<char, 5> kPayload{
      static_cast<char>(0xF0),      // 😀
      static_cast<char>(0x9F),
      static_cast<char>(0x98),
      static_cast<char>(0x80),
      static_cast<char>(0x21),      // !
  };

  const auto first = zcore::utf8::DecodeAt(kPayload.data(), kPayload.size(), 0U);
  ASSERT_TRUE(first.HasValue());
  EXPECT_EQ(first.Value().value, 0x1F600U);
  EXPECT_EQ(first.Value().width, 4U);

  const auto second = zcore::utf8::DecodeAt(kPayload.data(), kPayload.size(), 4U);
  ASSERT_TRUE(second.HasValue());
  EXPECT_EQ(second.Value().value, 0x21U);
  EXPECT_EQ(second.Value().width, 1U);

  const auto invalidOffset = zcore::utf8::DecodeAt(kPayload.data(), kPayload.size(), 5U);
  EXPECT_FALSE(invalidOffset.HasValue());
}

TEST(Utf8Test, BoundaryAndAdvanceCodePointsRespectUtf8Units) {
  constexpr std::array<char, 12> kPayload{
      static_cast<char>(0x41),      // A
      static_cast<char>(0xC2),      // ¢
      static_cast<char>(0xA2),
      static_cast<char>(0xE2),      // €
      static_cast<char>(0x82),
      static_cast<char>(0xAC),
      static_cast<char>(0xF0),      // 😀
      static_cast<char>(0x9F),
      static_cast<char>(0x98),
      static_cast<char>(0x80),
      static_cast<char>(0x42),      // B
      static_cast<char>(0x00),
  };

  EXPECT_TRUE(zcore::utf8::IsCodePointBoundary(kPayload.data(), 11U, 0U));
  EXPECT_TRUE(zcore::utf8::IsCodePointBoundary(kPayload.data(), 11U, 1U));
  EXPECT_FALSE(zcore::utf8::IsCodePointBoundary(kPayload.data(), 11U, 2U));
  EXPECT_TRUE(zcore::utf8::IsCodePointBoundary(kPayload.data(), 11U, 3U));

  const auto secondCodePoint = zcore::utf8::AdvanceCodePoints(kPayload.data(), 11U, 0U, 1U);
  ASSERT_TRUE(secondCodePoint.HasValue());
  EXPECT_EQ(secondCodePoint.Value(), 1U);

  const auto emojiOffset = zcore::utf8::AdvanceCodePoints(kPayload.data(), 11U, 0U, 3U);
  ASSERT_TRUE(emojiOffset.HasValue());
  EXPECT_EQ(emojiOffset.Value(), 6U);

  const auto beyondRange = zcore::utf8::AdvanceCodePoints(kPayload.data(), 11U, 0U, 6U);
  EXPECT_FALSE(beyondRange.HasValue());

  const auto fromContinuation = zcore::utf8::AdvanceCodePoints(kPayload.data(), 11U, 2U, 1U);
  EXPECT_FALSE(fromContinuation.HasValue());
}

TEST(Utf8Test, NullPointerRangesAreOnlyValidWhenEmpty) {
  EXPECT_TRUE(zcore::utf8::IsCodePointBoundary(nullptr, 0U, 0U));
  EXPECT_FALSE(zcore::utf8::IsCodePointBoundary(nullptr, 5U, 0U));
  EXPECT_FALSE(zcore::utf8::IsCodePointBoundary(nullptr, 5U, 5U));
}

TEST(Utf8Test, EncodeCodePointRejectsInvalidScalarsAndRoundTrips) {
  char out[4] = {};
  EXPECT_FALSE(zcore::utf8::EncodeCodePoint(0xD800U, out, sizeof(out)).HasValue());
  EXPECT_FALSE(zcore::utf8::EncodeCodePoint(0x110000U, out, sizeof(out)).HasValue());
  EXPECT_FALSE(zcore::utf8::EncodeCodePoint(0x1F600U, out, 3U).HasValue());

  const auto asciiWidth = zcore::utf8::EncodeCodePoint(0x41U, out, sizeof(out));
  ASSERT_TRUE(asciiWidth.HasValue());
  EXPECT_EQ(asciiWidth.Value(), 1U);
  auto decoded = zcore::utf8::DecodeAt(out, asciiWidth.Value(), 0U);
  ASSERT_TRUE(decoded.HasValue());
  EXPECT_EQ(decoded.Value().value, 0x41U);

  const auto emojiWidth = zcore::utf8::EncodeCodePoint(0x1F600U, out, sizeof(out));
  ASSERT_TRUE(emojiWidth.HasValue());
  EXPECT_EQ(emojiWidth.Value(), 4U);
  decoded = zcore::utf8::DecodeAt(out, emojiWidth.Value(), 0U);
  ASSERT_TRUE(decoded.HasValue());
  EXPECT_EQ(decoded.Value().value, 0x1F600U);
}

}  // namespace
