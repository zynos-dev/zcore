/**************************************************************************/
/*  error_result_status_test.cpp                                          */
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
 * @file tests/error_result_status_test.cpp
 * @brief Unit tests for Error, Result, and Status semantics.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/error.hpp>
#include <zcore/option.hpp>
#include <zcore/result.hpp>
#include <zcore/status.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

namespace {

using zcore::Error;
using zcore::ErrorCode;
using zcore::ErrorContext;
using zcore::ErrorDomain;
using zcore::Option;
using zcore::Result;
using zcore::Status;
using IntErrorResult = Result<int, Error>;
using VoidErrorResult = Result<void, Error>;

TEST(ErrorTest, BuildsStructuredError) {
  constexpr ErrorDomain kDomain{.id = 42U, .name = "io"};
  constexpr ErrorContext kContext{
      .subsystem = "io",
      .operation = "read",
      .message = "short read",
      .file = "file.cpp",
      .line = 7U,
  };

  constexpr Error kError = zcore::MakeError(kDomain, 2, kContext);
  static_assert(!kError.IsOk());
  EXPECT_EQ(kError.code.domain.id, 42U);
  EXPECT_STREQ(kError.code.domain.name, "io");
  EXPECT_EQ(kError.code.value, 2);
  EXPECT_STREQ(kError.context.operation, "read");
}

TEST(ErrorDomainTest, SuccessDomainValidityAndClassification) {
  constexpr ErrorDomain kSuccess = ErrorDomain::Success();
  static_assert(kSuccess.IsValid());
  static_assert(kSuccess.IsSuccessDomain());
  static_assert(!kSuccess.IsFailureDomain());
  EXPECT_TRUE(kSuccess.IsValid());
  EXPECT_TRUE(kSuccess.IsSuccessDomain());
  EXPECT_FALSE(kSuccess.IsFailureDomain());
}

TEST(ErrorDomainTest, InvalidSuccessDomainShapeIsRejected) {
  constexpr ErrorDomain kWrongName{.id = ErrorDomain::kSuccessId, .name = "success"};
  constexpr ErrorDomain kNullName{.id = ErrorDomain::kSuccessId, .name = nullptr};
  static_assert(!kWrongName.IsValid());
  static_assert(!kNullName.IsValid());
  EXPECT_FALSE(kWrongName.IsValid());
  EXPECT_FALSE(kNullName.IsValid());
}

TEST(ErrorCodeTest, IsOkRequiresCanonicalSuccessDomain) {
  constexpr ErrorCode kOkCode{
      .domain = ErrorDomain::Success(),
      .value = 0,
  };
  constexpr ErrorCode kZeroInFailureDomain{
      .domain = ErrorDomain{.id = 9U, .name = "net"},
      .value = 0,
  };
  constexpr ErrorCode kNonZeroInSuccessDomain{
      .domain = ErrorDomain::Success(),
      .value = 7,
  };

  static_assert(kOkCode.IsOk());
  static_assert(!kZeroInFailureDomain.IsOk());
  static_assert(!kNonZeroInSuccessDomain.IsOk());
  EXPECT_TRUE(kOkCode.IsOk());
  EXPECT_FALSE(kZeroInFailureDomain.IsOk());
  EXPECT_FALSE(kNonZeroInSuccessDomain.IsOk());
}

TEST(ResultTest, SuccessContainsValue) {
  auto result = Result<int, Error>::Success(7);
  ASSERT_TRUE(result.HasValue());
  EXPECT_FALSE(result.HasError());
  EXPECT_EQ(result.Value(), 7);
  EXPECT_EQ(result.TryError(), nullptr);
}

TEST(ResultTest, FailureContainsError) {
  const Error error = zcore::MakeError(
      zcore::kZcoreErrorDomain,
      11,
      zcore::MakeErrorContext("parse", "parse_int", "invalid input", __FILE__, static_cast<uint32_t>(__LINE__)));

  auto result = Result<int, Error>::Failure(error);
  ASSERT_TRUE(result.HasError());
  EXPECT_FALSE(result.HasValue());
  EXPECT_EQ(result.TryValue(), nullptr);
  EXPECT_EQ(result.Error().code.value, 11);
  EXPECT_STREQ(result.Error().context.operation, "parse_int");
}

TEST(ResultVoidTest, SuccessWithoutPayload) {
  auto result = Result<void, Error>::Success();
  EXPECT_TRUE(result.HasValue());
  EXPECT_FALSE(result.HasError());
}

TEST(StatusTest, OkAndErrorHelpers) {
  const Status ok = zcore::OkStatus();
  EXPECT_TRUE(ok.HasValue());

  const Error error = zcore::MakeError(
      ErrorDomain{.id = 9U, .name = "net"},
      55,
      ErrorContext{
          .subsystem = "net",
          .operation = "connect",
          .message = "timeout",
          .file = __FILE__,
          .line = static_cast<uint32_t>(__LINE__),
      });
  Status failed = zcore::ErrorStatus(error);
  ASSERT_TRUE(failed.HasError());
  EXPECT_EQ(failed.Error().code.value, 55);
  EXPECT_STREQ(failed.Error().context.subsystem, "net");
}

TEST(ResultTest, MoveOnlyValueSupport) {
  auto result = Result<std::unique_ptr<int>, Error>::Success(std::make_unique<int>(3));
  ASSERT_TRUE(result.HasValue());
  ASSERT_NE(result.Value(), nullptr);
  EXPECT_EQ(*result.Value(), 3);
}

TEST(ResultTest, CopyAssignmentSwitchesState) {
  auto first = Result<std::string, Error>::Success(std::string("alpha"));
  auto second = Result<std::string, Error>::Failure(
      zcore::MakeError(zcore::kZcoreErrorDomain, 4, zcore::MakeErrorContext("core", "x", "y", __FILE__, 1U)));

  second = first;
  ASSERT_TRUE(second.HasValue());
  EXPECT_EQ(second.Value(), "alpha");
}

TEST(ResultTest, MapTransformsSuccessValue) {
  auto source = Result<int, Error>::Success(4);
  auto mapped = source.Map([](const int value) { return value * 2; });
  ASSERT_TRUE(mapped.HasValue());
  EXPECT_EQ(mapped.Value(), 8);
}

TEST(ResultTest, MapErrorTransformsFailure) {
  auto source = Result<int, Error>::Failure(
      zcore::MakeError(zcore::kZcoreErrorDomain, 9, zcore::MakeErrorContext("cfg", "load", "bad", __FILE__, 1U)));

  auto mapped = source.MapError([](const Error& err) {
    return std::string(err.context.operation);
  });

  ASSERT_TRUE(mapped.HasError());
  EXPECT_EQ(mapped.Error(), "load");
}

TEST(ResultTest, AndThenChainsSuccessPath) {
  auto source = Result<int, Error>::Success(10);
  auto chained = source.AndThen([](const int value) {
    return Result<std::string, Error>::Success(std::to_string(value));
  });

  ASSERT_TRUE(chained.HasValue());
  EXPECT_EQ(chained.Value(), "10");
}

TEST(ResultTest, OrElseRecoversFromFailure) {
  auto source = Result<int, Error>::Failure(
      zcore::MakeError(zcore::kZcoreErrorDomain, 21, zcore::MakeErrorContext("io", "read", "eof", __FILE__, 1U)));
  auto recovered = source.OrElse([](const Error&) {
    return Result<int, Error>::Success(99);
  });

  ASSERT_TRUE(recovered.HasValue());
  EXPECT_EQ(recovered.Value(), 99);
}

TEST(ResultVoidTest, CombinatorsWorkForVoidValueType) {
  auto ok = Result<void, Error>::Success();
  auto mapped = ok.Map([] { return 3; });
  ASSERT_TRUE(mapped.HasValue());
  EXPECT_EQ(mapped.Value(), 3);

  auto chained = ok.AndThen([] { return Result<int, Error>::Success(12); });
  ASSERT_TRUE(chained.HasValue());
  EXPECT_EQ(chained.Value(), 12);

  auto failed = Result<void, Error>::Failure(
      zcore::MakeError(zcore::kZcoreErrorDomain, 1, zcore::MakeErrorContext("net", "connect", "timeout", __FILE__, 1U)));
  auto recovered = failed.OrElse([](const Error&) { return Result<void, Error>::Success(); });
  EXPECT_TRUE(recovered.HasValue());
}

TEST(ResultTest, MapSupportsVoidCallbackReturn) {
  auto source = Result<int, Error>::Success(4);
  int observed = 0;

  auto mapped = source.Map([&observed](const int value) {
    observed = value;
  });

  EXPECT_EQ(observed, 4);
  EXPECT_TRUE(mapped.HasValue());
}

TEST(ResultVoidTest, MapSupportsVoidCallbackReturn) {
  auto source = Result<void, Error>::Success();
  int calls = 0;

  auto mapped = source.Map([&calls] { ++calls; });

  EXPECT_EQ(calls, 1);
  EXPECT_TRUE(mapped.HasValue());
}

TEST(ResultTest, OrElseAllowsVoidResultTarget) {
  auto source = Result<int, Error>::Success(7);
  auto out = source.OrElse([](const Error&) { return Result<void, Error>::Success(); });
  EXPECT_TRUE(out.HasValue());
}

TEST(ResultTest, OkAndErrProjectBranchesToOption) {
  auto success = Result<int, Error>::Success(12);
  auto ok = success.Ok();
  ASSERT_TRUE(ok.HasValue());
  EXPECT_EQ(ok.Value(), 12);
  EXPECT_FALSE(success.Err().HasValue());

  auto failure = Result<int, Error>::Failure(
      zcore::MakeError(zcore::kZcoreErrorDomain, 41, zcore::MakeErrorContext("cfg", "load", "bad", __FILE__, 1U)));
  auto err = failure.Err();
  ASSERT_TRUE(err.HasValue());
  EXPECT_EQ(err.Value().code.value, 41);
  EXPECT_FALSE(failure.Ok().HasValue());
}

TEST(ResultTest, MapOrAndMapOrElseEvaluateExpectedBranch) {
  auto success = Result<int, Error>::Success(6);
  EXPECT_EQ(success.MapOr(10, [](const int value) { return value * 2; }), 12);
  EXPECT_EQ(success.MapOrElse(
                [](const Error&) { return 15; },
                [](const int value) { return value * 3; }),
            18);

  auto failure = Result<int, Error>::Failure(
      zcore::MakeError(zcore::kZcoreErrorDomain, 3, zcore::MakeErrorContext("io", "read", "eof", __FILE__, 1U)));
  EXPECT_EQ(failure.MapOr(10, [](const int value) { return value * 2; }), 10);
  EXPECT_EQ(failure.MapOrElse(
                [](const Error& err) { return err.code.value; },
                [](const int value) { return value * 3; }),
            3);
}

TEST(ResultTest, UnwrapOrAndUnwrapOrElseReturnFallbackOnFailure) {
  auto success = Result<int, Error>::Success(5);
  EXPECT_EQ(success.UnwrapOr(99), 5);
  EXPECT_EQ(success.UnwrapOrElse([](const Error&) { return 77; }), 5);

  auto failure = Result<int, Error>::Failure(
      zcore::MakeError(zcore::kZcoreErrorDomain, 8, zcore::MakeErrorContext("net", "connect", "down", __FILE__, 1U)));
  EXPECT_EQ(failure.UnwrapOr(99), 99);
  EXPECT_EQ(failure.UnwrapOrElse([](const Error& err) { return err.code.value * 10; }), 80);
}

TEST(ResultTest, InspectAndInspectErrorObserveStateWithoutMutation) {
  auto success = Result<int, Error>::Success(22);
  int observedValue = 0;
  int observedError = 0;

  const auto& inspectedSuccess = success
                                     .Inspect([&observedValue](const int value) { observedValue = value; })
                                     .InspectError([&observedError](const Error&) { observedError = -1; });

  EXPECT_EQ(observedValue, 22);
  EXPECT_EQ(observedError, 0);
  EXPECT_TRUE(inspectedSuccess.HasValue());

  auto failure = Result<int, Error>::Failure(
      zcore::MakeError(zcore::kZcoreErrorDomain, 13, zcore::MakeErrorContext("cfg", "open", "missing", __FILE__, 1U)));
  failure.Inspect([&observedValue](const int) { observedValue = -2; })
      .InspectError([&observedError](const Error& err) { observedError = err.code.value; });

  EXPECT_EQ(observedValue, 22);
  EXPECT_EQ(observedError, 13);
}

TEST(ResultTest, TransposeConvertsResultOptionToOptionResult) {
  const Result<Option<int>, Error> successWithValue = Result<Option<int>, Error>::Success(Option<int>(3));
  auto transposedValue = successWithValue.Transpose();
  ASSERT_TRUE(transposedValue.HasValue());
  ASSERT_TRUE(transposedValue.Value().HasValue());
  EXPECT_EQ(transposedValue.Value().Value(), 3);

  const Result<Option<int>, Error> successWithoutValue = Result<Option<int>, Error>::Success(Option<int>(zcore::None));
  auto transposedNone = successWithoutValue.Transpose();
  EXPECT_FALSE(transposedNone.HasValue());

  const Result<Option<int>, Error> failure = Result<Option<int>, Error>::Failure(
      zcore::MakeError(zcore::kZcoreErrorDomain, 51, zcore::MakeErrorContext("io", "read", "bad", __FILE__, 1U)));
  auto transposedErr = failure.Transpose();
  ASSERT_TRUE(transposedErr.HasValue());
  ASSERT_TRUE(transposedErr.Value().HasError());
  EXPECT_EQ(transposedErr.Value().Error().code.value, 51);
}

#if GTEST_HAS_DEATH_TEST
TEST(ResultContractTest, ValueOnFailureTerminates) {
  EXPECT_DEATH(
      {
        auto failure = IntErrorResult::Failure(
            zcore::MakeError(zcore::kZcoreErrorDomain, 2, zcore::MakeErrorContext("cfg", "read", "bad", __FILE__, 1U)));
        static_cast<void>(failure.Value());
      },
      "");
}

TEST(ResultContractTest, ValueOnFailureConstAndRvalueOverloadsTerminate) {
  EXPECT_DEATH(
      {
        const auto failure = IntErrorResult::Failure(
            zcore::MakeError(zcore::kZcoreErrorDomain, 2, zcore::MakeErrorContext("cfg", "read", "bad", __FILE__, 1U)));
        static_cast<void>(failure.Value());
      },
      "");

  EXPECT_DEATH(
      {
        auto failure = IntErrorResult::Failure(
            zcore::MakeError(zcore::kZcoreErrorDomain, 2, zcore::MakeErrorContext("cfg", "read", "bad", __FILE__, 1U)));
        static_cast<void>(std::move(failure).Value());
      },
      "");
}

TEST(ResultContractTest, ErrorOnSuccessTerminates) {
  EXPECT_DEATH(
      {
        auto success = IntErrorResult::Success(9);
        static_cast<void>(success.Error());
      },
      "");
}

TEST(ResultContractTest, ErrorOnSuccessConstAndRvalueOverloadsTerminate) {
  EXPECT_DEATH(
      {
        const auto success = IntErrorResult::Success(9);
        static_cast<void>(success.Error());
      },
      "");

  EXPECT_DEATH(
      {
        auto success = IntErrorResult::Success(9);
        static_cast<void>(std::move(success).Error());
      },
      "");
}

TEST(ResultContractTest, VoidErrorOnSuccessTerminates) {
  EXPECT_DEATH(
      {
        auto success = VoidErrorResult::Success();
        static_cast<void>(success.Error());
      },
      "");
}

TEST(ResultContractTest, VoidErrorOnSuccessConstAndRvalueOverloadsTerminate) {
  EXPECT_DEATH(
      {
        const auto success = VoidErrorResult::Success();
        static_cast<void>(success.Error());
      },
      "");

  EXPECT_DEATH(
      {
        auto success = VoidErrorResult::Success();
        static_cast<void>(std::move(success).Error());
      },
      "");
}
#endif

}  // namespace
