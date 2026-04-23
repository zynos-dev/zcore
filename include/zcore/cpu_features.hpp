/**************************************************************************/
/*  cpu_features.hpp                                                      */
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
 * @file include/zcore/cpu_features.hpp
 * @brief Runtime CPU architecture and feature detection helpers.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/cpu_features.hpp>
 * const zcore::CpuFeatures features = zcore::DetectCpuFeatures();
 * @endcode
 */

#pragma once

#include <array>
#include <cstdint>
#include <zcore/foundation.hpp>

#if defined(_MSC_VER)
  #include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
  #if defined(__i386__) || defined(__x86_64__)
    #include <cpuid.h>
  #endif
#endif

namespace zcore {

/**
 * @brief Normalized target CPU architecture classification.
 */
enum class CpuArchitecture : u8 {
    /// @brief Unknown or unsupported architecture.
    UNKNOWN = 0,
    /// @brief 32-bit x86.
    X86_32,
    /// @brief 64-bit x86.
    X86_64,
    /// @brief 32-bit ARM.
    ARM32,
    /// @brief 64-bit ARM (AArch64).
    ARM64,
};

/**
 * @brief Runtime feature bitset describing CPU and OS-enabled capabilities.
 */
struct CpuFeatures final {
    /// @brief Runtime-detected architecture family.
    CpuArchitecture architecture;

    // x86/x64 feature bits
    /// @brief SSE2 instruction support.
    bool sse2;
    /// @brief SSE4.1 instruction support.
    bool sse41;
    /// @brief AVX instruction support with OS state enablement.
    bool avx;
    /// @brief AVX2 instruction support with OS state enablement.
    bool avx2;
    /// @brief FMA instruction support.
    bool fma;
    /// @brief BMI1 instruction support.
    bool bmi1;
    /// @brief BMI2 instruction support.
    bool bmi2;
    /// @brief AES-NI instruction support.
    bool aesNi;
    /// @brief PCLMULQDQ instruction support.
    bool pclmulQdq;
    /// @brief OSXSAVE capability bit (x86/x64).
    bool osXsave;

    // ARM/ARM64 feature bits (best-effort in header-only mode)
    /// @brief NEON SIMD support.
    bool neon;
    /// @brief ARM AES instruction support.
    bool aes;
    /// @brief ARM CRC32 instruction support.
    bool crc32;

    /// @brief Value equality for deterministic testing and caching checks.
    [[nodiscard]] constexpr bool operator==(const CpuFeatures&) const noexcept = default;
};

namespace detail {

[[nodiscard]] constexpr CpuArchitecture DetectArchitecture() noexcept
{
#if defined(_M_X64) || defined(__x86_64__)
    return CpuArchitecture::X86_64;
#elif defined(_M_IX86) || defined(__i386__)
    return CpuArchitecture::X86_32;
#elif defined(_M_ARM64) || defined(__aarch64__)
    return CpuArchitecture::ARM64;
#elif defined(_M_ARM) || defined(__arm__)
    return CpuArchitecture::ARM32;
#else
    return CpuArchitecture::UNKNOWN;
#endif
}

[[nodiscard]] constexpr CpuFeatures DefaultCpuFeatures() noexcept
{
    return CpuFeatures{
            .architecture = DetectArchitecture(),
            .sse2 = false,
            .sse41 = false,
            .avx = false,
            .avx2 = false,
            .fma = false,
            .bmi1 = false,
            .bmi2 = false,
            .aesNi = false,
            .pclmulQdq = false,
            .osXsave = false,
            .neon = false,
            .aes = false,
            .crc32 = false,
    };
}

#if defined(_M_X64) || defined(_M_IX86) || defined(__x86_64__) || defined(__i386__)
[[nodiscard]] inline std::array<std::uint32_t, 4> Cpuid(std::uint32_t leaf, std::uint32_t subleaf) noexcept
{
    std::array<std::uint32_t, 4> regs{0U, 0U, 0U, 0U};
  #if defined(_MSC_VER)
    int out[4] = {0, 0, 0, 0};
    __cpuidex(out, static_cast<int>(leaf), static_cast<int>(subleaf));
    regs[0] = static_cast<std::uint32_t>(out[0]);
    regs[1] = static_cast<std::uint32_t>(out[1]);
    regs[2] = static_cast<std::uint32_t>(out[2]);
    regs[3] = static_cast<std::uint32_t>(out[3]);
  #else
    // Required mutable output operands for __cpuid_count.
    // NOLINTBEGIN(misc-const-correctness)
    std::uint32_t eax = 0;
    std::uint32_t ebx = 0;
    std::uint32_t ecx = 0;
    std::uint32_t edx = 0;
    // NOLINTEND(misc-const-correctness)
    __cpuid_count(leaf, subleaf, eax, ebx, ecx, edx);
    regs[0] = eax;
    regs[1] = ebx;
    regs[2] = ecx;
    regs[3] = edx;
  #endif
    return regs;
}

[[nodiscard]] inline std::uint64_t Xgetbv0() noexcept
{
  #if defined(_MSC_VER)
    return static_cast<std::uint64_t>(_xgetbv(0));
  #elif defined(__GNUC__) || defined(__clang__)
    // Required mutable output operands for xgetbv inline assembly.
    // NOLINTBEGIN(misc-const-correctness)
    std::uint32_t eax = 0;
    std::uint32_t edx = 0;
    // NOLINTEND(misc-const-correctness)
    __asm__ volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
    return (static_cast<std::uint64_t>(edx) << 32U) | eax;
  #else
    return 0;
  #endif
}
#endif

[[nodiscard]] inline CpuFeatures DetectCpuFeaturesImpl() noexcept
{
    CpuFeatures features = DefaultCpuFeatures();

#if defined(_M_X64) || defined(_M_IX86) || defined(__x86_64__) || defined(__i386__)
    const auto leaf0 = Cpuid(0U, 0U);
    const std::uint32_t maxLeaf = leaf0[0];

    if (maxLeaf >= 1U) {
        const auto leaf1 = Cpuid(1U, 0U);
        const std::uint32_t ecx = leaf1[2];
        const std::uint32_t edx = leaf1[3];

        features.sse2 = (edx & (1U << 26U)) != 0U;
        features.sse41 = (ecx & (1U << 19U)) != 0U;
        features.fma = (ecx & (1U << 12U)) != 0U;
        features.aesNi = (ecx & (1U << 25U)) != 0U;
        features.pclmulQdq = (ecx & (1U << 1U)) != 0U;

        const bool avxHw = (ecx & (1U << 28U)) != 0U;
        features.osXsave = (ecx & (1U << 27U)) != 0U;
        if (avxHw && features.osXsave) {
            const std::uint64_t xcr0 = Xgetbv0();
            const bool xmmEnabled = (xcr0 & (1ULL << 1U)) != 0ULL;
            const bool ymmEnabled = (xcr0 & (1ULL << 2U)) != 0ULL;
            features.avx = xmmEnabled && ymmEnabled;
        }
    }

    if (maxLeaf >= 7U) {
        const auto leaf7 = Cpuid(7U, 0U);
        const std::uint32_t ebx = leaf7[1];

        features.bmi1 = (ebx & (1U << 3U)) != 0U;
        features.bmi2 = (ebx & (1U << 8U)) != 0U;
        const bool avx2Hw = (ebx & (1U << 5U)) != 0U;
        features.avx2 = features.avx && avx2Hw;
    }
#endif

#if defined(_M_ARM64) || defined(__aarch64__)
    features.neon = true;
  #if defined(__ARM_FEATURE_AES)
    features.aes = true;
  #endif
  #if defined(__ARM_FEATURE_CRC32)
    features.crc32 = true;
  #endif
#elif defined(_M_ARM) || defined(__arm__)
  #if defined(__ARM_NEON) || defined(__ARM_NEON__)
    features.neon = true;
  #endif
  #if defined(__ARM_FEATURE_AES)
    features.aes = true;
  #endif
  #if defined(__ARM_FEATURE_CRC32)
    features.crc32 = true;
  #endif
#endif

    return features;
}

} // namespace detail

/**
 * @brief Performs runtime CPU feature detection on the current host.
 * @return Newly detected feature set.
 */
[[nodiscard]] inline CpuFeatures DetectCpuFeatures() noexcept
{
    return detail::DetectCpuFeaturesImpl();
}

/**
 * @brief Returns a cached process-wide `CpuFeatures` snapshot.
 *
 * The first call performs detection; subsequent calls return the cached value.
 */
[[nodiscard]] inline const CpuFeatures& GetCpuFeatures() noexcept
{
    static const CpuFeatures kCached = DetectCpuFeatures();
    return kCached;
}

} // namespace zcore
