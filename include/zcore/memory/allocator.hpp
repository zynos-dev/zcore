/**************************************************************************/
/*  memory/allocator.hpp                                                  */
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
 * @file include/zcore/memory/allocator.hpp
 * @brief Fallible allocator interface and allocation contracts.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/allocator.hpp>
 * zcore::AllocationRequest request{.size = 64U, .alignment = 16U};
 * @endcode
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <zcore/error.hpp>
#include <zcore/foundation.hpp>
#include <zcore/result.hpp>
#include <zcore/slice.hpp>
#include <zcore/status.hpp>

namespace zcore {

/**
 * @brief Allocator-domain error codes for allocation interfaces.
 */
// NOLINTNEXTLINE(performance-enum-size): stable error-code ABI uses i32 payloads.
enum class AllocatorErrorCode : i32 {
    /// @brief Allocation request failed validation.
    INVALID_REQUEST = 1,
    /// @brief Allocation failed due to insufficient memory/resources.
    OUT_OF_MEMORY = 2,
    /// @brief Deallocation/allocation shape failed validation.
    INVALID_ALLOCATION = 3,
    /// @brief Requested operation is not supported by allocator implementation.
    UNSUPPORTED_OPERATION = 4,
};

/// @brief Built-in allocator error domain identifier.
inline constexpr ErrorDomain kAllocatorErrorDomain{
        .id = 2U,
        .name = "allocator",
};

/**
 * @brief Constructs allocator-domain error payload.
 * @param code Allocator error code.
 * @param operation Operation identifier.
 * @param message Human-readable message.
 * @param file Source file path.
 * @param line Source line.
 * @return Structured allocator error.
 */
[[nodiscard]] constexpr Error MakeAllocatorError(
        AllocatorErrorCode code, const char* operation, const char* message, const char* file = "", u32 line = 0U) noexcept
{
    return MakeError(kAllocatorErrorDomain, static_cast<i32>(code), MakeErrorContext("allocator", operation, message, file, line));
}

/**
 * @brief Returns `true` when value is a non-zero power of two.
 */
[[nodiscard]] constexpr bool IsPowerOfTwo(usize value) noexcept
{
    return value != 0U && (value & (value - 1U)) == 0U;
}

/**
 * @brief Returns `true` when alignment is allocator-valid.
 */
[[nodiscard]] constexpr bool IsValidAllocationAlignment(usize alignment) noexcept
{
    return IsPowerOfTwo(alignment);
}

/**
 * @brief Returns `true` when pointer address satisfies alignment.
 */
[[nodiscard]] inline bool IsAddressAligned(const void* pointer, usize alignment) noexcept
{
    if (pointer == nullptr) {
        return false;
    }
    if (!IsValidAllocationAlignment(alignment)) {
        return false;
    }
    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(pointer);
    return (address % alignment) == 0U;
}

/**
 * @brief Allocation request contract.
 */
struct AllocationRequest final {
    /// @brief Requested byte size.
    usize size;
    /// @brief Requested power-of-two byte alignment.
    usize alignment;

    /// @brief Returns request using default ABI alignment.
    [[nodiscard]] static constexpr AllocationRequest WithDefaultAlignment(usize sizeBytes) noexcept
    {
        return AllocationRequest{
                .size = sizeBytes,
                .alignment = alignof(std::max_align_t),
        };
    }

    /// @brief Returns `true` when request alignment is valid.
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return IsValidAllocationAlignment(alignment);
    }
};

/**
 * @brief Allocated memory block descriptor.
 */
struct Allocation final {
    /// @brief Pointer to allocated bytes (`nullptr` only when `size == 0`).
    Byte* data;
    /// @brief Byte size of allocation.
    usize size;
    /// @brief Byte alignment of allocation.
    usize alignment;

    /// @brief Returns canonical empty allocation descriptor.
    [[nodiscard]] static constexpr Allocation Empty() noexcept
    {
        return Allocation{
                .data = nullptr,
                .size = 0U,
                .alignment = alignof(std::max_align_t),
        };
    }

    /// @brief Returns `true` when no bytes are allocated.
    [[nodiscard]] constexpr bool IsEmpty() const noexcept
    {
        return size == 0U;
    }

    /**
   * @brief Returns `true` when descriptor shape is valid.
   *
   * Valid non-empty allocation requires non-null aligned pointer and valid alignment.
   * Valid empty allocation requires null pointer and valid alignment.
   */
    [[nodiscard]] inline bool IsValid() const noexcept
    {
        if (!IsValidAllocationAlignment(alignment)) {
            return false;
        }
        if (size == 0U) {
            return data == nullptr;
        }
        return IsAddressAligned(data, alignment);
    }

    /// @brief Returns mutable byte span view over allocated memory.
    [[nodiscard]] constexpr ByteSpanMut AsBytes() const noexcept
    {
        if (size == 0U || data == nullptr) {
            return ByteSpanMut();
        }
        return ByteSpanMut::FromRawUnchecked(data, size);
    }

    [[nodiscard]] inline explicit operator bool() const noexcept
    {
        return IsValid();
    }
};

/**
 * @brief Validates allocator request shape.
 * @param request Request to validate.
 * @return Success on valid request, allocator-domain error on invalid request.
 */
[[nodiscard]] constexpr Status ValidateAllocationRequest(AllocationRequest request) noexcept
{
    if (!request.IsValid()) {
        return ErrorStatus(
                MakeAllocatorError(AllocatorErrorCode::INVALID_REQUEST, "allocate", "alignment must be a non-zero power-of-two"));
    }
    return OkStatus();
}

/**
 * @brief Validates allocation descriptor shape.
 * @param allocation Allocation descriptor to validate.
 * @return Success on valid shape, allocator-domain error on invalid descriptor.
 */
[[nodiscard]] inline Status ValidateAllocation(Allocation allocation) noexcept
{
    if (!allocation.IsValid()) {
        return ErrorStatus(MakeAllocatorError(AllocatorErrorCode::INVALID_ALLOCATION,
                                              "deallocate",
                                              "allocation descriptor must be aligned and non-null when size is non-zero"));
    }
    return OkStatus();
}

/**
 * @brief Fallible allocator interface contract.
 */
class Allocator {
public:
    Allocator() = default;
    Allocator(const Allocator&) = default;
    Allocator& operator=(const Allocator&) = default;
    Allocator(Allocator&&) = default;
    Allocator& operator=(Allocator&&) = default;
    virtual ~Allocator() = default;

    /**
   * @brief Allocates bytes according to request.
   * @param request Allocation request.
   * @return Allocated descriptor or error.
   */
    [[nodiscard]] virtual Result<Allocation, Error> Allocate(AllocationRequest request) noexcept = 0;

    /**
   * @brief Deallocates previously allocated descriptor.
   * @param allocation Allocation descriptor.
   * @return Success or error.
   */
    [[nodiscard]] virtual Status Deallocate(Allocation allocation) noexcept = 0;

    /**
   * @brief Convenience allocation overload.
   * @param size Byte size.
   * @param alignment Byte alignment.
   * @return Allocated descriptor or error.
   */
    [[nodiscard]] Result<Allocation, Error> AllocateBytes(usize size, usize alignment = alignof(std::max_align_t)) noexcept
    {
        return Allocate(AllocationRequest{
                .size = size,
                .alignment = alignment,
        });
    }
};

} // namespace zcore
