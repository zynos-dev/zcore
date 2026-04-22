/**************************************************************************/
/*  borrow_test.cpp                                                       */
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
 * @file tests/borrow_test.cpp
 * @brief Unit tests for immutable borrow wrapper behavior.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/borrow.hpp>

#include <gtest/gtest.h>

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

static_assert(sizeof(zcore::Borrow<int>) == sizeof(const int*));
static_assert(std::is_trivially_copyable_v<zcore::Borrow<int>>);
static_assert(!std::is_default_constructible_v<zcore::Borrow<int>>);
static_assert(std::is_same_v<decltype(*std::declval<zcore::Borrow<int>>()), const int&>);

TEST(BorrowTest, ConstructsFromPointerAndReference) {
  const Widget widget{.value = 7};
  const zcore::Borrow<Widget> fromPointer(&widget);
  const zcore::Borrow<Widget> fromReference(widget);

  EXPECT_EQ(fromPointer.Get(), &widget);
  EXPECT_EQ(fromReference.Get(), &widget);
  EXPECT_EQ(fromPointer->Read(), 7);
  EXPECT_EQ((*fromReference).value, 7);
}

TEST(BorrowTest, SupportsVoidPointers) {
  const int value = 11;
  const zcore::Borrow<void> ptr(static_cast<const void*>(&value));

  EXPECT_EQ(ptr.Get(), static_cast<const void*>(&value));
  EXPECT_NE(ptr.Address(), 0U);
}

TEST(BorrowTest, SupportsCovariantAndNonNullConversions) {
  Derived derived{};
  derived.value = 19;
  derived.extra = 2;
  const zcore::NonNull<Derived> nonNullDerived(&derived);

  const zcore::Borrow<Derived> derivedBorrow(nonNullDerived);
  const zcore::Borrow<Base> baseBorrow(derivedBorrow);

  EXPECT_EQ(baseBorrow->value, 19);
  EXPECT_EQ(baseBorrow.Address(), derivedBorrow.Address());
}

TEST(BorrowTest, UnsafeFromPointerUncheckedPreservesAddress) {
  const int value = 5;
  const auto ptr = zcore::Borrow<int>::UnsafeFromPointerUnchecked(&value);

  EXPECT_EQ(ptr.Get(), &value);
  EXPECT_EQ(*ptr, 5);
}

#if GTEST_HAS_DEATH_TEST
TEST(BorrowTest, NullPointerConstructionTerminates) {
  EXPECT_DEATH(
      {
        auto invalid = zcore::Borrow<int>(static_cast<int*>(nullptr));
        static_cast<void>(invalid);
      },
      "");
}
#endif

}  // namespace
