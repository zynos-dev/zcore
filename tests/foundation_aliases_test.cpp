/**************************************************************************/
/*  foundation_aliases_test.cpp                                           */
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
 * @file tests/foundation_aliases_test.cpp
 * @brief Unit tests for foundational alias contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/foundation.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace {

static_assert(std::is_same_v<zcore::u8, std::uint8_t>);
static_assert(std::is_same_v<zcore::u16, std::uint16_t>);
static_assert(std::is_same_v<zcore::u32, std::uint32_t>);
static_assert(std::is_same_v<zcore::u64, std::uint64_t>);

static_assert(std::is_same_v<zcore::i8, std::int8_t>);
static_assert(std::is_same_v<zcore::i16, std::int16_t>);
static_assert(std::is_same_v<zcore::i32, std::int32_t>);
static_assert(std::is_same_v<zcore::i64, std::int64_t>);

static_assert(std::is_same_v<zcore::usize, std::size_t>);
static_assert(std::is_same_v<zcore::isize, std::ptrdiff_t>);
static_assert(std::is_same_v<zcore::Byte, std::byte>);

TEST(FoundationAliasesTest, FixedWidthAliasesHaveExpectedSizes) {
  EXPECT_EQ(sizeof(zcore::u8), 1U);
  EXPECT_EQ(sizeof(zcore::u16), 2U);
  EXPECT_EQ(sizeof(zcore::u32), 4U);
  EXPECT_EQ(sizeof(zcore::u64), 8U);

  EXPECT_EQ(sizeof(zcore::i8), 1U);
  EXPECT_EQ(sizeof(zcore::i16), 2U);
  EXPECT_EQ(sizeof(zcore::i32), 4U);
  EXPECT_EQ(sizeof(zcore::i64), 8U);
}

TEST(FoundationAliasesTest, ByteAliasMatchesStdByteSemantics) {
  const zcore::Byte value = std::byte{0x2A};
  EXPECT_EQ(std::to_integer<unsigned int>(value), 0x2AU);
}

}  // namespace
