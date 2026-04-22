/**************************************************************************/
/*  id.hpp                                                                */
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
 * @file include/zcore/id.hpp
 * @brief Strongly typed identifier wrapper with explicit invalid sentinel.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/id.hpp>
 * struct UserTag;
 * using UserId = zcore::Id<UserTag>;
 * @endcode
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <zcore/hash/customization.hpp>

namespace zcore {

/**
 * @brief Strongly typed identifier with explicit invalid sentinel.
 *
 * @tparam TagT Phantom tag type to prevent mixing unrelated ids.
 * @tparam ValueT Underlying unsigned storage type.
 * @tparam InvalidV Sentinel value considered invalid.
 */
template <typename TagT, typename ValueT = std::uint64_t, ValueT InvalidV = ValueT{0}>
class [[nodiscard("Id must be handled explicitly.")]] Id final {
public:
    static_assert(std::is_unsigned_v<ValueT>, "Id<TagT, ValueT, InvalidV> requires an unsigned integral ValueT.");
    static_assert(!std::is_same_v<TagT, void>, "Id<TagT, ValueT, InvalidV> requires a concrete tag type.");

    using TagType = TagT;
    using ValueType = ValueT;

    /// @brief Invalid sentinel encoded in this `Id` type.
    static constexpr ValueT kInvalidValue = InvalidV;

    /// @brief Constructs an invalid id.
    constexpr Id() noexcept : Value_(InvalidV)
    {
    }
    /**
   * @brief Constructs an id from a raw value.
   * @param value Raw identifier value.
   */
    constexpr explicit Id(ValueT value) noexcept : Value_(value)
    {
    }

    constexpr Id(const Id&) noexcept = default;
    constexpr Id& operator=(const Id&) noexcept = default;
    constexpr Id(Id&&) noexcept = default;
    constexpr Id& operator=(Id&&) noexcept = default;
    ~Id() = default;

    /**
   * @brief Returns a canonical invalid id.
   * @return Id with value `InvalidV`.
   */
    [[nodiscard]] static constexpr Id Invalid() noexcept
    {
        return Id(InvalidV);
    }

    /**
   * @brief Constructs from raw value without validation.
   * @param value Raw identifier value.
   * @return Id containing `value`.
   */
    [[nodiscard]] static constexpr Id FromRawUnchecked(ValueT value) noexcept
    {
        return Id(value);
    }

    /// @brief Returns the underlying raw value.
    [[nodiscard]] constexpr ValueT Raw() const noexcept
    {
        return Value_;
    }

    /// @brief Returns `true` when value is not equal to `InvalidV`.
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return Value_ != InvalidV;
    }

    /// @brief Returns `true` when value equals `InvalidV`.
    [[nodiscard]] constexpr bool IsInvalid() const noexcept
    {
        return Value_ == InvalidV;
    }

    /// @brief Resets id value to `InvalidV`.
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

    [[nodiscard]] constexpr bool operator==(const Id&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const Id&) const noexcept = default;

private:
    ValueT Value_;
};

} // namespace zcore

namespace zcore::hash {

/**
 * @brief `zcore::hash` specialization for `zcore::Id`.
 */
template <typename TagT, typename ValueT, ValueT InvalidV>
struct Hash<zcore::Id<TagT, ValueT, InvalidV>, void> final {
    [[nodiscard]] constexpr Hash64 operator()(const zcore::Id<TagT, ValueT, InvalidV>& value, u64 seed = 0ULL) const noexcept
    {
        return Hash<ValueT>{}(value.Raw(), seed);
    }
};

} // namespace zcore::hash

namespace std {

/**
 * @brief `std::hash` adapter for `zcore::Id` to support standard unordered containers.
 */
template <typename TagT, typename ValueT, ValueT InvalidV>
struct hash<zcore::Id<TagT, ValueT, InvalidV>> final {
    [[nodiscard]] std::size_t operator()(const zcore::Id<TagT, ValueT, InvalidV>& value) const noexcept
    {
        return zcore::hash::DigestToSizeT(zcore::hash::Hash<zcore::Id<TagT, ValueT, InvalidV>>{}(value));
    }
};

} // namespace std
