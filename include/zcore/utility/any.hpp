/**************************************************************************/
/*  utility/any.hpp                                                       */
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
 * @file include/zcore/utility/any.hpp
 * @brief Allocator-aware move-only type-erased value container.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/any.hpp>
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

/**
 * @brief Move-only allocator-aware type-erased value.
 *
 * Values that fit inline are stored without allocation. Larger/aligned values
 * spill to allocator-provided storage through explicit fallible APIs.
 */
class [[nodiscard("Any must be handled explicitly.")]] Any final {
public:
    static constexpr usize kInlineStorageSize = sizeof(void*) * 3U;
    static constexpr usize kInlineStorageAlignment = alignof(std::max_align_t);

    /// @brief Constructs empty container.
    constexpr Any() noexcept = default;

    Any(const Any&) = delete;
    Any& operator=(const Any&) = delete;

    constexpr Any(Any&& other) noexcept
    {
        MoveFrom(other);
    }

    constexpr Any& operator=(Any&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        Reset();
        MoveFrom(other);
        return *this;
    }

    ~Any()
    {
        Reset();
    }

    /**
   * @brief Constructs `Any` with value type `ValueT`.
   * @param allocator Allocator used when value spills out of inline storage.
   * @param args Constructor arguments for `ValueT`.
   * @return `Any` on success, allocator error on failure.
   */
    template <typename ValueT, typename... ArgsT>
        requires(std::is_nothrow_constructible_v<std::remove_cvref_t<ValueT>, ArgsT...>
                 && std::is_nothrow_move_constructible_v<std::remove_cvref_t<ValueT>>)
    [[nodiscard]] static Result<Any, Error> TryMake(Allocator& allocator, ArgsT&&... args) noexcept
    {
        Any out;
        Status status = out.TryEmplace<ValueT>(allocator, std::forward<ArgsT>(args)...);
        if (status.HasError()) {
            return Result<Any, Error>::Failure(status.Error());
        }
        return Result<Any, Error>::Success(std::move(out));
    }

    /**
   * @brief Replaces current value with `ValueT`.
   * @param allocator Allocator used when value spills out of inline storage.
   * @param args Constructor arguments for `ValueT`.
   * @return Success or allocator-domain error.
   */
    template <typename ValueT, typename... ArgsT>
        requires(std::is_nothrow_constructible_v<std::remove_cvref_t<ValueT>, ArgsT...>
                 && std::is_nothrow_move_constructible_v<std::remove_cvref_t<ValueT>>)
    [[nodiscard]] Status TryEmplace(Allocator& allocator, ArgsT&&... args) noexcept
    {
        using StoredValueT = std::remove_cvref_t<ValueT>;
        ZCORE_STATIC_REQUIRE(constraints::NonReferenceNonVoidNonFunctionType<StoredValueT>,
                             "Any does not support reference, void, or function value types.");

        Reset();

        if constexpr (kCanStoreInlineV<StoredValueT>) {
            std::construct_at(static_cast<StoredValueT*>(InlinePtr()), std::forward<ArgsT>(args)...);
            Data_ = InlinePtr();
            HeapAllocated_ = false;
            Allocation_ = Allocation::Empty();
            Allocator_ = nullptr;
        }
        else {
            auto allocationResult = allocator.Allocate(AllocationRequest{
                    .size = sizeof(StoredValueT),
                    .alignment = alignof(StoredValueT),
            });
            if (allocationResult.HasError()) {
                return ErrorStatus(allocationResult.Error());
            }

            const Allocation allocation = allocationResult.Value();
            ZCORE_CONTRACT_REQUIRE(allocation.IsValid(),
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::Any::TryEmplace requires allocator to return valid allocation");
            ZCORE_CONTRACT_REQUIRE(!allocation.IsEmpty(),
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::Any::TryEmplace requires non-empty allocation for stored value");

            auto* const valuePtr = reinterpret_cast<StoredValueT*>(allocation.data);
            std::construct_at(valuePtr, std::forward<ArgsT>(args)...);

            Data_ = valuePtr;
            HeapAllocated_ = true;
            Allocation_ = allocation;
            Allocator_ = &allocator;
        }

        VTable_ = &VTableFor<StoredValueT>();
        return OkStatus();
    }

    /// @brief Returns `true` when container stores a value.
    [[nodiscard]] constexpr bool HasValue() const noexcept
    {
        return VTable_ != nullptr;
    }

    /// @brief Returns `true` when container is empty.
    [[nodiscard]] constexpr bool IsEmpty() const noexcept
    {
        return !HasValue();
    }

    /// @brief Returns runtime type info for stored value, or invalid type for empty state.
    [[nodiscard]] constexpr TypeInfo Type() const noexcept
    {
        return HasValue() ? VTable_->Info : TypeInfo::Invalid();
    }

    /**
   * @brief Returns `true` when stored value is exactly `ValueT`.
   */
    template <typename ValueT>
    [[nodiscard]] constexpr bool Contains() const noexcept
    {
        if (!HasValue()) {
            return false;
        }
        return VTable_->Info.Id() == TypeId::Of<std::remove_cvref_t<ValueT>>();
    }

    /**
   * @brief Returns typed pointer when value type matches, otherwise `nullptr`.
   */
    template <typename ValueT>
    [[nodiscard]] ValueT* TryGet() noexcept
    {
        using StoredValueT = std::remove_cvref_t<ValueT>;
        if (!Contains<StoredValueT>()) {
            return nullptr;
        }
        return std::launder(reinterpret_cast<StoredValueT*>(Data_));
    }

    /**
   * @brief Returns typed pointer when value type matches, otherwise `nullptr`.
   */
    template <typename ValueT>
    [[nodiscard]] const ValueT* TryGet() const noexcept
    {
        using StoredValueT = std::remove_cvref_t<ValueT>;
        if (!Contains<StoredValueT>()) {
            return nullptr;
        }
        return std::launder(reinterpret_cast<const StoredValueT*>(Data_));
    }

    /**
   * @brief Returns stored value reference.
   * @pre `Contains<ValueT>()` must be true.
   */
    template <typename ValueT>
    [[nodiscard]] ValueT& Get() & noexcept
    {
        ValueT* const ptr = TryGet<ValueT>();
        ZCORE_CONTRACT_REQUIRE(ptr != nullptr,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Any::Get<ValueT>() requires stored value of ValueT");
        return *ptr;
    }

    /**
   * @brief Returns stored value reference.
   * @pre `Contains<ValueT>()` must be true.
   */
    template <typename ValueT>
    [[nodiscard]] const ValueT& Get() const& noexcept
    {
        const ValueT* const ptr = TryGet<ValueT>();
        ZCORE_CONTRACT_REQUIRE(ptr != nullptr,
                               detail::ContractViolationCode::PRECONDITION,
                               "zcore::Any::Get<ValueT>() requires stored value of ValueT");
        return *ptr;
    }

    /// @brief Clears contained value and releases owned storage.
    void Reset() noexcept
    {
        if (!HasValue()) {
            return;
        }

        VTable_->Destroy(Data_);
        if (HeapAllocated_) {
            ZCORE_CONTRACT_REQUIRE(Allocator_ != nullptr,
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::Any requires allocator when value is heap allocated");
            const Status deallocateStatus = Allocator_->Deallocate(Allocation_);
            ZCORE_CONTRACT_REQUIRE(deallocateStatus.HasValue(),
                                   detail::ContractViolationCode::PRECONDITION,
                                   "zcore::Any requires allocator deallocation success for heap value");
        }

        Clear();
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return HasValue();
    }

private:
    template <typename ValueT>
    static constexpr bool kCanStoreInlineV = sizeof(ValueT) <= kInlineStorageSize && alignof(ValueT) <= kInlineStorageAlignment;

    struct VTable final {
        TypeInfo Info;
        void (*Destroy)(void*) noexcept;
        void (*MoveConstruct)(void* destination, void* source) noexcept;
    };

    template <typename ValueT>
    [[nodiscard]] static const VTable& VTableFor() noexcept
    {
        static const VTable kTable{
                .Info = TypeInfo::Of<ValueT>(),
                .Destroy = &DestroyValue<ValueT>,
                .MoveConstruct = &MoveValue<ValueT>,
        };
        return kTable;
    }

    template <typename ValueT>
    static void DestroyValue(void* value) noexcept
    {
        auto* const ptr = std::launder(reinterpret_cast<ValueT*>(value));
        std::destroy_at(ptr);
    }

    template <typename ValueT>
    static void MoveValue(void* destination, void* source) noexcept
    {
        auto* const sourcePtr = std::launder(reinterpret_cast<ValueT*>(source));
        std::construct_at(std::launder(reinterpret_cast<ValueT*>(destination)), std::move(*sourcePtr));
    }

    constexpr void MoveFrom(Any& other) noexcept
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
