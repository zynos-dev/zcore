/**************************************************************************/
/*  non_null_test.cpp                                                     */
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
 * @file tests/non_null_test.cpp
 * @brief Unit tests for NonNull invariants and casts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/non_null.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <type_traits>

namespace {

struct Base {
  int value;
};

struct Derived final : Base {
  int extra;
};

struct Widget final {
  int value;
  [[nodiscard]] int Read() const noexcept {
    return value;
  }
};

static_assert(sizeof(zcore::NonNull<int>) == sizeof(int*));
static_assert(std::is_trivially_copyable_v<zcore::NonNull<int>>);

TEST(NonNullTest, ConstructsFromPointerAndReference) {
  Widget widget{.value = 7};
  zcore::NonNull<Widget> fromPointer(&widget);
  zcore::NonNull<Widget> fromReference(widget);

  EXPECT_EQ(fromPointer.Get(), &widget);
  EXPECT_EQ(fromReference.Get(), &widget);
  EXPECT_EQ(fromPointer->Read(), 7);
  EXPECT_EQ((*fromReference).value, 7);
}

TEST(NonNullTest, SupportsVoidPointers) {
  int value = 11;
  zcore::NonNull<void> ptr(static_cast<void*>(&value));

  EXPECT_EQ(ptr.Get(), static_cast<void*>(&value));
  EXPECT_NE(ptr.Address(), 0U);
}

TEST(NonNullTest, SupportsCovariantConstruction) {
  Widget widget{.value = 3};  // NOLINT(misc-const-correctness)
  zcore::NonNull<Widget> mutablePtr(&widget);
  widget.value = 4;
  zcore::NonNull<const Widget> constPtr(mutablePtr);

  EXPECT_EQ(constPtr->Read(), 4);
}

TEST(NonNullTest, StaticCastAndReinterpretCastAreExplicit) {
  Derived derived{};
  derived.value = 19;
  derived.extra = 2;
  zcore::NonNull<Derived> derivedPtr(&derived);

  zcore::NonNull<Base> basePtr = derivedPtr.StaticCast<Base>();
  EXPECT_EQ(basePtr->value, 19);

  zcore::NonNull<std::byte> bytes = derivedPtr.ReinterpretCast<std::byte>();
  EXPECT_EQ(bytes.Address(), derivedPtr.Address());
}

TEST(NonNullTest, ConstCastRemovesConstQualifierExplicitly) {
  Widget widget{.value = 21};  // NOLINT(misc-const-correctness)
  zcore::NonNull<const Widget> constPtr(&widget);
  zcore::NonNull<Widget> mutablePtr = constPtr.ConstCast();

  mutablePtr->value = 22;
  EXPECT_EQ(widget.value, 22);
}

TEST(NonNullTest, UnsafeFromPointerUncheckedPreservesAddress) {
  int value = 5;
  auto ptr = zcore::NonNull<int>::UnsafeFromPointerUnchecked(&value);

  EXPECT_EQ(ptr.Get(), &value);
  EXPECT_EQ(*ptr, 5);
}

#if GTEST_HAS_DEATH_TEST
TEST(NonNullTest, NullPointerConstructionTerminates) {
  EXPECT_DEATH(
      {
        auto invalid = zcore::NonNull<int>(static_cast<int*>(nullptr));
        static_cast<void>(invalid);
      },
      "");
}
#endif

}  // namespace
