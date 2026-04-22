/**************************************************************************/
/*  utility/function.hpp                                                  */
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
 * @file include/zcore/utility/function.hpp
 * @brief Allocator-aware move-only callable wrapper.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/function.hpp>
 * @endcode
 */

#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <zcore/contract_violation.hpp>
#include <zcore/memory/allocator.hpp>
#include <zcore/result.hpp>
#include <zcore/status.hpp>
#include <zcore/type_constraints.hpp>
#include <zcore/type_info.hpp>

namespace zcore {

template <typename SignatureT>
class Function;

/**
 * @brief Move-only allocator-aware callable wrapper with explicit fallible bind.
 *
 * @tparam ReturnT Callable return type.
 * @tparam ArgsT Callable parameter types.
 *
 * Callables that fit inline are stored without allocation. Larger/aligned
 * callables spill to allocator-provided storage through explicit fallible APIs.
 */
template <typename ReturnT, typename... ArgsT>
class [[nodiscard("Function must be handled explicitly.")]] Function<ReturnT(ArgsT...)> final {
public:
    using SignatureType = ReturnT(ArgsT...);
    using ReturnType = ReturnT;
    static constexpr usize kInlineStorageSize = sizeof(void*) * 4U;
    static constexpr usize kInlineStorageAlignment = alignof(std::max_align_t);

    /// @brief Constructs empty callable wrapper.
    constexpr Function() noexcept = default;

    Function(const Function&) = delete;
    Function& operator=(const Function&) = delete;

    constexpr Function(Function&& other) noexcept
    {
        MoveFrom(other);
    }

    constexpr Function& operator=(Function&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        Reset();
        MoveFrom(other);
        return *this;
    }

    ~Function()
    {
        Reset();
    }

    /**
   * @brief Constructs function wrapper from callable.
   * @param allocator Allocator used when callable spills out of inline storage.
   * @param callable Callable object to store.
   * @return Function on success, allocator error on failure.
   */
    template <typename CallableT>
        requires(std::is_nothrow_move_constructible_v<std::remove_cvref_t<CallableT>>
                 && std::is_nothrow_invocable_r_v<ReturnT, std::remove_cvref_t<CallableT>&, ArgsT...>)
    [[nodiscard]] static Result<Function, Error> TryMake(Allocator& allocator, CallableT&& callable) noexcept
    {
        Function out;
        Status status = out.TryBind(allocator, std::forward<CallableT>(callable));
        if (status.HasError()) {
            return Result<Function, Error>::Failure(status.Error());
        }
        return Result<Function, Error>::Success(std::move(out));
    }

    /**
   * @brief Replaces current callable target.
   * @param allocator Allocator used when callable spills out of inline storage.
   * @param callable Callable object to store.
   * @return Success or allocator-domain error.
   */
    template <typename CallableT>
        requires(std::is_nothrow_move_constructible_v<std::remove_cvref_t<CallableT>>
                 && std::is_nothrow_invocable_r_v<ReturnT, std::remove_cvref_t<CallableT>&, ArgsT...>)
    [[nodiscard]] Status TryBind(Allocator& allocator, CallableT&& callable) noexcept
    {
        using StoredCallableT = std::remove_cvref_t<CallableT>;
        ZCORE_STATIC_REQUIRE(!std::is_same_v<StoredCallableT, Function>, "Function cannot target itself.");
        ZCORE_STATIC_REQUIRE(constraints::ObjectType<StoredCallableT>, "Function requires object callable target type.");

        Reset();

        if constexpr (kCanStoreInlineV<StoredCallableT>) {
            std::construct_at(static_cast<StoredCallableT*>(InlinePtr()), std::forward<CallableT>(callable));
            Data_ = InlinePtr();
            HeapAllocated_ = false;
            Allocation_ = Allocation::Empty();
            Allocator_ = nullptr;
        }
        else {
            auto allocationResult = allocator.Allocate(AllocationRequest{
                    .size = sizeof(StoredCallableT),
                    .alignment = alignof(StoredCallableT),
            });
            if (allocationResult.HasError()) {
                return ErrorStatus(allocationResult.Error());
            }

            const Allocation allocation = allocationResult.Value();
            ZCORE_CONTRACT_REQUIRE(allocation.IsValid(),
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::Function::TryBind requires allocator to return valid allocation");
            ZCORE_CONTRACT_REQUIRE(!allocation.IsEmpty(),
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::Function::TryBind requires non-empty allocation for callable");

            auto* const callablePtr = reinterpret_cast<StoredCallableT*>(allocation.data);
            std::construct_at(callablePtr, std::forward<CallableT>(callable));

            Data_ = callablePtr;
            HeapAllocated_ = true;
            Allocation_ = allocation;
            Allocator_ = &allocator;
        }

        VTable_ = &VTableFor<StoredCallableT>();
        return OkStatus();
    }

    /// @brief Returns `true` when callable target is bound.
    [[nodiscard]] constexpr bool HasValue() const noexcept
    {
        return VTable_ != nullptr;
    }

    /// @brief Returns `true` when no callable target is bound.
    [[nodiscard]] constexpr bool IsEmpty() const noexcept
    {
        return !HasValue();
    }

    /// @brief Returns runtime type info for callable target, or invalid type when empty.
    [[nodiscard]] constexpr TypeInfo TargetType() const noexcept
    {
        return HasValue() ? VTable_->Info : TypeInfo::Invalid();
    }

    /**
   * @brief Returns `true` when target callable type is exactly `CallableT`.
   */
    template <typename CallableT>
    [[nodiscard]] constexpr bool ContainsTarget() const noexcept
    {
        if (!HasValue()) {
            return false;
        }
        return VTable_->Info.Id() == TypeId::Of<std::remove_cvref_t<CallableT>>();
    }

    /**
   * @brief Returns typed callable pointer when target type matches, otherwise `nullptr`.
   */
    template <typename CallableT>
    [[nodiscard]] CallableT* TryTarget() noexcept
    {
        using StoredCallableT = std::remove_cvref_t<CallableT>;
        if (!ContainsTarget<StoredCallableT>()) {
            return nullptr;
        }
        return std::launder(reinterpret_cast<StoredCallableT*>(Data_));
    }

    /**
   * @brief Returns typed callable pointer when target type matches, otherwise `nullptr`.
   */
    template <typename CallableT>
    [[nodiscard]] const CallableT* TryTarget() const noexcept
    {
        using StoredCallableT = std::remove_cvref_t<CallableT>;
        if (!ContainsTarget<StoredCallableT>()) {
            return nullptr;
        }
        return std::launder(reinterpret_cast<const StoredCallableT*>(Data_));
    }

    /**
   * @brief Invokes callable target.
   * @pre `HasValue()` must be true.
   */
    [[nodiscard]] ReturnT Invoke(ArgsT... args) noexcept
    {
        ZCORE_CONTRACT_REQUIRE(HasValue(),
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Function::Invoke requires bound callable target");

        if constexpr (std::is_void_v<ReturnT>) {
            VTable_->Invoke(Data_, std::forward<ArgsT>(args)...);
            return;
        }
        else {
            return VTable_->Invoke(Data_, std::forward<ArgsT>(args)...);
        }
    }

    /**
   * @brief Invokes callable target.
   * @pre `HasValue()` must be true.
   */
    [[nodiscard]] ReturnT operator()(ArgsT... args) noexcept
    {
        return Invoke(std::forward<ArgsT>(args)...);
    }

    /// @brief Clears callable target and releases owned storage.
    void Reset() noexcept
    {
        if (!HasValue()) {
            return;
        }

        VTable_->Destroy(Data_);
        if (HeapAllocated_) {
            ZCORE_CONTRACT_REQUIRE(Allocator_ != nullptr,
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::Function requires allocator when target is heap allocated");
            const Status deallocateStatus = Allocator_->Deallocate(Allocation_);
            ZCORE_CONTRACT_REQUIRE(deallocateStatus.HasValue(),
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::Function requires allocator deallocation success for heap target");
        }

        Clear();
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return HasValue();
    }

private:
    template <typename CallableT>
    static constexpr bool kCanStoreInlineV =
            sizeof(CallableT) <= kInlineStorageSize && alignof(CallableT) <= kInlineStorageAlignment;

    struct VTable final {
        TypeInfo Info;
        ReturnT (*Invoke)(void* target, ArgsT&&... args) noexcept;
        void (*Destroy)(void* target) noexcept;
        void (*MoveConstruct)(void* destination, void* source) noexcept;
    };

    template <typename CallableT>
    [[nodiscard]] static const VTable& VTableFor() noexcept
    {
        static const VTable kTable{
                .Info = TypeInfo::Of<CallableT>(),
                .Invoke = &InvokeTarget<CallableT>,
                .Destroy = &DestroyTarget<CallableT>,
                .MoveConstruct = &MoveTarget<CallableT>,
        };
        return kTable;
    }

    template <typename CallableT>
    static ReturnT InvokeTarget(void* target, ArgsT&&... args) noexcept
    {
        auto* const callable = std::launder(reinterpret_cast<CallableT*>(target));
        if constexpr (std::is_void_v<ReturnT>) {
            (*callable)(std::forward<ArgsT>(args)...);
            return;
        }
        else {
            return (*callable)(std::forward<ArgsT>(args)...);
        }
    }

    template <typename CallableT>
    static void DestroyTarget(void* target) noexcept
    {
        auto* const callable = std::launder(reinterpret_cast<CallableT*>(target));
        std::destroy_at(callable);
    }

    template <typename CallableT>
    static void MoveTarget(void* destination, void* source) noexcept
    {
        auto* const sourcePtr = std::launder(reinterpret_cast<CallableT*>(source));
        std::construct_at(std::launder(reinterpret_cast<CallableT*>(destination)), std::move(*sourcePtr));
    }

    constexpr void MoveFrom(Function& other) noexcept
    {
        if (!other.HasValue()) {
            return;
        }

        if (other.HeapAllocated_) {
            Data_ = other.Data_;
            VTable_ = other.VTable_;
            HeapAllocated_ = true;
            Allocation_ = other.Allocation_;
            Allocator_ = other.Allocator_;
            other.Clear();
            return;
        }

        VTable_ = other.VTable_;
        VTable_->MoveConstruct(InlinePtr(), other.Data_);
        VTable_->Destroy(other.Data_);
        Data_ = InlinePtr();
        HeapAllocated_ = false;
        Allocation_ = Allocation::Empty();
        Allocator_ = nullptr;
        other.Clear();
    }

    constexpr void Clear() noexcept
    {
        Data_ = nullptr;
        VTable_ = nullptr;
        HeapAllocated_ = false;
        Allocation_ = Allocation::Empty();
        Allocator_ = nullptr;
    }

    [[nodiscard]] constexpr void* InlinePtr() noexcept
    {
        return static_cast<void*>(InlineStorage_);
    }

    alignas(kInlineStorageAlignment) Byte InlineStorage_[kInlineStorageSize]{};
    void* Data_{nullptr};
    const VTable* VTable_{nullptr};
    bool HeapAllocated_{false};
    Allocation Allocation_{Allocation::Empty()};
    Allocator* Allocator_{nullptr};
};

} // namespace zcore
