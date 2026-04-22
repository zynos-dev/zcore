/**************************************************************************/
/*  option.hpp                                                            */
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
 * @file include/zcore/error/option.hpp
 * @brief Optional value type representing presence or absence.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/option.hpp>
 * zcore::Option<int> maybeValue = 42;
 * @endcode
 */

#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <zcore/contract_violation.hpp>
#include <zcore/type_constraints.hpp>

namespace zcore {

template <typename ValueT>
class Option;

namespace detail {

template <typename OptionT>
struct IsOptionType final : std::false_type {};

template <typename ValueT>
struct IsOptionType<Option<ValueT>> final : std::true_type {};

template <typename OptionT>
inline constexpr bool kIsOptionTypeV = IsOptionType<OptionT>::value;

} // namespace detail

/**
 * @brief Sentinel type representing the empty state for `Option<T>`.
 */
struct NoneType final {
    explicit constexpr NoneType(int)
    {
    }
};

/// @brief Internal constexpr storage for `None`.
inline constexpr NoneType kNone{0};
/// @brief Public singleton used to construct/reset empty options.
inline constexpr const NoneType& None = kNone;

/**
 * @brief Optional value container.
 *
 * @tparam ValueT Contained value type.
 *
 * Models either "some value" or "no value" without exceptions.
 */
template <typename ValueT>
class [[nodiscard("Option must be handled explicitly.")]] Option final {
public:
    ZCORE_STATIC_REQUIRE(constraints::NonReferenceType<ValueT>&& constraints::NonVoidType<ValueT>&&
                                 constraints::NonArrayType<ValueT>&& constraints::NonFunctionType<ValueT>,
                         "Option<ValueT> requires non-reference non-void non-array non-function value type.");
    ZCORE_STATIC_REQUIRE(!std::is_same_v<std::remove_cv_t<ValueT>, NoneType>, "Option<NoneType> is invalid.");

    using ValueType = ValueT;

    /// @brief Constructs an empty option.
    constexpr Option() noexcept : HasValue_(false)
    {
    }
    /// @brief Constructs an empty option from `None`.
    constexpr Option(NoneType) noexcept : HasValue_(false)
    {
    }

    /**
   * @brief Constructs an option containing a copied value.
   * @param value Value to copy.
   */
    explicit constexpr Option(const ValueT& value) : HasValue_(true)
    {
        std::construct_at(Ptr(), value);
    }

    /**
   * @brief Constructs an option containing a moved value.
   * @param value Value to move.
   */
    explicit constexpr Option(ValueT&& value) noexcept(std::is_nothrow_move_constructible_v<ValueT>) : HasValue_(true)
    {
        std::construct_at(Ptr(), std::move(value));
    }

    constexpr Option(const Option& other)
        requires(std::is_copy_constructible_v<ValueT>)
            : HasValue_(other.HasValue_)
    {
        if (HasValue_) {
            std::construct_at(Ptr(), *other.Ptr());
        }
    }

    constexpr Option(Option&& other) noexcept(std::is_nothrow_move_constructible_v<ValueT>)
        requires(std::is_move_constructible_v<ValueT>)
            : HasValue_(other.HasValue_)
    {
        if (HasValue_) {
            std::construct_at(Ptr(), std::move(*other.Ptr()));
        }
    }

    /// @brief Resets this option to empty.
    constexpr Option& operator=(NoneType) noexcept
    {
        ResetStorage();
        return *this;
    }

    constexpr Option& operator=(const Option& other)
        requires(std::is_copy_constructible_v<ValueT> && std::is_copy_assignable_v<ValueT>)
    {
        if (this == &other) {
            return *this;
        }

        if (HasValue_ && other.HasValue_) {
            *Ptr() = *other.Ptr();
            return *this;
        }

        if (other.HasValue_) {
            Emplace(*other.Ptr());
            return *this;
        }

        ResetStorage();
        return *this;
    }

    constexpr Option& operator=(Option&& other) noexcept(std::is_nothrow_move_constructible_v<ValueT>
                                                         && std::is_nothrow_move_assignable_v<ValueT>)
        requires(std::is_move_constructible_v<ValueT> && std::is_move_assignable_v<ValueT>)
    {
        if (this == &other) {
            return *this;
        }

        if (HasValue_ && other.HasValue_) {
            *Ptr() = std::move(*other.Ptr());
            return *this;
        }

        if (other.HasValue_) {
            Emplace(std::move(*other.Ptr()));
            return *this;
        }

        ResetStorage();
        return *this;
    }

    constexpr ~Option()
    {
        ResetStorage();
    }

    /// @brief Returns `true` when a value is present.
    [[nodiscard]] constexpr bool HasValue() const noexcept
    {
        return HasValue_;
    }
    /// @brief Returns `true` when a value is present.
    [[nodiscard]] constexpr bool IsSome() const noexcept
    {
        return HasValue_;
    }
    /// @brief Returns `true` when no value is present.
    [[nodiscard]] constexpr bool IsNone() const noexcept
    {
        return !HasValue_;
    }
    /// @brief Boolean conversion; equivalent to `HasValue()`.
    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return HasValue_;
    }

    /// @brief Returns mutable pointer to contained value or `nullptr`.
    [[nodiscard]] constexpr ValueT* TryValue() noexcept
    {
        return HasValue_ ? Ptr() : nullptr;
    }

    /// @brief Returns const pointer to contained value or `nullptr`.
    [[nodiscard]] constexpr const ValueT* TryValue() const noexcept
    {
        return HasValue_ ? Ptr() : nullptr;
    }

    /**
   * @brief Returns contained value reference.
   * @pre `HasValue()` must be true.
   */
    [[nodiscard]] constexpr ValueT& Value() &
    {
        CheckHasValue();
        return *Ptr();
    }

    /**
   * @brief Returns contained value reference.
   * @pre `HasValue()` must be true.
   */
    [[nodiscard]] constexpr const ValueT& Value() const&
    {
        CheckHasValue();
        return *Ptr();
    }

    /**
   * @brief Moves and returns contained value.
   * @pre `HasValue()` must be true.
   */
    [[nodiscard]] constexpr ValueT&& Value() &&
    {
        CheckHasValue();
        return std::move(*Ptr());
    }

    /**
   * @brief Moves and returns contained value (const rvalue overload).
   * @pre `HasValue()` must be true.
   */
    [[nodiscard]] constexpr const ValueT&& Value() const&&
    {
        CheckHasValue();
        return std::move(*Ptr());
    }

    /**
   * @brief Maps a present value to another value type.
   * @param fn Mapping callback.
   * @return `Option<U>` where `U` is callback return type.
   */
    template <typename FuncT>
        requires(std::is_invocable_v<FuncT, const ValueT&>)
    [[nodiscard]] constexpr auto Map(FuncT&& fn) const& -> Option<std::invoke_result_t<FuncT, const ValueT&>>
    {
        using NextValueT = std::invoke_result_t<FuncT, const ValueT&>;
        static_assert(!std::is_void_v<NextValueT>, "Option::Map callback must return a value type. Use Inspect for side effects.");
        if (HasValue_) {
            return Option<NextValueT>(std::forward<FuncT>(fn)(*Ptr()));
        }
        return None;
    }

    /**
   * @brief Chains option-producing callback when value is present.
   * @param fn Callback returning `Option<U>`.
   * @return Callback result when present, otherwise `None`.
   */
    template <typename FuncT>
        requires(std::is_invocable_v<FuncT, const ValueT&> && detail::kIsOptionTypeV<std::invoke_result_t<FuncT, const ValueT&>>)
    [[nodiscard]] constexpr auto AndThen(FuncT&& fn) const& -> std::invoke_result_t<FuncT, const ValueT&>
    {
        if (HasValue_) {
            return std::forward<FuncT>(fn)(*Ptr());
        }
        return None;
    }

    /**
   * @brief Returns current option if present, otherwise computes fallback.
   * @param fn Fallback producer.
   */
    template <typename FuncT>
        requires(std::is_invocable_r_v<Option<ValueT>, FuncT> && std::is_copy_constructible_v<ValueT>)
    [[nodiscard]] constexpr Option OrElse(FuncT&& fn) const&
    {
        if (HasValue_) {
            return Option(*Ptr());
        }
        return std::forward<FuncT>(fn)();
    }

    /**
   * @brief Moves current option if present, otherwise computes fallback.
   * @param fn Fallback producer.
   */
    template <typename FuncT>
        requires(std::is_invocable_r_v<Option<ValueT>, FuncT> && std::is_move_constructible_v<ValueT>)
    [[nodiscard]] constexpr Option OrElse(FuncT&& fn) &&
    {
        if (HasValue_) {
            return Option(std::move(*Ptr()));
        }
        return std::forward<FuncT>(fn)();
    }

    /**
   * @brief Returns contained value or provided fallback.
   * @param defaultValue Value used when option is empty.
   */
    template <typename DefaultValueT>
        requires(std::is_convertible_v<DefaultValueT &&, ValueT>)
    [[nodiscard]] constexpr ValueT ValueOr(DefaultValueT&& defaultValue) const&
    {
        if (HasValue_) {
            return *Ptr();
        }
        return static_cast<ValueT>(std::forward<DefaultValueT>(defaultValue));
    }

    /**
   * @brief Moves contained value or returns provided fallback.
   * @param defaultValue Value used when option is empty.
   */
    template <typename DefaultValueT>
        requires(std::is_convertible_v<DefaultValueT &&, ValueT>)
    [[nodiscard]] constexpr ValueT ValueOr(DefaultValueT&& defaultValue) &&
    {
        if (HasValue_) {
            return std::move(*Ptr());
        }
        return static_cast<ValueT>(std::forward<DefaultValueT>(defaultValue));
    }

    /**
   * @brief Maps contained value or returns direct fallback.
   * @param defaultValue Fallback mapped value.
   * @param fn Mapping callback.
   */
    template <typename DefaultT, typename FuncT>
        requires(std::is_convertible_v<DefaultT &&, std::invoke_result_t<FuncT, const ValueT&>>
                 && std::is_invocable_v<FuncT, const ValueT&>)
    [[nodiscard]] constexpr auto MapOr(DefaultT&& defaultValue, FuncT&& fn) const& -> std::invoke_result_t<FuncT, const ValueT&>
    {
        using MappedT = std::invoke_result_t<FuncT, const ValueT&>;
        static_assert(!std::is_void_v<MappedT>, "Option::MapOr callback must return a value type.");
        if (HasValue_) {
            return std::forward<FuncT>(fn)(*Ptr());
        }
        return static_cast<MappedT>(std::forward<DefaultT>(defaultValue));
    }

    /**
   * @brief Maps contained value or lazily computes fallback.
   * @param defaultFn Fallback producer.
   * @param fn Mapping callback.
   */
    template <typename DefaultFuncT, typename FuncT>
        requires(std::is_invocable_v<DefaultFuncT> && std::is_invocable_v<FuncT, const ValueT&>
                 && std::is_convertible_v<std::invoke_result_t<DefaultFuncT>, std::invoke_result_t<FuncT, const ValueT&>>)
    [[nodiscard]] constexpr auto MapOrElse(DefaultFuncT&& defaultFn,
                                           FuncT&& fn) const& -> std::invoke_result_t<FuncT, const ValueT&>
    {
        using MappedT = std::invoke_result_t<FuncT, const ValueT&>;
        static_assert(!std::is_void_v<MappedT>, "Option::MapOrElse callback must return a value type.");
        if (HasValue_) {
            return std::forward<FuncT>(fn)(*Ptr());
        }
        return static_cast<MappedT>(std::forward<DefaultFuncT>(defaultFn)());
    }

    /**
   * @brief Runs callback when value is present without changing state.
   * @param fn Observer callback.
   * @return `*this`.
   */
    template <typename FuncT>
        requires(std::is_invocable_v<FuncT, const ValueT&>)
    constexpr const Option& Inspect(FuncT&& fn) const&
    {
        if (HasValue_) {
            std::forward<FuncT>(fn)(*Ptr());
        }
        return *this;
    }

    /**
   * @brief Replaces current state with in-place constructed value.
   * @param args Constructor args.
   * @return Reference to newly emplaced value.
   */
    template <typename... ArgsT>
    constexpr ValueT& Emplace(ArgsT&&... args)
    {
        ResetStorage();
        std::construct_at(Ptr(), std::forward<ArgsT>(args)...);
        HasValue_ = true;
        return *Ptr();
    }

    /// @brief Clears option to empty state.
    constexpr void Reset() noexcept
    {
        ResetStorage();
    }

    /**
   * @brief Moves value out and leaves this option empty.
   * @return Previous value wrapped in `Option`, or `None`.
   */
    [[nodiscard]] constexpr Option Take()
        requires(std::is_move_constructible_v<ValueT>)
    {
        if (!HasValue_) {
            return None;
        }
        Option<ValueT> moved(std::move(*Ptr()));
        ResetStorage();
        return moved;
    }

    /**
   * @brief Replaces contained value and returns previous one.
   * @param replacement New value.
   * @return Previous option state.
   */
    template <typename AssignValueT>
        requires(std::is_constructible_v<ValueT, AssignValueT &&>)
    [[nodiscard]] constexpr Option Replace(AssignValueT&& replacement)
    {
        Option<ValueT> previous = Take();
        Emplace(std::forward<AssignValueT>(replacement));
        return previous;
    }

private:
    constexpr void CheckHasValue() const
    {
        ZCORE_CONTRACT_REQUIRE(HasValue_,
                               detail::ContractViolationCode::OPTION_EMPTY_VALUE_ACCESS,
                               "zcore::Option::Value() called on None");
    }

    [[nodiscard]] constexpr ValueT* Ptr() noexcept
    {
        return std::launder(reinterpret_cast<ValueT*>(&Storage_));
    }

    [[nodiscard]] constexpr const ValueT* Ptr() const noexcept
    {
        return std::launder(reinterpret_cast<const ValueT*>(&Storage_));
    }

    constexpr void ResetStorage() noexcept
    {
        if (HasValue_) {
            std::destroy_at(Ptr());
            HasValue_ = false;
        }
    }

    alignas(ValueT) std::byte Storage_[sizeof(ValueT)];
    bool HasValue_;
};

/**
 * @brief Creates an engaged `Option` from a value.
 * @tparam ValueT Input value type.
 * @param value Value to store.
 * @return `Option<std::remove_cvref_t<ValueT>>` containing the value.
 */
template <typename ValueT>
[[nodiscard]] constexpr auto Some(ValueT&& value) -> Option<std::remove_cvref_t<ValueT>>
{
    return Option<std::remove_cvref_t<ValueT>>(std::forward<ValueT>(value));
}

} // namespace zcore
