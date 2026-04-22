/**************************************************************************/
/*  intrinsics_test.cpp                                                   */
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
 * @file tests/intrinsics_test.cpp
 * @brief Unit tests for intrinsics dispatch hook contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/intrinsics.hpp>

#include <gtest/gtest.h>

namespace {

struct IntrinsicsResetGuard final {
  IntrinsicsResetGuard() = default;
  IntrinsicsResetGuard(const IntrinsicsResetGuard&) = delete;
  IntrinsicsResetGuard& operator=(const IntrinsicsResetGuard&) = delete;
  IntrinsicsResetGuard(IntrinsicsResetGuard&&) = delete;
  IntrinsicsResetGuard& operator=(IntrinsicsResetGuard&&) = delete;

  ~IntrinsicsResetGuard() {
    zcore::intrinsics::ResetFeatureProvider();
    zcore::intrinsics::ResetPathSelector();
  }
};

[[nodiscard]] constexpr zcore::CpuFeatures MakeX86Sse2() noexcept {
  return zcore::CpuFeatures{
      .architecture = zcore::CpuArchitecture::X86_64,
      .sse2 = true,
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

[[nodiscard]] constexpr zcore::CpuFeatures MakeX86Avx2() noexcept {
  return zcore::CpuFeatures{
      .architecture = zcore::CpuArchitecture::X86_64,
      .sse2 = true,
      .sse41 = true,
      .avx = true,
      .avx2 = true,
      .fma = true,
      .bmi1 = true,
      .bmi2 = true,
      .aesNi = true,
      .pclmulQdq = true,
      .osXsave = true,
      .neon = false,
      .aes = false,
      .crc32 = false,
  };
}

[[nodiscard]] zcore::CpuFeatures ProvideSse2Features() noexcept {
  return MakeX86Sse2();
}

[[nodiscard]] zcore::CpuFeatures ProvideAvx2Features() noexcept {
  return MakeX86Avx2();
}

[[nodiscard]] zcore::intrinsics::Path ForceScalarPath(const zcore::CpuFeatures&) noexcept {
  return zcore::intrinsics::Path::SCALAR;
}

TEST(IntrinsicsTest, DefaultPathMatchesBuiltinFeatureSelection) {
  const IntrinsicsResetGuard guard;
  const zcore::intrinsics::Path expected = zcore::intrinsics::SelectPathForFeatures(zcore::GetCpuFeatures());
  EXPECT_EQ(zcore::intrinsics::GetPath(), expected);
}

TEST(IntrinsicsTest, FeatureProviderOverrideAffectsPathSelection) {
  const IntrinsicsResetGuard guard;

  zcore::intrinsics::SetFeatureProvider(&ProvideSse2Features);
  EXPECT_EQ(zcore::intrinsics::GetPath(), zcore::intrinsics::Path::X86_SSE2);

  zcore::intrinsics::SetFeatureProvider(&ProvideAvx2Features);
  EXPECT_EQ(zcore::intrinsics::GetPath(), zcore::intrinsics::Path::X86_AVX2);
}

TEST(IntrinsicsTest, PathSelectorOverrideTakesPrecedence) {
  const IntrinsicsResetGuard guard;
  zcore::intrinsics::SetFeatureProvider(&ProvideAvx2Features);
  ASSERT_EQ(zcore::intrinsics::GetPath(), zcore::intrinsics::Path::X86_AVX2);

  zcore::intrinsics::SetPathSelector(&ForceScalarPath);
  EXPECT_EQ(zcore::intrinsics::GetPath(), zcore::intrinsics::Path::SCALAR);
}

}  // namespace
