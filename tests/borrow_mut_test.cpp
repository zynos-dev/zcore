/**************************************************************************/
/*  borrow_mut_test.cpp                                                   */
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
 * @file tests/borrow_mut_test.cpp
 * @brief Unit tests for mutable borrow wrapper behavior.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/borrow_mut.hpp>

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
  void Set(int next) noexcept {
    value = next;
  }
  [[nodiscard]] int Read() const noexcept {
    return value;
  }
};

static_assert(sizeof(zcore::BorrowMut<int>) == sizeof(int*));
static_assert(std::is_trivially_copyable_v<zcore::BorrowMut<int>>);
static_assert(!std::is_default_constructible_v<zcore::BorrowMut<int>>);
static_assert(std::is_same_v<decltype(*std::declval<zcore::BorrowMut<int>>()), int&>);

TEST(BorrowMutTest, ConstructsFromPointerAndReferenceAndAllowsMutation) {
  Widget widget{.value = 7};  // NOLINT(misc-const-correctness)
  const zcore::BorrowMut<Widget> fromPointer(&widget);
  const zcore::BorrowMut<Widget> fromReference(widget);

  fromPointer->Set(8);
  (*fromReference).Set(9);

  EXPECT_EQ(fromPointer.Get(), &widget);
  EXPECT_EQ(fromReference.Get(), &widget);
  EXPECT_EQ(widget.value, 9);
}

TEST(BorrowMutTest, SupportsVoidPointers) {
  int value = 11;  // NOLINT(misc-const-correctness)
  const zcore::BorrowMut<void> ptr(static_cast<void*>(&value));

  EXPECT_EQ(ptr.Get(), static_cast<void*>(&value));
  EXPECT_NE(ptr.Address(), 0U);
}

TEST(BorrowMutTest, SupportsCovariantAndNonNullConversions) {
  Derived derived{};  // NOLINT(misc-const-correctness)
  derived.value = 19;
  derived.extra = 2;
  const zcore::NonNull<Derived> nonNullDerived(&derived);

  const zcore::BorrowMut<Derived> derivedBorrow(nonNullDerived);
  const zcore::BorrowMut<Base> baseBorrow(derivedBorrow);

  baseBorrow->value = 21;
  EXPECT_EQ(derived.value, 21);
  EXPECT_EQ(baseBorrow.Address(), derivedBorrow.Address());
}

TEST(BorrowMutTest, ConvertsToImmutableBorrow) {
  Widget widget{.value = 5};  // NOLINT(misc-const-correctness)
  const zcore::BorrowMut<Widget> mutBorrow(widget);

  const zcore::Borrow<Widget> immutableBorrow = mutBorrow;
  EXPECT_EQ(immutableBorrow->Read(), 5);
  EXPECT_EQ(immutableBorrow.Address(), mutBorrow.Address());
}

TEST(BorrowMutTest, UnsafeFromPointerUncheckedPreservesAddress) {
  int value = 5;  // NOLINT(misc-const-correctness)
  const auto ptr = zcore::BorrowMut<int>::UnsafeFromPointerUnchecked(&value);

  *ptr = 6;
  EXPECT_EQ(ptr.Get(), &value);
  EXPECT_EQ(value, 6);
}

#if GTEST_HAS_DEATH_TEST
TEST(BorrowMutTest, NullPointerConstructionTerminates) {
  EXPECT_DEATH(
      {
        auto invalid = zcore::BorrowMut<int>(static_cast<int*>(nullptr));
        static_cast<void>(invalid);
      },
      "");
}
#endif

}  // namespace
