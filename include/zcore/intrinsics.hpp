/**************************************************************************/
/*  intrinsics.hpp                                                        */
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
 * @file include/zcore/intrinsics.hpp
 * @brief Runtime intrinsics dispatch hooks and path-selection utilities.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/intrinsics.hpp>
 * const zcore::intrinsics::Path path = zcore::intrinsics::GetPath();
 * @endcode
 */

#pragma once

#include <atomic>
#include <zcore/cpu_features.hpp>
#include <zcore/foundation.hpp>

namespace zcore::intrinsics {

/**
 * @brief Normalized ISA dispatch path classification.
 */
enum class Path : u8 {
    /// @brief Portable scalar path.
    SCALAR = 0,
    /// @brief x86 SSE2 path.
    X86_SSE2,
    /// @brief x86 AVX2 path.
    X86_AVX2,
    /// @brief ARM NEON path.
    ARM_NEON,
};

/// @brief App-provided runtime CPU feature provider callback type.
using FeatureProviderFn = CpuFeatures (*)() noexcept;

/// @brief App-provided path-selection callback type.
using PathSelectorFn = Path (*)(const CpuFeatures&) noexcept;

namespace detail {

[[nodiscard]] inline std::atomic<FeatureProviderFn>& FeatureProviderSlot() noexcept
{
    static std::atomic<FeatureProviderFn> slot{nullptr};
    return slot;
}

[[nodiscard]] inline std::atomic<PathSelectorFn>& PathSelectorSlot() noexcept
{
    static std::atomic<PathSelectorFn> slot{nullptr};
    return slot;
}

} // namespace detail

/**
 * @brief Selects dispatch path for a specific feature set.
 * @param features Runtime feature set.
 * @return Preferred normalized dispatch path.
 */
[[nodiscard]] constexpr Path SelectPathForFeatures(const CpuFeatures& features) noexcept
{
    switch (features.architecture) {
    case CpuArchitecture::X86_32:
    case CpuArchitecture::X86_64:
        if (features.avx2) {
            return Path::X86_AVX2;
        }
        if (features.sse2) {
            return Path::X86_SSE2;
        }
        return Path::SCALAR;

    case CpuArchitecture::ARM32:
    case CpuArchitecture::ARM64:
        if (features.neon) {
            return Path::ARM_NEON;
        }
        return Path::SCALAR;

    case CpuArchitecture::UNKNOWN:
        return Path::SCALAR;
    }

    return Path::SCALAR;
}

/**
 * @brief Registers app-provided runtime CPU feature provider.
 * @param provider Callback; `nullptr` resets to builtin `GetCpuFeatures()`.
 */
inline void SetFeatureProvider(FeatureProviderFn provider) noexcept
{
    detail::FeatureProviderSlot().store(provider, std::memory_order_release);
}

/// @brief Resets to builtin process-cached feature provider.
inline void ResetFeatureProvider() noexcept
{
    SetFeatureProvider(nullptr);
}

/**
 * @brief Registers app-provided path-selection callback.
 * @param selector Callback; `nullptr` resets to builtin path selection.
 */
inline void SetPathSelector(PathSelectorFn selector) noexcept
{
    detail::PathSelectorSlot().store(selector, std::memory_order_release);
}

/// @brief Resets to builtin path-selection policy.
inline void ResetPathSelector() noexcept
{
    SetPathSelector(nullptr);
}

/**
 * @brief Returns current feature set from configured provider or builtin cache.
 */
[[nodiscard]] inline CpuFeatures GetFeatures() noexcept
{
    const FeatureProviderFn provider = detail::FeatureProviderSlot().load(std::memory_order_acquire);
    if (provider != nullptr) {
        return provider();
    }
    return GetCpuFeatures();
}

/**
 * @brief Returns selected dispatch path from configured selector or builtin policy.
 */
[[nodiscard]] inline Path GetPath() noexcept
{
    const CpuFeatures features = GetFeatures();
    const PathSelectorFn selector = detail::PathSelectorSlot().load(std::memory_order_acquire);
    if (selector != nullptr) {
        return selector(features);
    }
    return SelectPathForFeatures(features);
}

/**
 * @brief Selects one of four dispatch function pointers by normalized path.
 * @tparam DispatchFnT Function pointer type.
 * @param path Selected path.
 * @param scalar Scalar fallback function.
 * @param x86Sse2 x86 SSE2 function.
 * @param x86Avx2 x86 AVX2 function.
 * @param armNeon ARM NEON function.
 * @return Function pointer for selected path.
 */
template <typename DispatchFnT>
[[nodiscard]] constexpr DispatchFnT
SelectDispatch(Path path, DispatchFnT scalar, DispatchFnT x86Sse2, DispatchFnT x86Avx2, DispatchFnT armNeon) noexcept
{
    switch (path) {
    case Path::SCALAR:
        return scalar;
    case Path::X86_SSE2:
        return x86Sse2;
    case Path::X86_AVX2:
        return x86Avx2;
    case Path::ARM_NEON:
        return armNeon;
    }
    return scalar;
}

} // namespace zcore::intrinsics
