/**************************************************************************/
/*  fixed_bit_set.hpp                                                     */
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
 * @file include/zcore/container/fixed_bit_set.hpp
 * @brief Deterministic fixed-size inline bitset.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/fixed_bit_set.hpp>
 * zcore::FixedBitSet<128> bits;
 * bits.Set(5U);
 * @endcode
 */

#pragma once

#include <array>
#include <bit>
#include <zcore/contract_violation.hpp>
#include <zcore/foundation.hpp>
#include <zcore/option.hpp>

namespace zcore {

/**
 * @brief Fixed-size bitset backed by inline machine words.
 *
 * @tparam BitCountV Total bit count.
 */
template <usize BitCountV>
class [[nodiscard("FixedBitSet must be handled explicitly.")]] FixedBitSet final {
public:
    using SizeType = usize;
    using WordType = u64;

    static constexpr usize kBitCount = BitCountV;
    static constexpr usize kBitsPerWord = sizeof(WordType) * 8U;
    static constexpr usize kWordCount = kBitCount == 0U ? 0U : ((kBitCount + (kBitsPerWord - 1U)) / kBitsPerWord);

    constexpr FixedBitSet() noexcept = default;
    constexpr FixedBitSet(const FixedBitSet&) noexcept = default;
    constexpr FixedBitSet& operator=(const FixedBitSet&) noexcept = default;
    constexpr FixedBitSet(FixedBitSet&&) noexcept = default;
    constexpr FixedBitSet& operator=(FixedBitSet&&) noexcept = default;
    ~FixedBitSet() = default;

    [[nodiscard]] static constexpr usize BitCount() noexcept
    {
        return kBitCount;
    }

    [[nodiscard]] static constexpr usize WordCount() noexcept
    {
        return kWordCount;
    }

    [[nodiscard]] constexpr bool Empty() const noexcept
    {
        for (usize index = 0U; index < kWordCount; ++index) {
            if (Words_[index] != 0U) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] constexpr bool Any() const noexcept
    {
        return !Empty();
    }

    [[nodiscard]] constexpr bool NoneSet() const noexcept
    {
        return Empty();
    }

    [[nodiscard]] constexpr bool All() const noexcept
    {
        if constexpr (kBitCount == 0U) {
            return true;
        }

        if constexpr (kWordCount > 1U) {
            for (usize index = 0U; index < (kWordCount - 1U); ++index) {
                if (Words_[index] != ~WordType{0U}) {
                    return false;
                }
            }
        }
        return Words_[kWordCount - 1U] == LastWordMask();
    }

    [[nodiscard]] constexpr WordType RawWord(usize index) const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(index < kWordCount,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::FixedBitSet::RawWord() word index out of bounds");
        return Words_[index];
    }

    [[nodiscard]] constexpr Option<WordType> TryRawWord(usize index) const noexcept
    {
        if (index >= kWordCount) {
            return None;
        }
        return Option<WordType>(Words_[index]);
    }

    [[nodiscard]] constexpr bool Test(usize bitIndex) const noexcept
    {
        RequireBitInRange(bitIndex, "zcore::FixedBitSet::Test() bit index out of bounds");
        return TestUnchecked(bitIndex);
    }

    [[nodiscard]] constexpr Option<bool> TryTest(usize bitIndex) const noexcept
    {
        if (bitIndex >= kBitCount) {
            return None;
        }
        return Option<bool>(TestUnchecked(bitIndex));
    }

    [[nodiscard]] constexpr bool TrySet(usize bitIndex) noexcept
    {
        if (bitIndex >= kBitCount) {
            return false;
        }
        SetUnchecked(bitIndex);
        return true;
    }

    [[nodiscard]] constexpr bool TryClear(usize bitIndex) noexcept
    {
        if (bitIndex >= kBitCount) {
            return false;
        }
        ClearUnchecked(bitIndex);
        return true;
    }

    [[nodiscard]] constexpr bool TryToggle(usize bitIndex) noexcept
    {
        if (bitIndex >= kBitCount) {
            return false;
        }
        ToggleUnchecked(bitIndex);
        return true;
    }

    [[nodiscard]] constexpr bool TryAssign(usize bitIndex, bool enabled) noexcept
    {
        if (bitIndex >= kBitCount) {
            return false;
        }
        if (enabled) {
            SetUnchecked(bitIndex);
        }
        else {
            ClearUnchecked(bitIndex);
        }
        return true;
    }

    constexpr void Set(usize bitIndex) noexcept
    {
        RequireBitInRange(bitIndex, "zcore::FixedBitSet::Set() bit index out of bounds");
        SetUnchecked(bitIndex);
    }

    constexpr void Clear(usize bitIndex) noexcept
    {
        RequireBitInRange(bitIndex, "zcore::FixedBitSet::Clear() bit index out of bounds");
        ClearUnchecked(bitIndex);
    }

    constexpr void Toggle(usize bitIndex) noexcept
    {
        RequireBitInRange(bitIndex, "zcore::FixedBitSet::Toggle() bit index out of bounds");
        ToggleUnchecked(bitIndex);
    }

    constexpr void Assign(usize bitIndex, bool enabled) noexcept
    {
        RequireBitInRange(bitIndex, "zcore::FixedBitSet::Assign() bit index out of bounds");
        if (enabled) {
            SetUnchecked(bitIndex);
        }
        else {
            ClearUnchecked(bitIndex);
        }
    }

    constexpr void SetAll() noexcept
    {
        for (usize index = 0U; index < kWordCount; ++index) {
            Words_[index] = ~WordType{0U};
        }
        ApplyTailMask();
    }

    constexpr void ClearAll() noexcept
    {
        for (usize index = 0U; index < kWordCount; ++index) {
            Words_[index] = 0U;
        }
    }

    constexpr void FlipAll() noexcept
    {
        for (usize index = 0U; index < kWordCount; ++index) {
            Words_[index] = ~Words_[index];
        }
        ApplyTailMask();
    }

    [[nodiscard]] constexpr usize CountSet() const noexcept
    {
        usize count = 0U;
        for (usize index = 0U; index < kWordCount; ++index) {
            count += static_cast<usize>(std::popcount(Words_[index]));
        }
        return count;
    }

    constexpr FixedBitSet& operator|=(const FixedBitSet& other) noexcept
    {
        for (usize index = 0U; index < kWordCount; ++index) {
            Words_[index] |= other.Words_[index];
        }
        return *this;
    }

    constexpr FixedBitSet& operator&=(const FixedBitSet& other) noexcept
    {
        for (usize index = 0U; index < kWordCount; ++index) {
            Words_[index] &= other.Words_[index];
        }
        return *this;
    }

    constexpr FixedBitSet& operator^=(const FixedBitSet& other) noexcept
    {
        for (usize index = 0U; index < kWordCount; ++index) {
            Words_[index] ^= other.Words_[index];
        }
        ApplyTailMask();
        return *this;
    }

    [[nodiscard]] constexpr bool operator==(const FixedBitSet&) const noexcept = default;

    [[nodiscard]] friend constexpr FixedBitSet operator~(FixedBitSet value) noexcept
    {
        value.FlipAll();
        return value;
    }

    [[nodiscard]] friend constexpr FixedBitSet operator|(FixedBitSet lhs, const FixedBitSet& rhs) noexcept
    {
        lhs |= rhs;
        return lhs;
    }

    [[nodiscard]] friend constexpr FixedBitSet operator&(FixedBitSet lhs, const FixedBitSet& rhs) noexcept
    {
        lhs &= rhs;
        return lhs;
    }

    [[nodiscard]] friend constexpr FixedBitSet operator^(FixedBitSet lhs, const FixedBitSet& rhs) noexcept
    {
        lhs ^= rhs;
        return lhs;
    }

private:
    [[nodiscard]] static constexpr usize WordIndex(usize bitIndex) noexcept
    {
        return bitIndex / kBitsPerWord;
    }

    [[nodiscard]] static constexpr WordType BitMask(usize bitIndex) noexcept
    {
        return WordType{1U} << (bitIndex % kBitsPerWord);
    }

    [[nodiscard]] static constexpr WordType LastWordMask() noexcept
    {
        if constexpr (kBitCount == 0U) {
            return WordType{0U};
        }
        else {
            const usize usedBits = ((kBitCount - 1U) % kBitsPerWord) + 1U;
            if (usedBits == kBitsPerWord) {
                return ~WordType{0U};
            }
            return (WordType{1U} << usedBits) - 1U;
        }
    }

    constexpr void ApplyTailMask() noexcept
    {
        if constexpr (kWordCount > 0U) {
            Words_[kWordCount - 1U] &= LastWordMask();
        }
    }

    constexpr void RequireBitInRange(usize bitIndex, const char* message) const noexcept
    {
        ZCORE_CONTRACT_REQUIRE(bitIndex < kBitCount, detail::ContractViolationCode::PRECONDITION, message);
    }

    constexpr void SetUnchecked(usize bitIndex) noexcept
    {
        Words_[WordIndex(bitIndex)] |= BitMask(bitIndex);
    }

    constexpr void ClearUnchecked(usize bitIndex) noexcept
    {
        Words_[WordIndex(bitIndex)] &= static_cast<WordType>(~BitMask(bitIndex));
    }

    constexpr void ToggleUnchecked(usize bitIndex) noexcept
    {
        Words_[WordIndex(bitIndex)] ^= BitMask(bitIndex);
    }

    [[nodiscard]] constexpr bool TestUnchecked(usize bitIndex) const noexcept
    {
        return (Words_[WordIndex(bitIndex)] & BitMask(bitIndex)) != 0U;
    }

    std::array<WordType, kWordCount> Words_{};
};

} // namespace zcore
