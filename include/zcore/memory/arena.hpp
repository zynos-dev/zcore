/**************************************************************************/
/*  memory/arena.hpp                                                      */
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
 * @file include/zcore/memory/arena.hpp
 * @brief Bump-pointer arena allocator over caller-owned storage.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/arena.hpp>
 * std::array<zcore::Byte, 1024> storage{};
 * zcore::Arena arena(storage);
 * @endcode
 */

#pragma once

#include <array>
#include <cstdint>
#include <zcore/memory/allocator.hpp>

namespace zcore {

/**
 * @brief Fixed-capacity bump-pointer allocator over external storage.
 *
 * `Arena` never allocates from the system. Successful allocations are carved
 * from the backing storage and released in bulk via `Reset()`.
 */
class Arena final : public Allocator {
public:
    /// @brief Constructs an empty arena with zero capacity.
    constexpr Arena() noexcept : Storage_(), Offset_(0U)
    {
    }

    /**
   * @brief Constructs arena over mutable backing storage.
   * @param storage Backing memory span.
   */
    constexpr explicit Arena(ByteSpanMut storage) noexcept : Storage_(storage), Offset_(0U)
    {
    }

    /**
   * @brief Constructs arena over mutable byte array backing storage.
   * @tparam CapacityV Array size in bytes.
   * @param storage Backing memory.
   */
    template <usize CapacityV>
    constexpr explicit Arena(std::array<Byte, CapacityV>& storage) noexcept : Storage_(storage), Offset_(0U)
    {
    }

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    constexpr Arena(Arena&& other) noexcept : Storage_(other.Storage_), Offset_(other.Offset_)
    {
        other.Storage_ = ByteSpanMut();
        other.Offset_ = 0U;
    }

    constexpr Arena& operator=(Arena&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        Storage_ = other.Storage_;
        Offset_ = other.Offset_;
        other.Storage_ = ByteSpanMut();
        other.Offset_ = 0U;
        return *this;
    }
    ~Arena() override = default;

    /// @brief Returns total backing capacity in bytes.
    [[nodiscard]] constexpr usize Capacity() const noexcept
    {
        return Storage_.Size();
    }

    /// @brief Returns bytes currently reserved by arena allocations.
    [[nodiscard]] constexpr usize Used() const noexcept
    {
        return Offset_;
    }

    /// @brief Returns bytes remaining for future allocations.
    [[nodiscard]] constexpr usize Remaining() const noexcept
    {
        return Capacity() - Offset_;
    }

    /// @brief Returns whether no bytes are currently reserved.
    [[nodiscard]] constexpr bool IsEmpty() const noexcept
    {
        return Offset_ == 0U;
    }

    /// @brief Resets allocation cursor to beginning of backing storage.
    constexpr void Reset() noexcept
    {
        Offset_ = 0U;
    }

    /**
   * @brief Returns whether pointer belongs to arena backing storage range.
   */
    [[nodiscard]] constexpr bool Owns(const Byte* pointer) const noexcept
    {
        if (pointer == nullptr || Capacity() == 0U || Storage_.Data() == nullptr) {
            return false;
        }

        const std::uintptr_t baseAddress = reinterpret_cast<std::uintptr_t>(Storage_.Data());
        const std::uintptr_t pointerAddress = reinterpret_cast<std::uintptr_t>(pointer);
        if (pointerAddress < baseAddress) {
            return false;
        }
        return (pointerAddress - baseAddress) < Capacity();
    }

    /**
   * @brief Allocates from arena backing storage.
   * @param request Requested size/alignment.
   * @return Allocation descriptor or allocator-domain error.
   */
    [[nodiscard]] Result<Allocation, Error> Allocate(AllocationRequest request) noexcept override
    {
        const Status requestStatus = ValidateAllocationRequest(request);
        if (requestStatus.HasError()) {
            return Result<Allocation, Error>::Failure(requestStatus.Error());
        }

        if (request.size == 0U) {
            return Result<Allocation, Error>::Success(Allocation::Empty());
        }

        if (Storage_.Data() == nullptr || Capacity() == 0U) {
            return Result<Allocation, Error>::Failure(
                    MakeAllocatorError(AllocatorErrorCode::OUT_OF_MEMORY, "allocate", "arena has no backing storage"));
        }

        const std::uintptr_t baseAddress = reinterpret_cast<std::uintptr_t>(Storage_.Data());
        const std::uintptr_t currentAddress = baseAddress + Offset_;
        const usize misalignment = static_cast<usize>(currentAddress % request.alignment);
        const usize padding = misalignment == 0U ? 0U : (request.alignment - misalignment);

        if (padding > Remaining()) {
            return Result<Allocation, Error>::Failure(
                    MakeAllocatorError(AllocatorErrorCode::OUT_OF_MEMORY, "allocate", "arena capacity exhausted by alignment"));
        }

        const usize startOffset = Offset_ + padding;
        if (request.size > (Capacity() - startOffset)) {
            return Result<Allocation, Error>::Failure(
                    MakeAllocatorError(AllocatorErrorCode::OUT_OF_MEMORY, "allocate", "arena capacity exhausted"));
        }

        Byte* const data = Storage_.Data() + startOffset;
        Offset_ = startOffset + request.size;
        return Result<Allocation, Error>::Success(Allocation{
                .data = data,
                .size = request.size,
                .alignment = request.alignment,
        });
    }

    /**
   * @brief Validates descriptor and accepts arena-owned blocks as no-op deallocation.
   * @param allocation Allocation descriptor.
   * @return Success for empty/owned blocks, error for malformed or foreign blocks.
   */
    [[nodiscard]] Status Deallocate(Allocation allocation) noexcept override
    {
        Status allocationStatus = ValidateAllocation(allocation);
        if (allocationStatus.HasError()) {
            return allocationStatus;
        }

        if (allocation.IsEmpty()) {
            return OkStatus();
        }

        if (!Owns(allocation.data)) {
            return ErrorStatus(MakeAllocatorError(AllocatorErrorCode::INVALID_ALLOCATION,
                                                  "deallocate",
                                                  "allocation does not belong to arena"));
        }

        return OkStatus();
    }

private:
    ByteSpanMut Storage_;
    usize Offset_;
};

} // namespace zcore
