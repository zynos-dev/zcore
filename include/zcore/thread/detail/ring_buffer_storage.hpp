/**************************************************************************/
/*  ring_buffer_storage.hpp                                               */
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
 * @file include/zcore/thread/detail/ring_buffer_storage.hpp
 * @brief Shared fixed-capacity slot storage for lock-free ring buffers.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/spsc_ring_buffer.hpp>
 * #include <zcore/mpsc_ring_buffer.hpp>
 * @endcode
 */

#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <utility>
#include <zcore/foundation.hpp>
#include <zcore/type_constraints.hpp>

namespace zcore::detail {

template <usize CapacityV>
[[nodiscard]] constexpr usize RingBufferSlotIndex(const usize cursor) noexcept
{
    static_assert(CapacityV > 0, "Ring buffer capacity must be greater than zero.");
    return cursor % CapacityV;
}

template <typename ValueT, usize CapacityV>
class RingBufferStorage final {
public:
    static_assert(CapacityV > 0, "Ring buffer capacity must be greater than zero.");
    ZCORE_STATIC_REQUIRE(constraints::MutableValueObjectType<ValueT>,
                         "Ring buffer requires mutable non-reference non-void object value type.");

    using ValueType = ValueT;
    static constexpr usize kCapacity = CapacityV;

    RingBufferStorage() noexcept = default;
    ~RingBufferStorage() = default;
    RingBufferStorage(const RingBufferStorage&) = delete;
    RingBufferStorage& operator=(const RingBufferStorage&) = delete;
    RingBufferStorage(RingBufferStorage&&) = delete;
    RingBufferStorage& operator=(RingBufferStorage&&) = delete;

    template <typename... ArgsT>
    void ConstructAt(const usize slot, ArgsT&&... args)
    {
        std::construct_at(PtrAt(slot), std::forward<ArgsT>(args)...);
    }

    void DestroyAt(const usize slot) noexcept
    {
        std::destroy_at(PtrAt(slot));
    }

    [[nodiscard]] ValueT& RefAt(const usize slot) noexcept
    {
        return *PtrAt(slot);
    }

    [[nodiscard]] const ValueT& RefAt(const usize slot) const noexcept
    {
        return *PtrAt(slot);
    }

private:
    [[nodiscard]] ValueT* PtrAt(const usize slot) noexcept
    {
        return std::launder(reinterpret_cast<ValueT*>(Storage_ + (slot * sizeof(ValueT))));
    }

    [[nodiscard]] const ValueT* PtrAt(const usize slot) const noexcept
    {
        return std::launder(reinterpret_cast<const ValueT*>(Storage_ + (slot * sizeof(ValueT))));
    }

    alignas(ValueT) std::byte Storage_[sizeof(ValueT) * CapacityV];
};

} // namespace zcore::detail
