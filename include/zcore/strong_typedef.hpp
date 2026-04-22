/**************************************************************************/
/*  strong_typedef.hpp                                                    */
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
 * @file include/zcore/strong_typedef.hpp
 * @brief Strongly typed value wrapper with explicit invalid sentinel.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/strong_typedef.hpp>
 * struct UserTag final {};
 * using UserToken = zcore::StrongTypedef<UserTag>;
 * @endcode
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <zcore/foundation.hpp>
#include <zcore/hash/customization.hpp>
#include <zcore/type_constraints.hpp>

namespace zcore {

/**
 * @brief Strongly typed value wrapper with explicit invalid sentinel.
 *
 * @tparam TagT Phantom tag type to prevent mixing unrelated domains.
 * @tparam ValueT Underlying storage/value type.
 * @tparam InvalidV Sentinel value considered invalid.
 */
template <typename TagT, typename ValueT = std::uint64_t, ValueT InvalidV = ValueT{0}>
class [[nodiscard("StrongTypedef must be handled explicitly.")]] StrongTypedef final {
public:
    ZCORE_STATIC_REQUIRE(!std::is_same_v<TagT, void>, "StrongTypedef<TagT, ValueT, InvalidV> requires a concrete tag type.");
    ZCORE_STATIC_REQUIRE(constraints::ObjectType<ValueT>&& constraints::NonReferenceNonVoidType<ValueT>,
                         "StrongTypedef<ValueT> requires non-reference non-void object value type.");

    using TagType = TagT;
    using ValueType = ValueT;

    /// @brief Invalid sentinel encoded in this strong typedef.
    static constexpr ValueT kInvalidValue = InvalidV;

    /// @brief Constructs invalid value.
    constexpr StrongTypedef() noexcept : Value_(InvalidV)
    {
    }

    /**
   * @brief Constructs from raw value.
   * @param value Raw wrapped value.
   */
    constexpr explicit StrongTypedef(ValueT value) noexcept : Value_(value)
    {
    }

    constexpr StrongTypedef(const StrongTypedef&) noexcept = default;
    constexpr StrongTypedef& operator=(const StrongTypedef&) noexcept = default;
    constexpr StrongTypedef(StrongTypedef&&) noexcept = default;
    constexpr StrongTypedef& operator=(StrongTypedef&&) noexcept = default;
    ~StrongTypedef() = default;

    /// @brief Returns canonical invalid value.
    [[nodiscard]] static constexpr StrongTypedef Invalid() noexcept
    {
        return StrongTypedef(InvalidV);
    }

    /**
   * @brief Constructs from raw value without validation.
   * @param value Raw wrapped value.
   */
    [[nodiscard]] static constexpr StrongTypedef FromRawUnchecked(ValueT value) noexcept
    {
        return StrongTypedef(value);
    }

    /// @brief Returns wrapped raw value.
    [[nodiscard]] constexpr ValueT Raw() const noexcept
    {
        return Value_;
    }

    /// @brief Returns `true` when value differs from invalid sentinel.
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return Value_ != InvalidV;
    }

    /// @brief Returns `true` when value equals invalid sentinel.
    [[nodiscard]] constexpr bool IsInvalid() const noexcept
    {
        return Value_ == InvalidV;
    }

    /// @brief Resets wrapped value to invalid sentinel.
    constexpr void Reset() noexcept
    {
        Value_ = InvalidV;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return IsValid();
    }

    [[nodiscard]] constexpr explicit operator ValueT() const noexcept
    {
        return Value_;
    }

    [[nodiscard]] constexpr bool operator==(const StrongTypedef&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const StrongTypedef&) const noexcept = default;

private:
    ValueT Value_;
};

} // namespace zcore

namespace zcore::hash {

/**
 * @brief `zcore::hash` specialization for `zcore::StrongTypedef`.
 */
template <typename TagT, typename ValueT, ValueT InvalidV>
struct Hash<zcore::StrongTypedef<TagT, ValueT, InvalidV>, void> final {
    [[nodiscard]] constexpr Hash64 operator()(const zcore::StrongTypedef<TagT, ValueT, InvalidV>& value,
                                              u64 seed = 0ULL) const noexcept
    {
        return Hash<ValueT>{}(value.Raw(), seed);
    }
};

} // namespace zcore::hash

namespace std {

/**
 * @brief `std::hash` adapter for `zcore::StrongTypedef`.
 */
template <typename TagT, typename ValueT, ValueT InvalidV>
struct hash<zcore::StrongTypedef<TagT, ValueT, InvalidV>> final {
    [[nodiscard]] std::size_t operator()(const zcore::StrongTypedef<TagT, ValueT, InvalidV>& value) const noexcept
    {
        return zcore::hash::DigestToSizeT(zcore::hash::Hash<zcore::StrongTypedef<TagT, ValueT, InvalidV>>{}(value));
    }
};

} // namespace std
