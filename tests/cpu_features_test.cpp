/**************************************************************************/
/*  cpu_features_test.cpp                                                 */
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
 * @file tests/cpu_features_test.cpp
 * @brief Unit tests for CPU feature detection contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/cpu_features.hpp>

#include <gtest/gtest.h>

namespace {

TEST(CpuFeaturesTest, DetectionIsDeterministicAcrossCalls) {
  const zcore::CpuFeatures first = zcore::DetectCpuFeatures();
  const zcore::CpuFeatures second = zcore::DetectCpuFeatures();
  EXPECT_EQ(first, second);
}

TEST(CpuFeaturesTest, CachedAccessorMatchesDirectDetection) {
  const zcore::CpuFeatures detected = zcore::DetectCpuFeatures();
  const zcore::CpuFeatures cached = zcore::GetCpuFeatures();
  EXPECT_EQ(detected, cached);
}

TEST(CpuFeaturesTest, ArchitectureSpecificInvariantsHold) {
  const zcore::CpuFeatures features = zcore::GetCpuFeatures();

  switch (features.architecture) {
    case zcore::CpuArchitecture::X86_32:
    case zcore::CpuArchitecture::X86_64:
      if (features.avx2) {
        EXPECT_TRUE(features.avx);
      }
      if (features.avx) {
        EXPECT_TRUE(features.osXsave);
      }
      if (features.sse41) {
        EXPECT_TRUE(features.sse2);
      }
      break;

    case zcore::CpuArchitecture::ARM64:
      EXPECT_TRUE(features.neon);
      break;

    case zcore::CpuArchitecture::ARM32:
    case zcore::CpuArchitecture::UNKNOWN:
      break;
  }
}

}  // namespace
