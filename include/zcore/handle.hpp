/**************************************************************************/
/*  handle.hpp                                                            */
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
 * @file include/zcore/handle.hpp
 * @brief Stable typed handle with index and generation components.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/handle.hpp>
 * struct EntityTag final {};
 * using EntityHandle = zcore::Handle<EntityTag>;
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
 * @brief Strongly typed handle for stable index-generation identity.
 *
 * @tparam TagT Phantom tag type to prevent mixing unrelated handle domains.
 * @tparam IndexT Unsigned index storage type.
 * @tparam GenerationT Unsigned generation storage type.
 * @tparam InvalidIndexV Invalid index sentinel.
 * @tparam InvalidGenerationV Invalid generation sentinel.
 */
template <typename TagT,
          typename IndexT = std::uint32_t,
          typename GenerationT = std::uint32_t,
          IndexT InvalidIndexV = IndexT{0},
          GenerationT InvalidGenerationV = GenerationT{0}>
class [[nodiscard("Handle must be handled explicitly.")]] Handle final {
public:
    static_assert(std::is_unsigned_v<IndexT>, "Handle requires unsigned integral IndexT.");
    static_assert(std::is_unsigned_v<GenerationT>, "Handle requires unsigned integral GenerationT.");
    static_assert(!std::is_same_v<TagT, void>, "Handle requires a concrete tag type.");

    using TagType = TagT;
    using IndexType = IndexT;
    using GenerationType = GenerationT;

    /// @brief Invalid index sentinel encoded in this handle type.
    static constexpr IndexT kInvalidIndex = InvalidIndexV;
    /// @brief Invalid generation sentinel encoded in this handle type.
    static constexpr GenerationT kInvalidGeneration = InvalidGenerationV;

    /// @brief Constructs an invalid handle.
    constexpr Handle() noexcept : Index_(InvalidIndexV), Generation_(InvalidGenerationV)
    {
    }

    /**
   * @brief Constructs a handle from explicit index and generation.
   * @param index Slot index value.
   * @param generation Slot generation value.
   */
    constexpr Handle(IndexT index, GenerationT generation) noexcept : Index_(index), Generation_(generation)
    {
    }

    constexpr Handle(const Handle&) noexcept = default;
    constexpr Handle& operator=(const Handle&) noexcept = default;
    constexpr Handle(Handle&&) noexcept = default;
    constexpr Handle& operator=(Handle&&) noexcept = default;
    ~Handle() = default;

    /// @brief Returns a canonical invalid handle.
    [[nodiscard]] static constexpr Handle Invalid() noexcept
    {
        return Handle(InvalidIndexV, InvalidGenerationV);
    }

    /**
   * @brief Constructs from raw parts without validation.
   * @param index Slot index value.
   * @param generation Slot generation value.
   * @return Handle containing raw parts.
   */
    [[nodiscard]] static constexpr Handle FromRawUnchecked(IndexT index, GenerationT generation) noexcept
    {
        return Handle(index, generation);
    }

    /// @brief Returns raw index value.
    [[nodiscard]] constexpr IndexT Index() const noexcept
    {
        return Index_;
    }

    /// @brief Returns raw generation value.
    [[nodiscard]] constexpr GenerationT Generation() const noexcept
    {
        return Generation_;
    }

    /// @brief Returns `true` when both index and generation are not invalid sentinels.
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return Index_ != InvalidIndexV && Generation_ != InvalidGenerationV;
    }

    /// @brief Returns `true` when index or generation equals invalid sentinel.
    [[nodiscard]] constexpr bool IsInvalid() const noexcept
    {
        return !IsValid();
    }

    /// @brief Returns `true` when two handles refer to same index slot.
    [[nodiscard]] constexpr bool SameSlot(Handle other) const noexcept
    {
        return Index_ == other.Index_;
    }

    /// @brief Resets index and generation to invalid sentinels.
    constexpr void Reset() noexcept
    {
        Index_ = InvalidIndexV;
        Generation_ = InvalidGenerationV;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return IsValid();
    }

    [[nodiscard]] constexpr bool operator==(const Handle&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const Handle&) const noexcept = default;

private:
    IndexT Index_;
    GenerationT Generation_;
};

} // namespace zcore

namespace zcore::hash {

/**
 * @brief `zcore::hash` specialization for `zcore::Handle`.
 */
template <typename TagT, typename IndexT, typename GenerationT, IndexT InvalidIndexV, GenerationT InvalidGenerationV>
struct Hash<zcore::Handle<TagT, IndexT, GenerationT, InvalidIndexV, InvalidGenerationV>, void> final {
    [[nodiscard]] constexpr Hash64
    operator()(const zcore::Handle<TagT, IndexT, GenerationT, InvalidIndexV, InvalidGenerationV>& value,
               u64 seed = 0ULL) const noexcept
    {
        const Hash64 indexDigest = Hash<IndexT>{}(value.Index(), seed);
        return Hash<GenerationT>{}(value.Generation(), indexDigest);
    }
};

} // namespace zcore::hash

namespace std {

/**
 * @brief `std::hash` adapter for `zcore::Handle` to support standard unordered containers.
 */
template <typename TagT, typename IndexT, typename GenerationT, IndexT InvalidIndexV, GenerationT InvalidGenerationV>
struct hash<zcore::Handle<TagT, IndexT, GenerationT, InvalidIndexV, InvalidGenerationV>> final {
    [[nodiscard]] std::size_t
    operator()(const zcore::Handle<TagT, IndexT, GenerationT, InvalidIndexV, InvalidGenerationV>& value) const noexcept
    {
        return zcore::hash::DigestToSizeT(
                zcore::hash::Hash<zcore::Handle<TagT, IndexT, GenerationT, InvalidIndexV, InvalidGenerationV>>{}(value));
    }
};

} // namespace std
