/**************************************************************************/
/*  mpsc_ring_buffer.hpp                                                  */
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
 * @file include/zcore/thread/mpsc_ring_buffer.hpp
 * @brief Multi-producer/single-consumer fixed-capacity ring buffer.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/mpsc_ring_buffer.hpp>
 * @endcode
 */

#pragma once

#include <array>
#include <atomic>
#include <type_traits>
#include <utility>
#include <zcore/foundation.hpp>
#include <zcore/option.hpp>
#include <zcore/thread/detail/ring_buffer_storage.hpp>

namespace zcore {

/**
 * @brief Lock-free MPSC fixed-capacity queue for deterministic runtime queues.
 *
 * This type is thread-safe for many producer threads and exactly one consumer
 * thread.
 */
template <typename ValueT, usize CapacityV>
class [[nodiscard("MpscRingBuffer must be handled explicitly.")]] MpscRingBuffer final {
public:
    static_assert(std::is_move_constructible_v<ValueT>, "MpscRingBuffer requires move-constructible value type.");

    using ValueType = ValueT;
    using SizeType = usize;
    static constexpr usize kCapacity = CapacityV;

    MpscRingBuffer() noexcept
    {
        for (usize index = 0; index < kCapacity; ++index) {
            Sequence_[index].store(index, std::memory_order_relaxed);
        }
    }

    ~MpscRingBuffer()
    {
        Clear();
    }

    MpscRingBuffer(const MpscRingBuffer&) = delete;
    MpscRingBuffer& operator=(const MpscRingBuffer&) = delete;
    MpscRingBuffer(MpscRingBuffer&&) = delete;
    MpscRingBuffer& operator=(MpscRingBuffer&&) = delete;

    [[nodiscard]] static constexpr usize Capacity() noexcept
    {
        return kCapacity;
    }

    [[nodiscard]] usize SizeApprox() const noexcept
    {
        const usize dequeue = DequeuePos_.load(std::memory_order_acquire);
        const usize enqueue = EnqueuePos_.load(std::memory_order_acquire);
        return enqueue - dequeue;
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
        const usize dequeue = DequeuePos_.load(std::memory_order_relaxed);
        const usize slot = detail::RingBufferSlotIndex<kCapacity>(dequeue);
        const usize sequence = Sequence_[slot].load(std::memory_order_acquire);
        if (sequence != (dequeue + 1)) {
            return None;
        }

        Option<ValueT> value(std::move(Storage_.RefAt(slot)));
        Storage_.DestroyAt(slot);
        DequeuePos_.store(dequeue + 1, std::memory_order_relaxed);
        Sequence_[slot].store(dequeue + kCapacity, std::memory_order_release);
        return value;
    }

    void Clear() noexcept
    {
        usize dequeue = DequeuePos_.load(std::memory_order_relaxed);
        const usize enqueue = EnqueuePos_.load(std::memory_order_acquire);

        while (dequeue < enqueue) {
            const usize slot = detail::RingBufferSlotIndex<kCapacity>(dequeue);
            const usize sequence = Sequence_[slot].load(std::memory_order_acquire);
            if (sequence != (dequeue + 1U)) {
                break;
            }

            Storage_.DestroyAt(slot);
            Sequence_[slot].store(dequeue + kCapacity, std::memory_order_release);
            ++dequeue;
        }

        DequeuePos_.store(dequeue, std::memory_order_relaxed);
    }

private:
    template <typename... ArgsT>
        requires(std::is_constructible_v<ValueT, ArgsT && ...>)
    [[nodiscard]] bool TryEmplaceImpl(ArgsT&&... args)
    {
        usize enqueue = EnqueuePos_.load(std::memory_order_relaxed);
        for (;;) {
            const usize slot = detail::RingBufferSlotIndex<kCapacity>(enqueue);
            const usize sequence = Sequence_[slot].load(std::memory_order_acquire);
            if (sequence == enqueue) {
                if (EnqueuePos_.compare_exchange_weak(enqueue, enqueue + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
                    Storage_.ConstructAt(slot, std::forward<ArgsT>(args)...);
                    Sequence_[slot].store(enqueue + 1, std::memory_order_release);
                    return true;
                }
                continue;
            }

            if (sequence < enqueue) {
                return false;
            }

            enqueue = EnqueuePos_.load(std::memory_order_relaxed);
        }
    }

    detail::RingBufferStorage<ValueT, kCapacity> Storage_{};
    std::array<std::atomic<usize>, kCapacity> Sequence_{};
    std::atomic<usize> EnqueuePos_{0};
    std::atomic<usize> DequeuePos_{0};
};

} // namespace zcore
