/**************************************************************************/
/*  type_info.hpp                                                         */
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
 * @file include/zcore/type_info.hpp
 * @brief Deterministic compile-time type metadata contract.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/type_info.hpp>
 * constexpr zcore::TypeInfo info = zcore::TypeInfo::Of<int>();
 * @endcode
 */

#pragma once

#include <cstddef>
#include <functional>
#include <string_view>
#include <type_traits>
#include <zcore/hash/customization.hpp>
#include <zcore/type_constraints.hpp>
#include <zcore/type_id.hpp>

namespace zcore {

/**
 * @brief Deterministic metadata token for an exact C++ type.
 */
class [[nodiscard("TypeInfo must be handled explicitly.")]] TypeInfo final {
public:
    using IdType = TypeId;
    using NameType = std::string_view;
    using SizeType = usize;
    using AlignmentType = usize;

    /// @brief Constructs invalid type metadata.
    constexpr TypeInfo() noexcept
            : Id_(TypeId::Invalid())
            , Name_()
            , Size_(0U)
            , Alignment_(0U)
            , IsTriviallyCopyable_(false)
            , IsTriviallyDestructible_(false)
            , IsTriviallyDefaultConstructible_(false)
    {
    }

    /**
   * @brief Constructs metadata from raw values.
   * @param id Type identity token.
   * @param name Type name/view token.
   * @param size Object size in bytes.
   * @param alignment Object alignment in bytes.
   * @param isTriviallyCopyable Trivial-copyability trait.
   * @param isTriviallyDestructible Trivial-destructibility trait.
   * @param isTriviallyDefaultConstructible Trivial-default-construction trait.
   */
    constexpr TypeInfo(TypeId id,
                       std::string_view name,
                       usize size,
                       usize alignment,
                       bool isTriviallyCopyable,
                       bool isTriviallyDestructible,
                       bool isTriviallyDefaultConstructible) noexcept
            : Id_(id)
            , Name_(name)
            , Size_(size)
            , Alignment_(alignment)
            , IsTriviallyCopyable_(isTriviallyCopyable)
            , IsTriviallyDestructible_(isTriviallyDestructible)
            , IsTriviallyDefaultConstructible_(isTriviallyDefaultConstructible)
    {
    }

    constexpr TypeInfo(const TypeInfo&) noexcept = default;
    constexpr TypeInfo& operator=(const TypeInfo&) noexcept = default;
    constexpr TypeInfo(TypeInfo&&) noexcept = default;
    constexpr TypeInfo& operator=(TypeInfo&&) noexcept = default;
    ~TypeInfo() = default;

    /// @brief Returns canonical invalid metadata token.
    [[nodiscard]] static constexpr TypeInfo Invalid() noexcept
    {
        return TypeInfo();
    }

    /**
   * @brief Constructs metadata from raw values without validation.
   * @return Metadata token containing provided raw values.
   */
    [[nodiscard]] static constexpr TypeInfo FromRawUnchecked(TypeId id,
                                                             std::string_view name,
                                                             usize size,
                                                             usize alignment,
                                                             bool isTriviallyCopyable,
                                                             bool isTriviallyDestructible,
                                                             bool isTriviallyDefaultConstructible) noexcept
    {
        return TypeInfo(id, name, size, alignment, isTriviallyCopyable, isTriviallyDestructible, isTriviallyDefaultConstructible);
    }

    /**
   * @brief Returns deterministic metadata for exact type `ValueT`.
   * @tparam ValueT Type to describe.
   */
    template <typename ValueT>
    [[nodiscard]] static consteval TypeInfo Of() noexcept
    {
        ZCORE_STATIC_REQUIRE(constraints::NonReferenceNonVoidType<ValueT>,
                             "TypeInfo::Of<ValueT>() requires non-reference non-void ValueT.");

        return TypeInfo(TypeId::Of<ValueT>(),
                        TypeNameOf<ValueT>(),
                        sizeof(ValueT),
                        alignof(ValueT),
                        std::is_trivially_copyable_v<ValueT>,
                        std::is_trivially_destructible_v<ValueT>,
                        std::is_trivially_default_constructible_v<ValueT>);
    }

    /// @brief Returns type identity token.
    [[nodiscard]] constexpr TypeId Id() const noexcept
    {
        return Id_;
    }

    /// @brief Returns deterministic type name/view token.
    [[nodiscard]] constexpr std::string_view Name() const noexcept
    {
        return Name_;
    }

    /// @brief Returns object size in bytes.
    [[nodiscard]] constexpr usize Size() const noexcept
    {
        return Size_;
    }

    /// @brief Returns object alignment in bytes.
    [[nodiscard]] constexpr usize Alignment() const noexcept
    {
        return Alignment_;
    }

    /// @brief Returns `true` when type is trivially copyable.
    [[nodiscard]] constexpr bool IsTriviallyCopyable() const noexcept
    {
        return IsTriviallyCopyable_;
    }

    /// @brief Returns `true` when type is trivially destructible.
    [[nodiscard]] constexpr bool IsTriviallyDestructible() const noexcept
    {
        return IsTriviallyDestructible_;
    }

    /// @brief Returns `true` when type is trivially default constructible.
    [[nodiscard]] constexpr bool IsTriviallyDefaultConstructible() const noexcept
    {
        return IsTriviallyDefaultConstructible_;
    }

    /// @brief Returns compact trait bitmask in stable order.
    [[nodiscard]] constexpr u8 TraitBits() const noexcept
    {
        return static_cast<u8>((IsTriviallyCopyable_ ? 0x1U : 0U) | (IsTriviallyDestructible_ ? 0x2U : 0U)
                               | (IsTriviallyDefaultConstructible_ ? 0x4U : 0U));
    }

    /// @brief Returns `true` when metadata is structurally valid.
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return Id_.IsValid() && !Name_.empty() && Size_ > 0U && Alignment_ > 0U;
    }

    /// @brief Returns `true` when metadata is invalid.
    [[nodiscard]] constexpr bool IsInvalid() const noexcept
    {
        return !IsValid();
    }

    /// @brief Resets to canonical invalid metadata.
    constexpr void Reset() noexcept
    {
        *this = TypeInfo::Invalid();
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return IsValid();
    }

    [[nodiscard]] constexpr bool operator==(const TypeInfo&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const TypeInfo&) const noexcept = default;

private:
    template <typename ValueT>
    [[nodiscard]] static consteval std::string_view TypeNameOf() noexcept
    {
#if defined(__clang__) || defined(__GNUC__)
        return std::string_view(__PRETTY_FUNCTION__, sizeof(__PRETTY_FUNCTION__) - 1U);
#elif defined(_MSC_VER)
        return std::string_view(__FUNCSIG__, sizeof(__FUNCSIG__) - 1U);
#else
        return std::string_view("zcore::TypeInfo::unknown", sizeof("zcore::TypeInfo::unknown") - 1U);
#endif
    }

    TypeId Id_;
    std::string_view Name_;
    usize Size_;
    usize Alignment_;
    bool IsTriviallyCopyable_;
    bool IsTriviallyDestructible_;
    bool IsTriviallyDefaultConstructible_;
};

} // namespace zcore

namespace zcore::hash {

/**
 * @brief `zcore::hash` specialization for `zcore::TypeInfo`.
 */
template <>
struct Hash<zcore::TypeInfo, void> final {
    [[nodiscard]] constexpr Hash64 operator()(const zcore::TypeInfo& value, u64 seed = 0ULL) const noexcept
    {
        const Hash64 idDigest = Hash<zcore::TypeId>{}(value.Id(), seed);
        const Hash64 sizeDigest = Hash<zcore::usize>{}(value.Size(), idDigest);
        const Hash64 alignmentDigest = Hash<zcore::usize>{}(value.Alignment(), sizeDigest);
        return Hash<zcore::u8>{}(value.TraitBits(), alignmentDigest);
    }
};

} // namespace zcore::hash

namespace std {

/**
 * @brief `std::hash` adapter for `zcore::TypeInfo`.
 */
template <>
struct hash<zcore::TypeInfo> final {
    [[nodiscard]] std::size_t operator()(const zcore::TypeInfo& value) const noexcept
    {
        return zcore::hash::DigestToSizeT(zcore::hash::Hash<zcore::TypeInfo>{}(value));
    }
};

} // namespace std
