/**************************************************************************/
/*  option_test.cpp                                                       */
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
 * @file tests/option_test.cpp
 * @brief Unit tests for Option behavior and combinators.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/option.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <string>

namespace {

using zcore::None;
using zcore::Option;
using zcore::Some;

template <typename ValueT, typename FuncT>
concept OptionAndThenInvocable = requires(const Option<ValueT>& value, FuncT fn) {
  { value.AndThen(fn) };
};

struct ReturnsOption final {
  [[nodiscard]] Option<int> operator()(const int value) const {
    return Option<int>(value);
  }
};

struct ReturnsInt final {
  [[nodiscard]] int operator()(const int value) const {
    return value;
  }
};

TEST(OptionTest, DefaultConstructedIsEmpty) {
  const Option<int> value;
  EXPECT_FALSE(value.HasValue());
  EXPECT_FALSE(value.IsSome());
  EXPECT_FALSE(static_cast<bool>(value));
}

TEST(OptionTest, AndThenRequiresOptionReturnTypeAtCompileTime) {
  static_assert(OptionAndThenInvocable<int, ReturnsOption>);
  static_assert(!OptionAndThenInvocable<int, ReturnsInt>);
}

TEST(OptionTest, ConstructsWithValue) {
  Option<int> value(42);
  ASSERT_TRUE(value.HasValue());
  EXPECT_TRUE(value.IsSome());
  EXPECT_EQ(value.Value(), 42);
}

TEST(OptionTest, SomeHelperConstructsEngagedOption) {
  const auto value = Some(17);
  ASSERT_TRUE(value.HasValue());
  EXPECT_EQ(value.Value(), 17);
}

TEST(OptionTest, AssignNoneResetsValue) {
  Option<std::string> value(std::string("zcore"));
  ASSERT_TRUE(value.HasValue());

  value = None;
  EXPECT_FALSE(value.HasValue());
}

TEST(OptionTest, EmplaceReplacesState) {
  Option<std::string> value;
  auto& ref = value.Emplace("first");
  EXPECT_EQ(ref, "first");
  EXPECT_EQ(value.Value(), "first");

  value.Emplace("second");
  EXPECT_EQ(value.Value(), "second");
}

TEST(OptionTest, ValueOrUsesDefaultForEmpty) {
  const Option<int> empty;
  EXPECT_EQ(empty.ValueOr(7), 7);

  const Option<int> value(3);
  EXPECT_EQ(value.ValueOr(7), 3);
}

TEST(OptionTest, SupportsMoveOnlyTypes) {
  Option<std::unique_ptr<int>> value(std::make_unique<int>(9));
  ASSERT_TRUE(value.HasValue());
  ASSERT_NE(value.Value(), nullptr);
  EXPECT_EQ(*value.Value(), 9);
}

TEST(OptionTest, CopyAssignPreservesValueState) {
  const Option<std::string> left(std::string("left"));
  Option<std::string> right;

  right = left;
  ASSERT_TRUE(right.HasValue());
  EXPECT_EQ(right.Value(), "left");
}

TEST(OptionTest, TryValueExposesPointerWhenPresent) {
  Option<int> value(11);
  ASSERT_NE(value.TryValue(), nullptr);
  EXPECT_EQ(*value.TryValue(), 11);

  value = None;
  EXPECT_EQ(value.TryValue(), nullptr);
}

TEST(OptionTest, MapTransformsContainedValue) {
  const Option<int> source(5);
  auto mapped = source.Map([](const int value) { return value * 3; });

  ASSERT_TRUE(mapped.HasValue());
  EXPECT_EQ(mapped.Value(), 15);
}

TEST(OptionTest, AndThenChainsOptionalComputation) {
  const Option<int> source(8);
  auto chained = source.AndThen([](const int value) {
    return value > 0 ? Option<std::string>(std::to_string(value)) : Option<std::string>(None);
  });

  ASSERT_TRUE(chained.HasValue());
  EXPECT_EQ(chained.Value(), "8");
}

TEST(OptionTest, OrElseProvidesFallbackOption) {
  const Option<int> empty;
  auto recovered = empty.OrElse([] { return Option<int>(99); });
  ASSERT_TRUE(recovered.HasValue());
  EXPECT_EQ(recovered.Value(), 99);
}

TEST(OptionTest, MapOrAndMapOrElseUseFallbackForEmpty) {
  const Option<int> empty;
  EXPECT_EQ(empty.MapOr(17, [](const int v) { return v * 2; }), 17);
  EXPECT_EQ(empty.MapOrElse([] { return 19; }, [](const int v) { return v * 2; }), 19);

  const Option<int> value(4);
  EXPECT_EQ(value.MapOr(17, [](const int v) { return v * 2; }), 8);
  EXPECT_EQ(value.MapOrElse([] { return 19; }, [](const int v) { return v * 2; }), 8);
}

TEST(OptionTest, InspectRunsSideEffectWithoutChangingState) {
  const Option<int> source(6);
  int observed = 0;
  const Option<int>& out = source.Inspect([&observed](const int v) { observed = v; });

  EXPECT_EQ(observed, 6);
  EXPECT_TRUE(out.HasValue());
  EXPECT_EQ(out.Value(), 6);
}

TEST(OptionTest, TakeMovesValueOutAndClearsOption) {
  Option<std::string> source(std::string("moved"));
  Option<std::string> taken = source.Take();

  ASSERT_TRUE(taken.HasValue());
  EXPECT_EQ(taken.Value(), "moved");
  EXPECT_FALSE(source.HasValue());
}

TEST(OptionTest, ReplaceReturnsPreviousValue) {
  Option<std::string> source(std::string("old"));
  Option<std::string> previous = source.Replace(std::string("new"));

  ASSERT_TRUE(previous.HasValue());
  EXPECT_EQ(previous.Value(), "old");
  ASSERT_TRUE(source.HasValue());
  EXPECT_EQ(source.Value(), "new");
}

#if GTEST_HAS_DEATH_TEST
TEST(OptionTest, ValueOnNoneTerminates) {
  EXPECT_DEATH(
      {
        const Option<int> value;
        static_cast<void>(value.Value());
      },
      "");
}
#endif

}  // namespace
