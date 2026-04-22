/**************************************************************************/
/*  result.hpp                                                            */
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
 * @file include/zcore/error/result.hpp
 * @brief Fallible return type representing success or failure.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/result.hpp>
 * zcore::Result<int, zcore::Error> value =
 *     zcore::Result<int, zcore::Error>::Success(42);
 * @endcode
 */

#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <zcore/contract_violation.hpp>
#include <zcore/option.hpp>
#include <zcore/type_constraints.hpp>

namespace zcore {

template <typename ValueT, typename ErrorT>
class Result;

namespace detail {
template <typename ResultT>
struct IsResult final : std::false_type {};

template <typename ValueT, typename ErrorT>
struct IsResult<Result<ValueT, ErrorT>> final : std::true_type {};

template <typename ResultT>
inline constexpr bool kIsResultV = IsResult<ResultT>::value;

template <typename OptionT>
struct IsOption final : std::false_type {};

template <typename ValueT>
struct IsOption<Option<ValueT>> final : std::true_type {};

template <typename OptionT>
inline constexpr bool kIsOptionV = IsOption<OptionT>::value;

template <typename OptionT>
struct OptionValueType;

template <typename ValueT>
struct OptionValueType<Option<ValueT>> final {
    using Type = ValueT;
};

template <typename OptionT>
using OptionValueTypeT = typename OptionValueType<OptionT>::Type;
} // namespace detail

/**
 * @brief Fallible value type containing either `ValueT` or `ErrorT`.
 *
 * @tparam ValueT Success payload type.
 * @tparam ErrorT Error payload type.
 */
template <typename ValueT, typename ErrorT>
class [[nodiscard("Result must be handled explicitly.")]] Result final {
public:
    ZCORE_STATIC_REQUIRE(constraints::NonReferenceNonVoidType<ValueT>,
                         "Result<ValueT, ErrorT> requires non-reference non-void success value type.");
    ZCORE_STATIC_REQUIRE(constraints::NonReferenceNonVoidType<ErrorT>,
                         "Result<ValueT, ErrorT> requires non-reference non-void error type.");
    ZCORE_STATIC_REQUIRE(!std::is_same_v<ValueT, ErrorT>,
                         "Result<ValueT, ErrorT> requires distinct value and error types to keep construction explicit.");

    using ValueType = ValueT;
    using ErrorType = ErrorT;

    /// @brief Constructs a success result from copied value.
    [[nodiscard]] static constexpr Result Success(const ValueT& value)
    {
        return Result(value);
    }

    /// @brief Constructs a success result from moved value.
    [[nodiscard]] static constexpr Result Success(ValueT&& value)
    {
        return Result(std::move(value));
    }

    /// @brief Constructs a failure result from copied error.
    [[nodiscard]] static constexpr Result Failure(const ErrorT& error)
    {
        return Result(error, FailureTag{});
    }

    /// @brief Constructs a failure result from moved error.
    [[nodiscard]] static constexpr Result Failure(ErrorT&& error)
    {
        return Result(std::move(error), FailureTag{});
    }

    constexpr Result(const Result& other)
        requires(std::is_copy_constructible_v<ValueT> && std::is_copy_constructible_v<ErrorT>)
            : HasValue_(other.HasValue_)
    {
        if (HasValue_) {
            std::construct_at(ValuePtr(), *other.ValuePtr());
        }
        else {
            std::construct_at(ErrorPtr(), *other.ErrorPtr());
        }
    }

    constexpr Result(Result&& other) noexcept(std::is_nothrow_move_constructible_v<ValueT>
                                              && std::is_nothrow_move_constructible_v<ErrorT>)
        requires(std::is_move_constructible_v<ValueT> && std::is_move_constructible_v<ErrorT>)
            : HasValue_(other.HasValue_)
    {
        if (HasValue_) {
            std::construct_at(ValuePtr(), std::move(*other.ValuePtr()));
        }
        else {
            std::construct_at(ErrorPtr(), std::move(*other.ErrorPtr()));
        }
    }

    constexpr Result& operator=(const Result& other)
        requires(std::is_copy_constructible_v<ValueT> && std::is_copy_constructible_v<ErrorT> && std::is_copy_assignable_v<ValueT>
                 && std::is_copy_assignable_v<ErrorT>)
    {
        if (this == &other) {
            return *this;
        }

        if (HasValue_ && other.HasValue_) {
            *ValuePtr() = *other.ValuePtr();
            return *this;
        }

        if (!HasValue_ && !other.HasValue_) {
            *ErrorPtr() = *other.ErrorPtr();
            return *this;
        }

        DestroyActive();
        HasValue_ = other.HasValue_;
        if (HasValue_) {
            std::construct_at(ValuePtr(), *other.ValuePtr());
        }
        else {
            std::construct_at(ErrorPtr(), *other.ErrorPtr());
        }

        return *this;
    }

    constexpr Result&
    operator=(Result&& other) noexcept(std::is_nothrow_move_constructible_v<ValueT> && std::is_nothrow_move_constructible_v<ErrorT>
                                       && std::is_nothrow_move_assignable_v<ValueT> && std::is_nothrow_move_assignable_v<ErrorT>)
        requires(std::is_move_constructible_v<ValueT> && std::is_move_constructible_v<ErrorT> && std::is_move_assignable_v<ValueT>
                 && std::is_move_assignable_v<ErrorT>)
    {
        if (this == &other) {
            return *this;
        }

        if (HasValue_ && other.HasValue_) {
            *ValuePtr() = std::move(*other.ValuePtr());
            return *this;
        }

        if (!HasValue_ && !other.HasValue_) {
            *ErrorPtr() = std::move(*other.ErrorPtr());
            return *this;
        }

        DestroyActive();
        HasValue_ = other.HasValue_;
        if (HasValue_) {
            std::construct_at(ValuePtr(), std::move(*other.ValuePtr()));
        }
        else {
            std::construct_at(ErrorPtr(), std::move(*other.ErrorPtr()));
        }

        return *this;
    }

    constexpr ~Result()
    {
        DestroyActive();
    }

    /// @brief Returns `true` when result is success.
    [[nodiscard]] constexpr bool HasValue() const noexcept
    {
        return HasValue_;
    }

    /// @brief Alias for `HasValue()`.
    [[nodiscard]] constexpr bool IsOk() const noexcept
    {
        return HasValue_;
    }

    /// @brief Returns `true` when result is failure.
    [[nodiscard]] constexpr bool HasError() const noexcept
    {
        return !HasValue_;
    }

    /// @brief Alias for `HasError()`.
    [[nodiscard]] constexpr bool IsErr() const noexcept
    {
        return !HasValue_;
    }

    /// @brief Boolean conversion; equivalent to `HasValue()`.
    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return HasValue_;
    }

    /**
   * @brief Returns success value.
   * @pre `HasValue()` must be true.
   */
    [[nodiscard]] constexpr ValueT& Value() &
    {
        ZCORE_CONTRACT_REQUIRE(HasValue_,
                               detail::ContractViolationCode::RESULT_EXPECTED_VALUE,
                               "zcore::Result::Value() requires a success value");
        return *ValuePtr();
    }

    /**
   * @brief Returns success value.
   * @pre `HasValue()` must be true.
   */
    [[nodiscard]] constexpr const ValueT& Value() const&
    {
        ZCORE_CONTRACT_REQUIRE(HasValue_,
                               detail::ContractViolationCode::RESULT_EXPECTED_VALUE,
                               "zcore::Result::Value() requires a success value");
        return *ValuePtr();
    }

    /**
   * @brief Moves and returns success value.
   * @pre `HasValue()` must be true.
   */
    [[nodiscard]] constexpr ValueT&& Value() &&
    {
        ZCORE_CONTRACT_REQUIRE(HasValue_,
                               detail::ContractViolationCode::RESULT_EXPECTED_VALUE,
                               "zcore::Result::Value() requires a success value");
        return std::move(*ValuePtr());
    }

    /**
   * @brief Moves and returns success value (const rvalue overload).
   * @pre `HasValue()` must be true.
   */
    [[nodiscard]] constexpr const ValueT&& Value() const&&
    {
        ZCORE_CONTRACT_REQUIRE(HasValue_,
                               detail::ContractViolationCode::RESULT_EXPECTED_VALUE,
                               "zcore::Result::Value() requires a success value");
        return std::move(*ValuePtr());
    }

    /**
   * @brief Returns error value.
   * @pre `HasError()` must be true.
   */
    [[nodiscard]] constexpr ErrorT& Error() &
    {
        ZCORE_CONTRACT_REQUIRE(!HasValue_,
                               detail::ContractViolationCode::RESULT_EXPECTED_ERROR,
                               "zcore::Result::Error() requires an error value");
        return *ErrorPtr();
    }

    /**
   * @brief Returns error value.
   * @pre `HasError()` must be true.
   */
    [[nodiscard]] constexpr const ErrorT& Error() const&
    {
        ZCORE_CONTRACT_REQUIRE(!HasValue_,
                               detail::ContractViolationCode::RESULT_EXPECTED_ERROR,
                               "zcore::Result::Error() requires an error value");
        return *ErrorPtr();
    }

    /**
   * @brief Moves and returns error value.
   * @pre `HasError()` must be true.
   */
    [[nodiscard]] constexpr ErrorT&& Error() &&
    {
        ZCORE_CONTRACT_REQUIRE(!HasValue_,
                               detail::ContractViolationCode::RESULT_EXPECTED_ERROR,
                               "zcore::Result::Error() requires an error value");
        return std::move(*ErrorPtr());
    }

    /**
   * @brief Moves and returns error value (const rvalue overload).
   * @pre `HasError()` must be true.
   */
    [[nodiscard]] constexpr const ErrorT&& Error() const&&
    {
        ZCORE_CONTRACT_REQUIRE(!HasValue_,
                               detail::ContractViolationCode::RESULT_EXPECTED_ERROR,
                               "zcore::Result::Error() requires an error value");
        return std::move(*ErrorPtr());
    }

    /// @brief Returns pointer to success value or `nullptr`.
    [[nodiscard]] constexpr ValueT* TryValue() noexcept
    {
        return HasValue_ ? ValuePtr() : nullptr;
    }

    /// @brief Returns const pointer to success value or `nullptr`.
    [[nodiscard]] constexpr const ValueT* TryValue() const noexcept
    {
        return HasValue_ ? ValuePtr() : nullptr;
    }

    /// @brief Returns pointer to error value or `nullptr`.
    [[nodiscard]] constexpr ErrorT* TryError() noexcept
    {
        return HasValue_ ? nullptr : ErrorPtr();
    }

    /// @brief Returns const pointer to error value or `nullptr`.
    [[nodiscard]] constexpr const ErrorT* TryError() const noexcept
    {
        return HasValue_ ? nullptr : ErrorPtr();
    }

    /// @brief Projects success branch into `Option<ValueT>`.
    [[nodiscard]] constexpr Option<ValueT> Ok() const&
        requires(std::is_copy_constructible_v<ValueT>)
    {
        if (HasValue_) {
            return Option<ValueT>(*ValuePtr());
        }
        return None;
    }

    /// @brief Projects success branch into `Option<ValueT>` by move.
    [[nodiscard]] constexpr Option<ValueT> Ok() &&
        requires(std::is_move_constructible_v<ValueT>)
    {
        if (HasValue_) {
            return Option<ValueT>(std::move(*ValuePtr()));
        }
        return None;
    }

    /// @brief Projects error branch into `Option<ErrorT>`.
    [[nodiscard]] constexpr Option<ErrorT> Err() const&
        requires(std::is_copy_constructible_v<ErrorT>)
    {
        if (HasValue_) {
            return None;
        }
        return Option<ErrorT>(*ErrorPtr());
    }

    /// @brief Projects error branch into `Option<ErrorT>` by move.
    [[nodiscard]] constexpr Option<ErrorT> Err() &&
        requires(std::is_move_constructible_v<ErrorT>)
    {
        if (HasValue_) {
            return None;
        }
        return Option<ErrorT>(std::move(*ErrorPtr()));
    }

    /**
   * @brief Maps success value while preserving error branch.
   * @param fn Mapping callback.
   * @return `Result<U, ErrorT>`.
   */
    template <typename FuncT>
        requires(std::is_copy_constructible_v<ErrorT> && std::is_invocable_v<FuncT, const ValueT&>)
    [[nodiscard]] constexpr auto Map(FuncT&& fn) const& -> Result<std::invoke_result_t<FuncT, const ValueT&>, ErrorT>
    {
        using NextValueT = std::invoke_result_t<FuncT, const ValueT&>;
        if (HasValue_) {
            if constexpr (std::is_void_v<NextValueT>) {
                std::forward<FuncT>(fn)(*ValuePtr());
                return Result<NextValueT, ErrorT>::Success();
            }
            else {
                return Result<NextValueT, ErrorT>::Success(std::forward<FuncT>(fn)(*ValuePtr()));
            }
        }
        return Result<NextValueT, ErrorT>::Failure(*ErrorPtr());
    }

    /**
   * @brief Maps success with eager fallback value.
   * @param defaultValue Fallback mapped value for error branch.
   * @param fn Mapping callback.
   */
    template <typename DefaultT, typename FuncT>
        requires(std::is_invocable_v<FuncT, const ValueT&>
                 && std::is_convertible_v<DefaultT &&, std::invoke_result_t<FuncT, const ValueT&>>)
    [[nodiscard]] constexpr auto MapOr(DefaultT&& defaultValue, FuncT&& fn) const& -> std::invoke_result_t<FuncT, const ValueT&>
    {
        using MappedT = std::invoke_result_t<FuncT, const ValueT&>;
        static_assert(!std::is_void_v<MappedT>, "Result::MapOr callback must return a value type.");
        if (HasValue_) {
            return std::forward<FuncT>(fn)(*ValuePtr());
        }
        return static_cast<MappedT>(std::forward<DefaultT>(defaultValue));
    }

    /**
   * @brief Maps success with lazy fallback callback.
   * @param defaultFn Fallback mapper for error branch.
   * @param fn Mapping callback for success branch.
   */
    template <typename DefaultFuncT, typename FuncT>
        requires(std::is_invocable_v<DefaultFuncT, const ErrorT&> && std::is_invocable_v<FuncT, const ValueT&>
                 && std::is_convertible_v<std::invoke_result_t<DefaultFuncT, const ErrorT&>,
                                          std::invoke_result_t<FuncT, const ValueT&>>)
    [[nodiscard]] constexpr auto MapOrElse(DefaultFuncT&& defaultFn,
                                           FuncT&& fn) const& -> std::invoke_result_t<FuncT, const ValueT&>
    {
        using MappedT = std::invoke_result_t<FuncT, const ValueT&>;
        static_assert(!std::is_void_v<MappedT>, "Result::MapOrElse callback must return a value type.");
        if (HasValue_) {
            return std::forward<FuncT>(fn)(*ValuePtr());
        }
        return static_cast<MappedT>(std::forward<DefaultFuncT>(defaultFn)(*ErrorPtr()));
    }

    /**
   * @brief Maps error value while preserving success branch.
   * @param fn Error mapping callback.
   */
    template <typename FuncT>
        requires(std::is_copy_constructible_v<ValueT> && std::is_invocable_v<FuncT, const ErrorT&>)
    [[nodiscard]] constexpr auto MapError(FuncT&& fn) const& -> Result<ValueT, std::invoke_result_t<FuncT, const ErrorT&>>
    {
        using NextErrorT = std::invoke_result_t<FuncT, const ErrorT&>;
        static_assert(!std::is_void_v<NextErrorT>, "Result::MapError callback must return a value type.");
        if (HasValue_) {
            return Result<ValueT, NextErrorT>::Success(*ValuePtr());
        }
        return Result<ValueT, NextErrorT>::Failure(std::forward<FuncT>(fn)(*ErrorPtr()));
    }

    /**
   * @brief Chains another result-producing operation on success.
   * @param fn Callback returning another `Result`.
   */
    template <typename FuncT>
        requires(std::is_invocable_v<FuncT, const ValueT&>)
    [[nodiscard]] constexpr auto AndThen(FuncT&& fn) const& -> std::invoke_result_t<FuncT, const ValueT&>
    {
        using NextResultT = std::invoke_result_t<FuncT, const ValueT&>;
        static_assert(detail::kIsResultV<NextResultT>, "AndThen callback must return zcore::Result<U, E>.");
        using NextErrorT = typename NextResultT::ErrorType;
        static_assert(std::is_convertible_v<const ErrorT&, NextErrorT>,
                      "AndThen requires current error type to be convertible to next result error type.");

        if (HasValue_) {
            return std::forward<FuncT>(fn)(*ValuePtr());
        }
        return NextResultT::Failure(static_cast<NextErrorT>(*ErrorPtr()));
    }

    /**
   * @brief Recovers from error with another result-producing callback.
   * @param fn Callback returning another `Result`.
   */
    template <typename FuncT>
        requires(std::is_invocable_v<FuncT, const ErrorT&>)
    [[nodiscard]] constexpr auto OrElse(FuncT&& fn) const& -> std::invoke_result_t<FuncT, const ErrorT&>
    {
        using NextResultT = std::invoke_result_t<FuncT, const ErrorT&>;
        static_assert(detail::kIsResultV<NextResultT>, "OrElse callback must return zcore::Result<T, F>.");
        using NextValueT = typename NextResultT::ValueType;
        if constexpr (!std::is_void_v<NextValueT>) {
            static_assert(std::is_convertible_v<const ValueT&, NextValueT>,
                          "OrElse requires current value type to be convertible to next result value type.");
        }

        if (HasValue_) {
            if constexpr (std::is_void_v<NextValueT>) {
                return NextResultT::Success();
            }
            else {
                return NextResultT::Success(static_cast<NextValueT>(*ValuePtr()));
            }
        }
        return std::forward<FuncT>(fn)(*ErrorPtr());
    }

    /**
   * @brief Returns success value or eager fallback.
   * @param defaultValue Fallback used for error branch.
   */
    template <typename DefaultValueT>
        requires(std::is_convertible_v<DefaultValueT &&, ValueT>)
    [[nodiscard]] constexpr ValueT UnwrapOr(DefaultValueT&& defaultValue) const&
    {
        if (HasValue_) {
            return *ValuePtr();
        }
        return static_cast<ValueT>(std::forward<DefaultValueT>(defaultValue));
    }

    /**
   * @brief Moves success value or returns eager fallback.
   * @param defaultValue Fallback used for error branch.
   */
    template <typename DefaultValueT>
        requires(std::is_convertible_v<DefaultValueT &&, ValueT>)
    [[nodiscard]] constexpr ValueT UnwrapOr(DefaultValueT&& defaultValue) &&
    {
        if (HasValue_) {
            return std::move(*ValuePtr());
        }
        return static_cast<ValueT>(std::forward<DefaultValueT>(defaultValue));
    }

    /**
   * @brief Returns success value or computes fallback from error.
   * @param fn Fallback mapper from `ErrorT` to `ValueT`.
   */
    template <typename FuncT>
        requires(std::is_invocable_v<FuncT, const ErrorT&>
                 && std::is_convertible_v<std::invoke_result_t<FuncT, const ErrorT&>, ValueT>)
    [[nodiscard]] constexpr ValueT UnwrapOrElse(FuncT&& fn) const&
    {
        if (HasValue_) {
            return *ValuePtr();
        }
        return static_cast<ValueT>(std::forward<FuncT>(fn)(*ErrorPtr()));
    }

    /**
   * @brief Moves success value or computes fallback from error.
   * @param fn Fallback mapper from `ErrorT` to `ValueT`.
   */
    template <typename FuncT>
        requires(std::is_invocable_v<FuncT, const ErrorT&>
                 && std::is_convertible_v<std::invoke_result_t<FuncT, const ErrorT&>, ValueT>)
    [[nodiscard]] constexpr ValueT UnwrapOrElse(FuncT&& fn) &&
    {
        if (HasValue_) {
            return std::move(*ValuePtr());
        }
        return static_cast<ValueT>(std::forward<FuncT>(fn)(*ErrorPtr()));
    }

    /**
   * @brief Observes success value without mutating result.
   * @param fn Observer callback.
   * @return `*this`.
   */
    template <typename FuncT>
        requires(std::is_invocable_v<FuncT, const ValueT&>)
    constexpr const Result& Inspect(FuncT&& fn) const&
    {
        if (HasValue_) {
            std::forward<FuncT>(fn)(*ValuePtr());
        }
        return *this;
    }

    /**
   * @brief Observes error value without mutating result.
   * @param fn Observer callback.
   * @return `*this`.
   */
    template <typename FuncT>
        requires(std::is_invocable_v<FuncT, const ErrorT&>)
    constexpr const Result& InspectError(FuncT&& fn) const&
    {
        if (!HasValue_) {
            std::forward<FuncT>(fn)(*ErrorPtr());
        }
        return *this;
    }

    /**
   * @brief Transposes `Result<Option<T>, E>` to `Option<Result<T, E>>`.
   *
   * If success contains `None`, returns `None`.
   */
    template <typename OptionT = ValueT>
        requires(detail::kIsOptionV<OptionT> && std::is_copy_constructible_v<detail::OptionValueTypeT<OptionT>>
                 && std::is_copy_constructible_v<ErrorT>)
    [[nodiscard]] constexpr auto Transpose() const& -> Option<Result<detail::OptionValueTypeT<OptionT>, ErrorT>>
    {
        using InnerValueT = detail::OptionValueTypeT<OptionT>;
        if (HasValue_) {
            const OptionT& maybeValue = *ValuePtr();
            if (maybeValue.HasValue()) {
                return Option<Result<InnerValueT, ErrorT>>(Result<InnerValueT, ErrorT>::Success(maybeValue.Value()));
            }
            return None;
        }
        return Option<Result<InnerValueT, ErrorT>>(Result<InnerValueT, ErrorT>::Failure(*ErrorPtr()));
    }

private:
    struct FailureTag final {};

    explicit constexpr Result(const ValueT& value) : HasValue_(true)
    {
        std::construct_at(ValuePtr(), value);
    }

    explicit constexpr Result(ValueT&& value) : HasValue_(true)
    {
        std::construct_at(ValuePtr(), std::move(value));
    }

    explicit constexpr Result(const ErrorT& error, FailureTag) : HasValue_(false)
    {
        std::construct_at(ErrorPtr(), error);
    }

    explicit constexpr Result(ErrorT&& error, FailureTag) : HasValue_(false)
    {
        std::construct_at(ErrorPtr(), std::move(error));
    }

    [[nodiscard]] constexpr ValueT* ValuePtr() noexcept
    {
        return std::launder(reinterpret_cast<ValueT*>(&Storage_));
    }

    [[nodiscard]] constexpr const ValueT* ValuePtr() const noexcept
    {
        return std::launder(reinterpret_cast<const ValueT*>(&Storage_));
    }

    [[nodiscard]] constexpr ErrorT* ErrorPtr() noexcept
    {
        return std::launder(reinterpret_cast<ErrorT*>(&Storage_));
    }

    [[nodiscard]] constexpr const ErrorT* ErrorPtr() const noexcept
    {
        return std::launder(reinterpret_cast<const ErrorT*>(&Storage_));
    }

    constexpr void DestroyActive() noexcept
    {
        if (HasValue_) {
            std::destroy_at(ValuePtr());
        }
        else {
            std::destroy_at(ErrorPtr());
        }
    }

    static constexpr std::size_t kStorageAlign = alignof(ValueT) > alignof(ErrorT) ? alignof(ValueT) : alignof(ErrorT);
    static constexpr std::size_t kStorageSize = sizeof(ValueT) > sizeof(ErrorT) ? sizeof(ValueT) : sizeof(ErrorT);

    alignas(kStorageAlign) std::byte Storage_[kStorageSize];
    bool HasValue_;
};

/**
 * @brief `Result` specialization for success-without-value operations.
 *
 * @tparam ErrorT Error payload type.
 */
template <typename ErrorT>
class [[nodiscard("Result must be handled explicitly.")]] Result<void, ErrorT> final {
public:
    ZCORE_STATIC_REQUIRE(constraints::NonReferenceNonVoidType<ErrorT>,
                         "Result<void, ErrorT> requires non-reference non-void error type.");

    using ValueType = void;
    using ErrorType = ErrorT;

    /// @brief Constructs success result.
    [[nodiscard]] static constexpr Result Success() noexcept
    {
        return Result();
    }

    /// @brief Constructs failure result from copied error.
    [[nodiscard]] static constexpr Result Failure(const ErrorT& error)
    {
        return Result(error);
    }

    /// @brief Constructs failure result from moved error.
    [[nodiscard]] static constexpr Result Failure(ErrorT&& error)
    {
        return Result(std::move(error));
    }

    constexpr Result(const Result& other)
        requires(std::is_copy_constructible_v<ErrorT>)
            : HasValue_(other.HasValue_)
    {
        if (!HasValue_) {
            std::construct_at(ErrorPtr(), *other.ErrorPtr());
        }
    }

    constexpr Result(Result&& other) noexcept(std::is_nothrow_move_constructible_v<ErrorT>)
        requires(std::is_move_constructible_v<ErrorT>)
            : HasValue_(other.HasValue_)
    {
        if (!HasValue_) {
            std::construct_at(ErrorPtr(), std::move(*other.ErrorPtr()));
        }
    }

    constexpr Result& operator=(const Result& other)
        requires(std::is_copy_constructible_v<ErrorT> && std::is_copy_assignable_v<ErrorT>)
    {
        if (this == &other) {
            return *this;
        }

        if (!HasValue_ && !other.HasValue_) {
            *ErrorPtr() = *other.ErrorPtr();
            return *this;
        }

        if (!HasValue_) {
            std::destroy_at(ErrorPtr());
        }

        HasValue_ = other.HasValue_;
        if (!HasValue_) {
            std::construct_at(ErrorPtr(), *other.ErrorPtr());
        }

        return *this;
    }

    constexpr Result& operator=(Result&& other) noexcept(std::is_nothrow_move_constructible_v<ErrorT>
                                                         && std::is_nothrow_move_assignable_v<ErrorT>)
        requires(std::is_move_constructible_v<ErrorT> && std::is_move_assignable_v<ErrorT>)
    {
        if (this == &other) {
            return *this;
        }

        if (!HasValue_ && !other.HasValue_) {
            *ErrorPtr() = std::move(*other.ErrorPtr());
            return *this;
        }

        if (!HasValue_) {
            std::destroy_at(ErrorPtr());
        }

        HasValue_ = other.HasValue_;
        if (!HasValue_) {
            std::construct_at(ErrorPtr(), std::move(*other.ErrorPtr()));
        }

        return *this;
    }

    constexpr ~Result()
    {
        if (!HasValue_) {
            std::destroy_at(ErrorPtr());
        }
    }

    /// @brief Returns `true` when result is success.
    [[nodiscard]] constexpr bool HasValue() const noexcept
    {
        return HasValue_;
    }

    /// @brief Alias for `HasValue()`.
    [[nodiscard]] constexpr bool IsOk() const noexcept
    {
        return HasValue_;
    }

    /// @brief Returns `true` when result is failure.
    [[nodiscard]] constexpr bool HasError() const noexcept
    {
        return !HasValue_;
    }

    /// @brief Alias for `HasError()`.
    [[nodiscard]] constexpr bool IsErr() const noexcept
    {
        return !HasValue_;
    }

    /// @brief Boolean conversion; equivalent to `HasValue()`.
    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return HasValue_;
    }

    /**
   * @brief Returns error value.
   * @pre `HasError()` must be true.
   */
    [[nodiscard]] constexpr ErrorT& Error() &
    {
        ZCORE_CONTRACT_REQUIRE(!HasValue_,
                               detail::ContractViolationCode::RESULT_EXPECTED_ERROR,
                               "zcore::Result<void, ErrorT>::Error() requires an error value");
        return *ErrorPtr();
    }

    /**
   * @brief Returns error value.
   * @pre `HasError()` must be true.
   */
    [[nodiscard]] constexpr const ErrorT& Error() const&
    {
        ZCORE_CONTRACT_REQUIRE(!HasValue_,
                               detail::ContractViolationCode::RESULT_EXPECTED_ERROR,
                               "zcore::Result<void, ErrorT>::Error() requires an error value");
        return *ErrorPtr();
    }

    /**
   * @brief Moves and returns error value.
   * @pre `HasError()` must be true.
   */
    [[nodiscard]] constexpr ErrorT&& Error() &&
    {
        ZCORE_CONTRACT_REQUIRE(!HasValue_,
                               detail::ContractViolationCode::RESULT_EXPECTED_ERROR,
                               "zcore::Result<void, ErrorT>::Error() requires an error value");
        return std::move(*ErrorPtr());
    }

    /**
   * @brief Moves and returns error value (const rvalue overload).
   * @pre `HasError()` must be true.
   */
    [[nodiscard]] constexpr const ErrorT&& Error() const&&
    {
        ZCORE_CONTRACT_REQUIRE(!HasValue_,
                               detail::ContractViolationCode::RESULT_EXPECTED_ERROR,
                               "zcore::Result<void, ErrorT>::Error() requires an error value");
        return std::move(*ErrorPtr());
    }

    /// @brief Returns pointer to error or `nullptr`.
    [[nodiscard]] constexpr ErrorT* TryError() noexcept
    {
        return HasValue_ ? nullptr : ErrorPtr();
    }

    /// @brief Returns const pointer to error or `nullptr`.
    [[nodiscard]] constexpr const ErrorT* TryError() const noexcept
    {
        return HasValue_ ? nullptr : ErrorPtr();
    }

    /// @brief Projects error branch into `Option<ErrorT>`.
    [[nodiscard]] constexpr Option<ErrorT> Err() const&
        requires(std::is_copy_constructible_v<ErrorT>)
    {
        if (HasValue_) {
            return None;
        }
        return Option<ErrorT>(*ErrorPtr());
    }

    /// @brief Projects error branch into `Option<ErrorT>` by move.
    [[nodiscard]] constexpr Option<ErrorT> Err() &&
        requires(std::is_move_constructible_v<ErrorT>)
    {
        if (HasValue_) {
            return None;
        }
        return Option<ErrorT>(std::move(*ErrorPtr()));
    }

    /**
   * @brief Maps success to a value-producing callback.
   * @param fn Callback executed only on success.
   */
    template <typename FuncT>
        requires(std::is_copy_constructible_v<ErrorT> && std::is_invocable_v<FuncT>)
    [[nodiscard]] constexpr auto Map(FuncT&& fn) const& -> Result<std::invoke_result_t<FuncT>, ErrorT>
    {
        using NextValueT = std::invoke_result_t<FuncT>;
        if (HasValue_) {
            if constexpr (std::is_void_v<NextValueT>) {
                std::forward<FuncT>(fn)();
                return Result<NextValueT, ErrorT>::Success();
            }
            else {
                return Result<NextValueT, ErrorT>::Success(std::forward<FuncT>(fn)());
            }
        }
        return Result<NextValueT, ErrorT>::Failure(*ErrorPtr());
    }

    /**
   * @brief Maps success with eager fallback.
   * @param defaultValue Fallback used when error is present.
   * @param fn Callback executed on success.
   */
    template <typename DefaultT, typename FuncT>
        requires(std::is_invocable_v<FuncT> && std::is_convertible_v<DefaultT &&, std::invoke_result_t<FuncT>>)
    [[nodiscard]] constexpr auto MapOr(DefaultT&& defaultValue, FuncT&& fn) const& -> std::invoke_result_t<FuncT>
    {
        using MappedT = std::invoke_result_t<FuncT>;
        static_assert(!std::is_void_v<MappedT>, "Result::MapOr callback must return a value type.");
        if (HasValue_) {
            return std::forward<FuncT>(fn)();
        }
        return static_cast<MappedT>(std::forward<DefaultT>(defaultValue));
    }

    /**
   * @brief Maps success with lazy fallback from error.
   * @param defaultFn Fallback mapper for error branch.
   * @param fn Callback executed on success.
   */
    template <typename DefaultFuncT, typename FuncT>
        requires(std::is_invocable_v<DefaultFuncT, const ErrorT&> && std::is_invocable_v<FuncT>
                 && std::is_convertible_v<std::invoke_result_t<DefaultFuncT, const ErrorT&>, std::invoke_result_t<FuncT>>)
    [[nodiscard]] constexpr auto MapOrElse(DefaultFuncT&& defaultFn, FuncT&& fn) const& -> std::invoke_result_t<FuncT>
    {
        using MappedT = std::invoke_result_t<FuncT>;
        static_assert(!std::is_void_v<MappedT>, "Result::MapOrElse callback must return a value type.");
        if (HasValue_) {
            return std::forward<FuncT>(fn)();
        }
        return static_cast<MappedT>(std::forward<DefaultFuncT>(defaultFn)(*ErrorPtr()));
    }

    /**
   * @brief Chains another result-producing operation on success.
   * @param fn Callback returning another `Result`.
   */
    template <typename FuncT>
        requires(std::is_invocable_v<FuncT>)
    [[nodiscard]] constexpr auto AndThen(FuncT&& fn) const& -> std::invoke_result_t<FuncT>
    {
        using NextResultT = std::invoke_result_t<FuncT>;
        static_assert(detail::kIsResultV<NextResultT>, "AndThen callback must return zcore::Result<U, E>.");
        using NextErrorT = typename NextResultT::ErrorType;
        static_assert(std::is_convertible_v<const ErrorT&, NextErrorT>,
                      "AndThen requires current error type to be convertible to next result error type.");

        if (HasValue_) {
            return std::forward<FuncT>(fn)();
        }
        return NextResultT::Failure(static_cast<NextErrorT>(*ErrorPtr()));
    }

    /**
   * @brief Recovers from error with callback returning another result.
   * @param fn Error recovery callback.
   */
    template <typename FuncT>
        requires(std::is_invocable_v<FuncT, const ErrorT&>)
    [[nodiscard]] constexpr auto OrElse(FuncT&& fn) const& -> std::invoke_result_t<FuncT, const ErrorT&>
    {
        using NextResultT = std::invoke_result_t<FuncT, const ErrorT&>;
        static_assert(detail::kIsResultV<NextResultT>, "OrElse callback must return zcore::Result<void, F>.");
        static_assert(std::is_same_v<typename NextResultT::ValueType, void>,
                      "OrElse for Result<void, E> requires callback result with void value type.");

        if (HasValue_) {
            return NextResultT::Success();
        }
        return std::forward<FuncT>(fn)(*ErrorPtr());
    }

    /**
   * @brief Maps error value while preserving success branch.
   * @param fn Error mapping callback.
   */
    template <typename FuncT>
        requires(std::is_invocable_v<FuncT, const ErrorT&>)
    [[nodiscard]] constexpr auto MapError(FuncT&& fn) const& -> Result<void, std::invoke_result_t<FuncT, const ErrorT&>>
    {
        using NextErrorT = std::invoke_result_t<FuncT, const ErrorT&>;
        static_assert(!std::is_void_v<NextErrorT>, "Result::MapError callback must return a value type.");
        if (HasValue_) {
            return Result<void, NextErrorT>::Success();
        }
        return Result<void, NextErrorT>::Failure(std::forward<FuncT>(fn)(*ErrorPtr()));
    }

    /**
   * @brief Observes success branch without mutating state.
   * @param fn Callback executed only on success.
   * @return `*this`.
   */
    template <typename FuncT>
        requires(std::is_invocable_v<FuncT>)
    constexpr const Result& Inspect(FuncT&& fn) const&
    {
        if (HasValue_) {
            std::forward<FuncT>(fn)();
        }
        return *this;
    }

    /**
   * @brief Observes error branch without mutating state.
   * @param fn Callback executed only on error.
   * @return `*this`.
   */
    template <typename FuncT>
        requires(std::is_invocable_v<FuncT, const ErrorT&>)
    constexpr const Result& InspectError(FuncT&& fn) const&
    {
        if (!HasValue_) {
            std::forward<FuncT>(fn)(*ErrorPtr());
        }
        return *this;
    }

private:
    constexpr Result() noexcept : HasValue_(true)
    {
    }

    explicit constexpr Result(const ErrorT& error) : HasValue_(false)
    {
        std::construct_at(ErrorPtr(), error);
    }

    explicit constexpr Result(ErrorT&& error) : HasValue_(false)
    {
        std::construct_at(ErrorPtr(), std::move(error));
    }

    [[nodiscard]] constexpr ErrorT* ErrorPtr() noexcept
    {
        return std::launder(reinterpret_cast<ErrorT*>(&Storage_));
    }

    [[nodiscard]] constexpr const ErrorT* ErrorPtr() const noexcept
    {
        return std::launder(reinterpret_cast<const ErrorT*>(&Storage_));
    }

    alignas(ErrorT) std::byte Storage_[sizeof(ErrorT)];
    bool HasValue_;
};

} // namespace zcore
