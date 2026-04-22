/**************************************************************************/
/*  io_interfaces_test.cpp                                                */
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
 * @file tests/io_interfaces_test.cpp
 * @brief Unit tests for `Reader`, `Writer`, and `Seeker` interface contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/io_error.hpp>
#include <zcore/reader.hpp>
#include <zcore/seeker.hpp>
#include <zcore/writer.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <limits>

namespace {

[[nodiscard]] unsigned int ByteValue(zcore::Byte value) {
  return std::to_integer<unsigned int>(value);
}

class MemoryReader final : public zcore::Reader {
 public:
  explicit MemoryReader(zcore::ByteSpan source) noexcept : Source_(source) {}

  [[nodiscard]] zcore::Result<zcore::usize, zcore::Error> Read(zcore::ByteSpanMut destination) noexcept override {
    if (destination.EmptyView()) {
      return zcore::Result<zcore::usize, zcore::Error>::Success(0U);
    }

    const zcore::usize remaining = Cursor_ < Source_.Size() ? (Source_.Size() - Cursor_) : 0U;
    const zcore::usize toRead = remaining < destination.Size() ? remaining : destination.Size();
    for (zcore::usize index = 0U; index < toRead; ++index) {
      destination[index] = Source_[Cursor_ + index];
    }
    Cursor_ += toRead;
    return zcore::Result<zcore::usize, zcore::Error>::Success(toRead);
  }

 private:
  zcore::ByteSpan Source_;
  zcore::usize Cursor_{0U};
};

class MemoryWriter final : public zcore::Writer {
 public:
  explicit MemoryWriter(zcore::ByteSpanMut destination) noexcept : Destination_(destination) {}

  void SetFlushFailure(bool enabled) noexcept {
    FailFlush_ = enabled;
  }

  [[nodiscard]] zcore::Result<zcore::usize, zcore::Error> Write(zcore::ByteSpan source) noexcept override {
    if (source.EmptyView()) {
      return zcore::Result<zcore::usize, zcore::Error>::Success(0U);
    }

    const zcore::usize remaining = Cursor_ < Destination_.Size() ? (Destination_.Size() - Cursor_) : 0U;
    const zcore::usize toWrite = remaining < source.Size() ? remaining : source.Size();
    for (zcore::usize index = 0U; index < toWrite; ++index) {
      Destination_[Cursor_ + index] = source[index];
    }
    Cursor_ += toWrite;
    return zcore::Result<zcore::usize, zcore::Error>::Success(toWrite);
  }

  [[nodiscard]] zcore::Status Flush() noexcept override {
    if (FailFlush_) {
      return zcore::ErrorStatus(
          zcore::MakeIoError(zcore::IoErrorCode::UNSUPPORTED_OPERATION, "flush", "flush not supported"));
    }
    return zcore::OkStatus();
  }

 private:
  zcore::ByteSpanMut Destination_;
  zcore::usize Cursor_{0U};
  bool FailFlush_{false};
};

class RangeSeeker final : public zcore::Seeker {
 public:
  explicit RangeSeeker(zcore::usize size) noexcept : Size_(size) {}

  [[nodiscard]] zcore::Result<zcore::usize, zcore::Error> Seek(
      zcore::isize offset,
      zcore::SeekOrigin origin) noexcept override {
    const zcore::isize base = BaseFor(origin);
    const zcore::isize target = base + offset;
    if (target < 0 || target > static_cast<zcore::isize>(Size_)) {
      return zcore::Result<zcore::usize, zcore::Error>::Failure(
          zcore::MakeIoError(zcore::IoErrorCode::OUT_OF_RANGE, "seek", "seek position out of range"));
    }
    Position_ = static_cast<zcore::usize>(target);
    return zcore::Result<zcore::usize, zcore::Error>::Success(Position_);
  }

 private:
  [[nodiscard]] zcore::isize BaseFor(zcore::SeekOrigin origin) const noexcept {
    if (origin == zcore::SeekOrigin::BEGIN) {
      return 0;
    }
    if (origin == zcore::SeekOrigin::CURRENT) {
      return static_cast<zcore::isize>(Position_);
    }
    return static_cast<zcore::isize>(Size_);
  }

  zcore::usize Size_;
  zcore::usize Position_{0U};
};

TEST(IoInterfaceTest, ReaderReadsUpToDestinationCapacity) {
  const std::array<zcore::Byte, 4U> source{
      static_cast<zcore::Byte>(0x10U),
      static_cast<zcore::Byte>(0x20U),
      static_cast<zcore::Byte>(0x30U),
      static_cast<zcore::Byte>(0x40U),
  };
  MemoryReader reader{zcore::ByteSpan(source)};

  std::array<zcore::Byte, 3U> destination{};
  auto firstRead = reader.Read(zcore::ByteSpanMut(destination));
  ASSERT_TRUE(firstRead.HasValue());
  EXPECT_EQ(firstRead.Value(), 3U);
  EXPECT_EQ(ByteValue(destination[0]), 0x10U);
  EXPECT_EQ(ByteValue(destination[1]), 0x20U);
  EXPECT_EQ(ByteValue(destination[2]), 0x30U);

  auto secondRead = reader.Read(zcore::ByteSpanMut(destination));
  ASSERT_TRUE(secondRead.HasValue());
  EXPECT_EQ(secondRead.Value(), 1U);
  EXPECT_EQ(ByteValue(destination[0]), 0x40U);

  auto thirdRead = reader.Read(zcore::ByteSpanMut(destination));
  ASSERT_TRUE(thirdRead.HasValue());
  EXPECT_EQ(thirdRead.Value(), 0U);
}

TEST(IoInterfaceTest, WriterWritesAndFlushes) {
  std::array<zcore::Byte, 4U> storage{};
  MemoryWriter writer{zcore::ByteSpanMut(storage)};
  const std::array<zcore::Byte, 3U> source{
      static_cast<zcore::Byte>(0xAAU),
      static_cast<zcore::Byte>(0xBBU),
      static_cast<zcore::Byte>(0xCCU),
  };

  auto writeResult = writer.Write(zcore::ByteSpan(source));
  ASSERT_TRUE(writeResult.HasValue());
  EXPECT_EQ(writeResult.Value(), 3U);
  EXPECT_EQ(ByteValue(storage[0]), 0xAAU);
  EXPECT_EQ(ByteValue(storage[1]), 0xBBU);
  EXPECT_EQ(ByteValue(storage[2]), 0xCCU);

  const zcore::Status flushStatus = writer.Flush();
  EXPECT_TRUE(flushStatus.HasValue());
}

TEST(IoInterfaceTest, WriterFlushCanReportIoError) {
  std::array<zcore::Byte, 2U> storage{};
  MemoryWriter writer{zcore::ByteSpanMut(storage)};
  writer.SetFlushFailure(true);

  const zcore::Status flushStatus = writer.Flush();
  ASSERT_TRUE(flushStatus.HasError());
  EXPECT_EQ(flushStatus.Error().code.domain.id, zcore::kIoErrorDomain.id);
  EXPECT_EQ(
      flushStatus.Error().code.value,
      static_cast<zcore::i32>(zcore::IoErrorCode::UNSUPPORTED_OPERATION));
}

TEST(IoInterfaceTest, SeekerSupportsBeginCurrentEndAndHelpers) {
  RangeSeeker seeker(10U);

  auto fromStart = seeker.Seek(4, zcore::SeekOrigin::BEGIN);
  ASSERT_TRUE(fromStart.HasValue());
  EXPECT_EQ(fromStart.Value(), 4U);

  auto fromCurrent = seeker.Seek(-2, zcore::SeekOrigin::CURRENT);
  ASSERT_TRUE(fromCurrent.HasValue());
  EXPECT_EQ(fromCurrent.Value(), 2U);

  auto fromEnd = seeker.Seek(-1, zcore::SeekOrigin::END);
  ASSERT_TRUE(fromEnd.HasValue());
  EXPECT_EQ(fromEnd.Value(), 9U);

  auto rewinded = seeker.Rewind();
  ASSERT_TRUE(rewinded.HasValue());
  EXPECT_EQ(rewinded.Value(), 0U);

  auto seekTo = seeker.SeekTo(7U);
  ASSERT_TRUE(seekTo.HasValue());
  EXPECT_EQ(seekTo.Value(), 7U);
}

TEST(IoInterfaceTest, SeekerRejectsOutOfRangeOffsets) {
  RangeSeeker seeker(10U);

  auto beforeStart = seeker.Seek(-1, zcore::SeekOrigin::BEGIN);
  ASSERT_TRUE(beforeStart.HasError());
  EXPECT_EQ(
      beforeStart.Error().code.value,
      static_cast<zcore::i32>(zcore::IoErrorCode::OUT_OF_RANGE));

  auto pastEnd = seeker.Seek(1, zcore::SeekOrigin::END);
  ASSERT_TRUE(pastEnd.HasError());
  EXPECT_EQ(
      pastEnd.Error().code.value,
      static_cast<zcore::i32>(zcore::IoErrorCode::OUT_OF_RANGE));

  auto seekToTooLarge = seeker.SeekTo(static_cast<zcore::usize>(std::numeric_limits<zcore::isize>::max()) + 1U);
  ASSERT_TRUE(seekToTooLarge.HasError());
  EXPECT_EQ(
      seekToTooLarge.Error().code.value,
      static_cast<zcore::i32>(zcore::IoErrorCode::OUT_OF_RANGE));
}

}  // namespace
