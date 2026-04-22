/**************************************************************************/
/*  spsc_ring_buffer.hpp                                                  */
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
 * @file include/zcore/thread/spsc_ring_buffer.hpp
 * @brief Single-producer/single-consumer fixed-capacity ring buffer.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/spsc_ring_buffer.hpp>
 * @endcode
 */

#pragma once

#include <atomic>
#include <type_traits>
#include <utility>
#include <zcore/foundation.hpp>
#include <zcore/option.hpp>
#include <zcore/thread/detail/ring_buffer_storage.hpp>

namespace zcore {

/**
 * @brief Lock-free SPSC fixed-capacity queue for deterministic hot paths.
 *
 * This type is thread-safe only for one producer thread and one consumer
 * thread. Other usage patterns are unsupported.
 */
template <typename ValueT, usize CapacityV>
class [[nodiscard("SpscRingBuffer must be handled explicitly.")]] SpscRingBuffer final {
public:
    static_assert(std::is_move_constructible_v<ValueT>, "SpscRingBuffer requires move-constructible value type.");

    using ValueType = ValueT;
    using SizeType = usize;
    static constexpr usize kCapacity = CapacityV;

    SpscRingBuffer() noexcept = default;
    ~SpscRingBuffer()
    {
        Clear();
    }

    SpscRingBuffer(const SpscRingBuffer&) = delete;
    SpscRingBuffer& operator=(const SpscRingBuffer&) = delete;
    SpscRingBuffer(SpscRingBuffer&&) = delete;
    SpscRingBuffer& operator=(SpscRingBuffer&&) = delete;

    [[nodiscard]] static constexpr usize Capacity() noexcept
    {
        return kCapacity;
    }

    [[nodiscard]] usize SizeApprox() const noexcept
    {
        const usize head = Head_.load(std::memory_order_acquire);
        const usize tail = Tail_.load(std::memory_order_acquire);
        return tail - head;
    }

    [[nodiscard]] bool Empty() const noexcept
    {
        return SizeApprox() == 0;
    }

    [[nodiscard]] bool Full() const noexcept
    {
        return SizeApprox() >= kCapacity;
    }

    [[nodiscard]] bool TryPush(const ValueT& value)
        requires(std::is_copy_constructible_v<ValueT>)
    {
        return TryEmplaceImpl(value);
    }

    [[nodiscard]] bool TryPush(ValueT&& value)
        requires(std::is_move_constructible_v<ValueT>)
    {
        return TryEmplaceImpl(std::move(value));
    }

    template <typename... ArgsT>
        requires(std::is_constructible_v<ValueT, ArgsT && ...>)
    [[nodiscard]] bool TryEmplace(ArgsT&&... args)
    {
        return TryEmplaceImpl(std::forward<ArgsT>(args)...);
    }

    [[nodiscard]] Option<ValueT> TryPop()
        requires(std::is_move_constructible_v<ValueT>)
    {
        const usize head = Head_.load(std::memory_order_relaxed);
        const usize tail = Tail_.load(std::memory_order_acquire);
        if (head == tail) {
            return None;
        }

        const usize slot = detail::RingBufferSlotIndex<kCapacity>(head);
        Option<ValueT> value(std::move(Storage_.RefAt(slot)));
        Storage_.DestroyAt(slot);
        Head_.store(head + 1, std::memory_order_release);
        return value;
    }

    void Clear() noexcept
    {
        usize head = Head_.load(std::memory_order_relaxed);
        const usize tail = Tail_.load(std::memory_order_relaxed);
        while (head != tail) {
            Storage_.DestroyAt(detail::RingBufferSlotIndex<kCapacity>(head));
            ++head;
        }
        Head_.store(tail, std::memory_order_relaxed);
    }

private:
    template <typename... ArgsT>
        requires(std::is_constructible_v<ValueT, ArgsT && ...>)
    [[nodiscard]] bool TryEmplaceImpl(ArgsT&&... args)
    {
        const usize tail = Tail_.load(std::memory_order_relaxed);
        const usize head = Head_.load(std::memory_order_acquire);
        if ((tail - head) >= kCapacity) {
            return false;
        }

        const usize slot = detail::RingBufferSlotIndex<kCapacity>(tail);
        Storage_.ConstructAt(slot, std::forward<ArgsT>(args)...);
        Tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    detail::RingBufferStorage<ValueT, kCapacity> Storage_{};
    std::atomic<usize> Head_{0};
    std::atomic<usize> Tail_{0};
};

} // namespace zcore
