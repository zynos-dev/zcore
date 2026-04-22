/**************************************************************************/
/*  endian_test.cpp                                                       */
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
 * @file tests/endian_test.cpp
 * @brief Unit tests for endian conversion helpers.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/endian.hpp>
#include <zcore/foundation.hpp>

#include <gtest/gtest.h>

namespace {

enum class PacketType : zcore::u16 {
  DATA = 0x1234U,
  DATA_SWAPPED = 0x3412U,
};

static_assert(zcore::endian::EndianValue<zcore::u32>);
static_assert(zcore::endian::EndianValue<PacketType>);
static_assert(!zcore::endian::EndianValue<bool>);

TEST(EndianTest, NativeEndianQueriesAreMutuallyExclusive) {
  EXPECT_FALSE(zcore::endian::IsNativeLittleEndian() && zcore::endian::IsNativeBigEndian());
}

TEST(EndianTest, ByteSwapTransformsIntegralAndEnumValues) {
  const zcore::u16 value16 = 0x1234U;
  const zcore::u32 value32 = 0x11223344U;

  EXPECT_EQ(zcore::endian::ByteSwap(value16), 0x3412U);
  EXPECT_EQ(zcore::endian::ByteSwap(value32), 0x44332211U);
  EXPECT_EQ(zcore::endian::ByteSwap(static_cast<zcore::u8>(0xABU)), static_cast<zcore::u8>(0xABU));

  const PacketType swappedEnum = zcore::endian::ByteSwap(PacketType::DATA);
  EXPECT_EQ(swappedEnum, PacketType::DATA_SWAPPED);
}

TEST(EndianTest, LittleEndianRoundTripPreservesValue) {
  const zcore::u64 value = 0x0102030405060708ULL;
  const zcore::u64 encoded = zcore::endian::ToLittleEndian(value);
  const zcore::u64 decoded = zcore::endian::FromLittleEndian(encoded);
  EXPECT_EQ(decoded, value);
}

TEST(EndianTest, BigEndianRoundTripPreservesValue) {
  const zcore::u64 value = 0xA1B2C3D4E5F60718ULL;
  const zcore::u64 encoded = zcore::endian::ToBigEndian(value);
  const zcore::u64 decoded = zcore::endian::FromBigEndian(encoded);
  EXPECT_EQ(decoded, value);
}

TEST(EndianTest, ShortAliasesMatchCanonicalFunctions) {
  const zcore::u32 value = 0xCAFEBABEU;
  EXPECT_EQ(zcore::endian::to_le(value), zcore::endian::ToLittleEndian(value));
  EXPECT_EQ(zcore::endian::from_le(value), zcore::endian::FromLittleEndian(value));
  EXPECT_EQ(zcore::endian::to_be(value), zcore::endian::ToBigEndian(value));
  EXPECT_EQ(zcore::endian::from_be(value), zcore::endian::FromBigEndian(value));
}

}  // namespace
