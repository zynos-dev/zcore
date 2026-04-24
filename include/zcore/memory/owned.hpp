/**************************************************************************/
/*  memory/owned.hpp                                                      */
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
 * @file include/zcore/memory/owned.hpp
 * @brief Allocator-owned unique value wrapper.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/owned.hpp>
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
 * @brief Unique owning wrapper for a single allocator-backed object.
 *
 * @tparam ValueT Owned value type.
 *
 * `Owned<ValueT>` allocates storage through `Allocator`, constructs `ValueT`
 * in-place, destroys it on release, then returns storage to the same allocator.
 */
template <typename ValueT>
class [[nodiscard("Owned must be handled explicitly.")]] Owned final {
public:
    ZCORE_STATIC_REQUIRE(constraints::NonReferenceNonVoidNonFunctionType<ValueT>,
                         "Owned<ValueT> requires non-reference non-void non-function value type.");

    using ElementType = ValueT;

    /// @brief Constructs an empty owned handle with no allocation.
    constexpr Owned() noexcept : Allocation_(Allocation::Empty()), Allocator_(nullptr)
    {
    }

    Owned(const Owned&) = delete;
    Owned& operator=(const Owned&) = delete;

    constexpr Owned(Owned&& other) noexcept : Allocation_(other.Allocation_), Allocator_(other.Allocator_)
    {
        other.Clear();
    }

    constexpr Owned& operator=(Owned&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        Reset();
        Allocation_ = other.Allocation_;
        Allocator_ = other.Allocator_;
        other.Clear();
        return *this;
    }

    ~Owned()
    {
        Reset();
    }

    /**
   * @brief Allocates and constructs an owned value.
   * @param allocator Allocator used for storage.
   * @param args Constructor arguments for `ValueT`.
   * @return Owned value or allocator-domain failure.
   */
    template <typename... ArgsT>
        requires(std::is_nothrow_constructible_v<ValueT, ArgsT...>)
    [[nodiscard]] static Result<Owned, Error> TryMake(Allocator& allocator, ArgsT&&... args) noexcept
    {
        auto allocationResult = allocator.Allocate(AllocationRequest{
                .size = sizeof(ValueT),
                .alignment = alignof(ValueT),
        });
        if (allocationResult.HasError()) {
            return Result<Owned, Error>::Failure(allocationResult.Error());
        }

        const Allocation allocation = allocationResult.Value();
        ZCORE_CONTRACT_REQUIRE(allocation.IsValid(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Owned::TryMake requires allocator to return valid allocation");
        ZCORE_CONTRACT_REQUIRE(!allocation.IsEmpty(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Owned::TryMake requires non-empty allocation for object storage");

        ValueT* const valuePtr = reinterpret_cast<ValueT*>(allocation.data);
        std::construct_at(valuePtr, std::forward<ArgsT>(args)...);
        return Result<Owned, Error>::Success(Owned(allocation, &allocator));
    }

    /// @brief Returns whether an owned value is present.
    [[nodiscard]] constexpr bool HasValue() const noexcept
    {
        return !Allocation_.IsEmpty();
    }

    /// @brief Returns whether this instance is empty.
    [[nodiscard]] constexpr bool IsEmpty() const noexcept
    {
        return !HasValue();
    }

    /// @brief Returns pointer to owned value or null when empty.
    [[nodiscard]] constexpr ValueT* Get() noexcept
    {
        return HasValue() ? reinterpret_cast<ValueT*>(Allocation_.data) : nullptr;
    }

    /// @brief Returns const pointer to owned value or null when empty.
    [[nodiscard]] constexpr const ValueT* Get() const noexcept
    {
        return HasValue() ? reinterpret_cast<const ValueT*>(Allocation_.data) : nullptr;
    }

    /**
   * @brief Returns owned value.
   * @pre `HasValue()` must be true.
   */
    [[nodiscard]] ValueT& Value() & noexcept
    {
        ZCORE_CONTRACT_REQUIRE(HasValue(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Owned::Value() requires a present value");
        return *Get();
    }

    /**
   * @brief Returns owned value.
   * @pre `HasValue()` must be true.
   */
    [[nodiscard]] const ValueT& Value() const& noexcept
    {
        ZCORE_CONTRACT_REQUIRE(HasValue(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Owned::Value() requires a present value");
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

    /// @brief Returns allocator associated with owned value or null when empty.
    [[nodiscard]] constexpr Allocator* AllocatorRef() const noexcept
    {
        return Allocator_;
    }

    /// @brief Destroys and deallocates owned value; no-op when empty.
    void Reset() noexcept
    {
        if (!HasValue()) {
            return;
        }

        ValueT* const valuePtr = Get();
        std::destroy_at(valuePtr);

        const Status deallocateStatus = Allocator_->Deallocate(Allocation_);
        ZCORE_CONTRACT_REQUIRE(deallocateStatus.HasValue(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Owned requires allocator deallocation success for owned allocations");
        Clear();
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return HasValue();
    }

private:
    constexpr Owned(Allocation allocation, Allocator* allocator) noexcept : Allocation_(allocation), Allocator_(allocator)
    {
    }

    constexpr void Clear() noexcept
    {
        Allocation_ = Allocation::Empty();
        Allocator_ = nullptr;
    }

    Allocation Allocation_;
    Allocator* Allocator_;
};

} // namespace zcore
