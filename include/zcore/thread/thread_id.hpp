/**************************************************************************/
/*  thread_id.hpp                                                         */
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
 * @file include/zcore/thread/thread_id.hpp
 * @brief Strongly typed runtime thread identity token.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/thread_id.hpp>
 * const zcore::ThreadId id = zcore::ThreadId::Current();
 * @endcode
 */

#pragma once

#include <cstddef>
#include <functional>
#include <thread>
#include <zcore/foundation.hpp>
#include <zcore/hash/customization.hpp>

namespace zcore {

/**
 * @brief Runtime thread identity wrapper with explicit invalid sentinel.
 */
class [[nodiscard("ThreadId must be handled explicitly.")]] ThreadId final {
public:
    using ValueType = u64;

    /// @brief Invalid sentinel for `ThreadId`.
    static constexpr ValueType kInvalidValue = 0ULL;

    /// @brief Constructs invalid thread id.
    constexpr ThreadId() noexcept : Value_(kInvalidValue)
    {
    }

    /**
   * @brief Constructs from raw value.
   * @param value Raw thread id token.
   */
    constexpr explicit ThreadId(ValueType value) noexcept : Value_(value)
    {
    }

    constexpr ThreadId(const ThreadId&) noexcept = default;
    constexpr ThreadId& operator=(const ThreadId&) noexcept = default;
    constexpr ThreadId(ThreadId&&) noexcept = default;
    constexpr ThreadId& operator=(ThreadId&&) noexcept = default;
    ~ThreadId() = default;

    /// @brief Returns canonical invalid thread id.
    [[nodiscard]] static constexpr ThreadId Invalid() noexcept
    {
        return ThreadId(kInvalidValue);
    }

    /**
   * @brief Constructs from raw value without validation.
   * @param value Raw thread id token.
   */
    [[nodiscard]] static constexpr ThreadId FromRawUnchecked(ValueType value) noexcept
    {
        return ThreadId(value);
    }

    /**
   * @brief Returns current thread id token.
   *
   * Token derives from `std::thread::id` hash and is valid (non-zero).
   */
    [[nodiscard]] static ThreadId Current() noexcept
    {
        const std::size_t hashed = std::hash<std::thread::id>{}(std::this_thread::get_id());
        const u64 value = static_cast<u64>(hashed);
        return ThreadId(value == kInvalidValue ? 1ULL : value);
    }

    /// @brief Returns raw thread id token.
    [[nodiscard]] constexpr ValueType Raw() const noexcept
    {
        return Value_;
    }

    /// @brief Returns `true` when token is not invalid sentinel.
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return Value_ != kInvalidValue;
    }

    /// @brief Returns `true` when token equals invalid sentinel.
    [[nodiscard]] constexpr bool IsInvalid() const noexcept
    {
        return Value_ == kInvalidValue;
    }

    /// @brief Resets to invalid sentinel.
    constexpr void Reset() noexcept
    {
        Value_ = kInvalidValue;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return IsValid();
    }

    [[nodiscard]] constexpr explicit operator ValueType() const noexcept
    {
        return Value_;
    }

    [[nodiscard]] constexpr bool operator==(const ThreadId&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ThreadId&) const noexcept = default;

private:
    ValueType Value_;
};

} // namespace zcore

namespace zcore::hash {

/**
 * @brief `zcore::hash` specialization for `zcore::ThreadId`.
 */
template <>
struct Hash<zcore::ThreadId, void> final {
    [[nodiscard]] constexpr Hash64 operator()(const zcore::ThreadId& value, u64 seed = 0ULL) const noexcept
    {
        return Hash<zcore::ThreadId::ValueType>{}(value.Raw(), seed);
    }
};

} // namespace zcore::hash

namespace std {

/**
 * @brief `std::hash` adapter for `zcore::ThreadId`.
 */
template <>
struct hash<zcore::ThreadId> final {
    [[nodiscard]] std::size_t operator()(const zcore::ThreadId& value) const noexcept
    {
        return zcore::hash::DigestToSizeT(zcore::hash::Hash<zcore::ThreadId>{}(value));
    }
};

} // namespace std
