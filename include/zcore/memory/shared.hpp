/**************************************************************************/
/*  memory/shared.hpp                                                     */
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
 * @file include/zcore/memory/shared.hpp
 * @brief Allocator-owned shared value wrapper.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/shared.hpp>
 * @endcode
 */

#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include <zcore/contract_violation.hpp>
#include <zcore/memory/allocator.hpp>
#include <zcore/type_constraints.hpp>

namespace zcore {

/**
 * @brief Shared owning wrapper for a single allocator-backed object.
 *
 * @tparam ValueT Shared value type.
 *
 * `Shared<ValueT>` allocates a control block through `Allocator`,
 * constructs `ValueT` in-place, and deallocates on last-owner release.
 *
 * Thread-safety is allocator/caller-defined; reference counting is not atomic.
 */
template <typename ValueT>
class [[nodiscard("Shared must be handled explicitly.")]] Shared final {
public:
    ZCORE_STATIC_REQUIRE(constraints::NonReferenceNonVoidNonFunctionType<ValueT>,
                         "Shared<ValueT> requires non-reference non-void non-function value type.");

    using ElementType = ValueT;

    /// @brief Constructs an empty shared handle with no allocation.
    constexpr Shared() noexcept : Block_(nullptr)
    {
    }

    constexpr Shared(const Shared& other) noexcept : Block_(other.Block_)
    {
        AddRef();
    }

    constexpr Shared& operator=(const Shared& other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        Release();
        Block_ = other.Block_;
        AddRef();
        return *this;
    }

    constexpr Shared(Shared&& other) noexcept : Block_(other.Block_)
    {
        other.Block_ = nullptr;
    }

    constexpr Shared& operator=(Shared&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        Release();
        Block_ = other.Block_;
        other.Block_ = nullptr;
        return *this;
    }

    ~Shared()
    {
        Release();
    }

    /**
   * @brief Allocates and constructs a shared value.
   * @param allocator Allocator used for control block storage.
   * @param args Constructor arguments for `ValueT`.
   * @return Shared value or allocator-domain failure.
   */
    template <typename... ArgsT>
        requires(std::is_nothrow_constructible_v<ValueT, ArgsT...>)
    [[nodiscard]] static Result<Shared, Error> TryMake(Allocator& allocator, ArgsT&&... args) noexcept
    {
        auto allocationResult = allocator.Allocate(AllocationRequest{
                .size = sizeof(ControlBlock),
                .alignment = alignof(ControlBlock),
        });
        if (allocationResult.HasError()) {
            return Result<Shared, Error>::Failure(allocationResult.Error());
        }

        const Allocation allocation = allocationResult.Value();
        ZCORE_CONTRACT_REQUIRE(allocation.IsValid(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Shared::TryMake requires allocator to return valid allocation");
        ZCORE_CONTRACT_REQUIRE(!allocation.IsEmpty(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Shared::TryMake requires non-empty allocation for control block");

        auto* const block = reinterpret_cast<ControlBlock*>(allocation.data);
        std::construct_at(block,
                          ControlBlock{
                                  .AllocationDescriptor = allocation,
                                  .AllocatorRef = &allocator,
                                  .RefCount = 1U,
                          });
        std::construct_at(block->ValuePtr(), std::forward<ArgsT>(args)...);

        return Result<Shared, Error>::Success(Shared(block));
    }

    /// @brief Returns whether a shared value is present.
    [[nodiscard]] constexpr bool HasValue() const noexcept
    {
        return Block_ != nullptr;
    }

    /// @brief Returns whether this instance is empty.
    [[nodiscard]] constexpr bool IsEmpty() const noexcept
    {
        return !HasValue();
    }

    /// @brief Returns shared reference count, `0` when empty.
    [[nodiscard]] constexpr usize UseCount() const noexcept
    {
        return HasValue() ? Block_->RefCount : 0U;
    }

    /// @brief Returns `true` when exactly one owner exists.
    [[nodiscard]] constexpr bool IsUnique() const noexcept
    {
        return UseCount() == 1U;
    }

    /// @brief Returns pointer to shared value or null when empty.
    [[nodiscard]] constexpr ValueT* Get() noexcept
    {
        return HasValue() ? Block_->ValuePtr() : nullptr;
    }

    /// @brief Returns const pointer to shared value or null when empty.
    [[nodiscard]] constexpr const ValueT* Get() const noexcept
    {
        return HasValue() ? Block_->ValuePtr() : nullptr;
    }

    /**
   * @brief Returns shared value.
   * @pre `HasValue()` must be true.
   */
    [[nodiscard]] ValueT& Value() & noexcept
    {
        ZCORE_CONTRACT_REQUIRE(HasValue(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Shared::Value() requires a present value");
        return *Get();
    }

    /**
   * @brief Returns shared value.
   * @pre `HasValue()` must be true.
   */
    [[nodiscard]] const ValueT& Value() const& noexcept
    {
        ZCORE_CONTRACT_REQUIRE(HasValue(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Shared::Value() requires a present value");
        return *Get();
    }

    [[nodiscard]] ValueT* operator->() noexcept
    {
        return &Value();
    }

    [[nodiscard]] const ValueT* operator->() const noexcept
    {
        return &Value();
    }

    [[nodiscard]] ValueT& operator*() noexcept
    {
        return Value();
    }

    [[nodiscard]] const ValueT& operator*() const noexcept
    {
        return Value();
    }

    /// @brief Returns allocator associated with shared value or null when empty.
    [[nodiscard]] constexpr Allocator* AllocatorRef() const noexcept
    {
        return HasValue() ? Block_->AllocatorRef : nullptr;
    }

    /// @brief Releases one ownership reference; last release destroys and deallocates.
    void Reset() noexcept
    {
        Release();
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return HasValue();
    }

private:
    struct ControlBlock final {
        Allocation AllocationDescriptor;
        Allocator* AllocatorRef;
        usize RefCount;
        alignas(ValueT) Byte ValueStorage[sizeof(ValueT)];

        [[nodiscard]] constexpr ValueT* ValuePtr() noexcept
        {
            return std::launder(reinterpret_cast<ValueT*>(ValueStorage));
        }

        [[nodiscard]] constexpr const ValueT* ValuePtr() const noexcept
        {
            return std::launder(reinterpret_cast<const ValueT*>(ValueStorage));
        }
    };

    constexpr explicit Shared(ControlBlock* block) noexcept : Block_(block)
    {
    }

    constexpr void AddRef() noexcept
    {
        if (!HasValue()) {
            return;
        }
        ZCORE_CONTRACT_REQUIRE(Block_->RefCount > 0U,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Shared requires positive reference count for live control block");
        ++Block_->RefCount;
    }

    void Release() noexcept
    {
        if (!HasValue()) {
            return;
        }

        ControlBlock* const block = Block_;
        Block_ = nullptr;

        ZCORE_CONTRACT_REQUIRE(block->RefCount > 0U,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Shared requires positive reference count on release");
        --block->RefCount;
        if (block->RefCount != 0U) {
            return;
        }

        ValueT* const valuePtr = block->ValuePtr();
        std::destroy_at(valuePtr);

        const Allocation allocation = block->AllocationDescriptor;
        Allocator* const allocator = block->AllocatorRef;
        std::destroy_at(block);

        const Status deallocateStatus = allocator->Deallocate(allocation);
        ZCORE_CONTRACT_REQUIRE(deallocateStatus.HasValue(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Shared requires allocator deallocation success for shared control block");
    }

    ControlBlock* Block_;
};

} // namespace zcore
