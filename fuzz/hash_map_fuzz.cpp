/**************************************************************************/
/*  hash_map_fuzz.cpp                                                     */
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
 * @file fuzz/hash_map_fuzz.cpp
 * @brief Local libFuzzer harness for HashMap state-machine invariants.
 */

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <unordered_map>
#include <zcore/hash_map.hpp>

namespace {

class FuzzCursor final {
public:
    FuzzCursor(const std::uint8_t* data, std::size_t size) noexcept : Data_(data), Size_(size)
    {
    }

    [[nodiscard]] std::size_t Remaining() const noexcept
    {
        return Position_ < Size_ ? Size_ - Position_ : 0U;
    }

    [[nodiscard]] std::uint8_t TakeByte() noexcept
    {
        if (Position_ >= Size_) {
            return 0U;
        }
        return Data_[Position_++];
    }

    [[nodiscard]] std::uint32_t TakeU32() noexcept
    {
        std::uint32_t value = 0U;
        for (std::uint32_t index = 0U; index < 4U; ++index) {
            value |= static_cast<std::uint32_t>(TakeByte()) << (index * 8U);
        }
        return value;
    }

private:
    const std::uint8_t* Data_{nullptr};
    std::size_t Size_{0U};
    std::size_t Position_{0U};
};

[[noreturn]] void FuzzAbort() noexcept
{
    std::abort();
}

class FuzzAllocator final : public zcore::Allocator {
public:
    explicit FuzzAllocator(zcore::usize failEvery) noexcept : FailEvery_(failEvery)
    {
    }

    [[nodiscard]] zcore::Result<zcore::Allocation, zcore::Error> Allocate(zcore::AllocationRequest request) noexcept override
    {
        const zcore::Status requestStatus = zcore::ValidateAllocationRequest(request);
        if (requestStatus.HasError()) {
            return zcore::Result<zcore::Allocation, zcore::Error>::Failure(requestStatus.Error());
        }
        if (request.size == 0U) {
            return zcore::Result<zcore::Allocation, zcore::Error>::Success(zcore::Allocation::Empty());
        }

        ++AllocateCalls_;
        if (FailEvery_ > 0U && (AllocateCalls_ % FailEvery_) == 0U) {
            return zcore::Result<zcore::Allocation, zcore::Error>::Failure(
                    zcore::MakeAllocatorError(zcore::AllocatorErrorCode::OUT_OF_MEMORY,
                                              "allocate",
                                              "fuzz allocator injected failure"));
        }

        void* const raw = ::operator new(request.size, static_cast<std::align_val_t>(request.alignment), std::nothrow);
        if (raw == nullptr) {
            return zcore::Result<zcore::Allocation, zcore::Error>::Failure(
                    zcore::MakeAllocatorError(zcore::AllocatorErrorCode::OUT_OF_MEMORY,
                                              "allocate",
                                              "system allocator returned null"));
        }
        return zcore::Result<zcore::Allocation, zcore::Error>::Success(zcore::Allocation{
                .data = static_cast<zcore::Byte*>(raw),
                .size = request.size,
                .alignment = request.alignment,
        });
    }

    [[nodiscard]] zcore::Status Deallocate(zcore::Allocation allocation) noexcept override
    {
        zcore::Status allocationStatus = zcore::ValidateAllocation(allocation);
        if (allocationStatus.HasError()) {
            return allocationStatus;
        }
        if (allocation.IsEmpty()) {
            return zcore::OkStatus();
        }
        ::operator delete(static_cast<void*>(allocation.data), static_cast<std::align_val_t>(allocation.alignment));
        return zcore::OkStatus();
    }

private:
    zcore::usize FailEvery_{0U};
    zcore::usize AllocateCalls_{0U};
};

void VerifyMap(const zcore::HashMap<zcore::u32, zcore::u32>& map, const std::unordered_map<zcore::u32, zcore::u32>& oracle)
{
    if (map.Size() != static_cast<zcore::usize>(oracle.size())) {
        FuzzAbort();
    }
    for (const auto& entry : oracle) {
        const zcore::u32* const found = map.TryGet(entry.first);
        if (found == nullptr || *found != entry.second) {
            FuzzAbort();
        }
        if (!map.Contains(entry.first)) {
            FuzzAbort();
        }
    }
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    FuzzCursor cursor(data, size);
    const zcore::usize failEvery = (cursor.TakeByte() % 5U == 0U) ? static_cast<zcore::usize>((cursor.TakeByte() % 7U) + 2U) : 0U;
    FuzzAllocator allocator(failEvery);

    zcore::HashMap<zcore::u32, zcore::u32> map(allocator);
    std::unordered_map<zcore::u32, zcore::u32> oracle;

    const zcore::usize steps = static_cast<zcore::usize>((cursor.TakeByte() % 192U) + 64U);
    for (zcore::usize step = 0U; step < steps; ++step) {
        const std::uint8_t op = static_cast<std::uint8_t>(cursor.TakeByte() % 8U);
        const zcore::u32 key = cursor.TakeU32();
        const zcore::u32 value = cursor.TakeU32();

        if (op == 0U) {
            const bool exists = oracle.find(key) != oracle.end();
            const auto insert = map.TryInsert(key, value);
            if (insert.HasValue()) {
                if (insert.Value() == exists) {
                    FuzzAbort();
                }
                if (!exists) {
                    oracle.emplace(key, value);
                }
            }
        }
        else if (op == 1U) {
            const zcore::Status status = map.TryInsertOrAssign(key, value);
            if (status.HasValue()) {
                oracle[key] = value;
            }
        }
        else if (op == 2U) {
            const bool removed = map.TryRemove(key);
            const bool expected = oracle.erase(key) > 0U;
            if (removed != expected) {
                FuzzAbort();
            }
        }
        else if (op == 3U) {
            const auto it = oracle.find(key);
            const zcore::Option<zcore::u32> removed = map.TryRemoveValue(key);
            if (it == oracle.end()) {
                if (removed.HasValue()) {
                    FuzzAbort();
                }
            }
            else {
                if (!removed.HasValue() || removed.Value() != it->second) {
                    FuzzAbort();
                }
                oracle.erase(it);
            }
        }
        else if (op == 4U) {
            const zcore::usize reserveTarget =
                    static_cast<zcore::usize>(cursor.TakeByte()) + (static_cast<zcore::usize>(cursor.TakeByte()) * 8U);
            (void) map.TryReserve(reserveTarget);
        }
        else if (op == 5U) {
            const bool contains = map.Contains(key);
            const auto it = oracle.find(key);
            if (contains != (it != oracle.end())) {
                FuzzAbort();
            }
            const zcore::u32* const found = map.TryGet(key);
            if (it == oracle.end()) {
                if (found != nullptr) {
                    FuzzAbort();
                }
            }
            else if (found == nullptr || *found != it->second) {
                FuzzAbort();
            }
        }
        else if (op == 6U) {
            map.Clear();
            oracle.clear();
        }
        else {
            map.Reset();
            oracle.clear();
        }

        VerifyMap(map, oracle);
    }

    VerifyMap(map, oracle);
    return 0;
}
