/**************************************************************************/
/*  math_test.cpp                                                         */
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
 * @file tests/math_test.cpp
 * @brief Unit tests for math primitive contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <zcore/aabb2.hpp>
#include <zcore/aabb3.hpp>
#include <zcore/mat3.hpp>
#include <zcore/mat4.hpp>
#include <zcore/quat.hpp>
#include <zcore/transform3.hpp>
#include <zcore/vec2.hpp>
#include <zcore/vec3.hpp>
#include <zcore/vec4.hpp>

#include <gtest/gtest.h>

#include <type_traits>

namespace {

static_assert(std::is_trivially_copyable_v<zcore::Vec2>);
static_assert(std::is_trivially_copyable_v<zcore::Vec3>);
static_assert(std::is_trivially_copyable_v<zcore::Vec4>);
static_assert(std::is_trivially_copyable_v<zcore::Quat>);

TEST(Vec2Test, ArithmeticAndDotAreDeterministic) {
  const zcore::Vec2 a(1.0F, 2.0F);
  const zcore::Vec2 b(3.0F, 5.0F);

  const zcore::Vec2 sum = a + b;
  const zcore::Vec2 diff = b - a;
  const zcore::Vec2 scaled = a * 2.0F;

  EXPECT_FLOAT_EQ(sum.x, 4.0F);
  EXPECT_FLOAT_EQ(sum.y, 7.0F);
  EXPECT_FLOAT_EQ(diff.x, 2.0F);
  EXPECT_FLOAT_EQ(diff.y, 3.0F);
  EXPECT_FLOAT_EQ(scaled.x, 2.0F);
  EXPECT_FLOAT_EQ(scaled.y, 4.0F);
  EXPECT_FLOAT_EQ(a.Dot(b), 13.0F);
}

TEST(Vec3Test, DotCrossAndHadamardFollowRightHandedRules) {
  const zcore::Vec3 x = zcore::Vec3::UnitX();
  const zcore::Vec3 y = zcore::Vec3::UnitY();
  const zcore::Vec3 z = x.Cross(y);

  EXPECT_EQ(z, zcore::Vec3::UnitZ());
  EXPECT_FLOAT_EQ(x.Dot(y), 0.0F);

  const zcore::Vec3 h = zcore::Vec3(2.0F, 3.0F, 4.0F).Hadamard(zcore::Vec3(5.0F, 6.0F, 7.0F));
  EXPECT_FLOAT_EQ(h.x, 10.0F);
  EXPECT_FLOAT_EQ(h.y, 18.0F);
  EXPECT_FLOAT_EQ(h.z, 28.0F);
}

TEST(Vec4Test, DotAndLengthSquaredAreDeterministic) {
  const zcore::Vec4 vector(1.0F, 2.0F, 3.0F, 4.0F);
  EXPECT_FLOAT_EQ(vector.Dot(zcore::Vec4::One()), 10.0F);
  EXPECT_FLOAT_EQ(vector.LengthSquared(), 30.0F);
}

TEST(Mat3Test, IdentityAndMultiplicationAreDeterministic) {
  const zcore::Mat3 identity = zcore::Mat3::Identity();
  const zcore::Vec3 vector(2.0F, 4.0F, 8.0F);
  EXPECT_EQ(identity * vector, vector);

  const zcore::Mat3 diagonal(
      zcore::Vec3(2.0F, 0.0F, 0.0F),
      zcore::Vec3(0.0F, 3.0F, 0.0F),
      zcore::Vec3(0.0F, 0.0F, 4.0F));
  const zcore::Vec3 scaled = diagonal * vector;
  EXPECT_FLOAT_EQ(scaled.x, 4.0F);
  EXPECT_FLOAT_EQ(scaled.y, 12.0F);
  EXPECT_FLOAT_EQ(scaled.z, 32.0F);

  EXPECT_EQ(identity * diagonal, diagonal);
}

TEST(Mat4Test, IdentityPreservesHomogeneousVectors) {
  const zcore::Mat4 identity = zcore::Mat4::Identity();
  const zcore::Vec4 vector(3.0F, 5.0F, 7.0F, 1.0F);
  EXPECT_EQ(identity * vector, vector);
  EXPECT_EQ(identity * identity, identity);
}

TEST(QuatTest, RotateVectorUsesQuaternionMultiplication) {
  const zcore::Quat rotate180AroundZ(0.0F, 0.0F, 1.0F, 0.0F);
  const zcore::Vec3 input(1.0F, 0.0F, 0.0F);
  const zcore::Vec3 output = rotate180AroundZ.RotateVector(input);

  EXPECT_FLOAT_EQ(output.x, -1.0F);
  EXPECT_FLOAT_EQ(output.y, 0.0F);
  EXPECT_FLOAT_EQ(output.z, 0.0F);
}

TEST(Aabb2Test, ConstructionCanonicalizesAndQueriesBounds) {
  const zcore::Aabb2 bounds(zcore::Vec2(4.0F, -2.0F), zcore::Vec2(-1.0F, 5.0F));

  EXPECT_FLOAT_EQ(bounds.Min().x, -1.0F);
  EXPECT_FLOAT_EQ(bounds.Min().y, -2.0F);
  EXPECT_FLOAT_EQ(bounds.Max().x, 4.0F);
  EXPECT_FLOAT_EQ(bounds.Max().y, 5.0F);
  EXPECT_TRUE(bounds.Contains(zcore::Vec2(0.0F, 0.0F)));
  EXPECT_FALSE(bounds.Contains(zcore::Vec2(10.0F, 0.0F)));
}

TEST(Aabb3Test, MergeAndIntersectionAreDeterministic) {
  zcore::Aabb3 lhs(zcore::Vec3(-1.0F, -1.0F, -1.0F), zcore::Vec3(1.0F, 1.0F, 1.0F));
  const zcore::Aabb3 rhs(zcore::Vec3(0.5F, 0.5F, 0.5F), zcore::Vec3(2.0F, 2.0F, 2.0F));
  EXPECT_TRUE(lhs.Intersects(rhs));

  lhs.Merge(rhs);
  EXPECT_TRUE(lhs.Contains(zcore::Vec3(2.0F, 2.0F, 2.0F)));
  EXPECT_EQ(lhs.Extents(), zcore::Vec3(3.0F, 3.0F, 3.0F));
}

TEST(Transform3Test, TransformPointMatchesMatrixRepresentation) {
  const zcore::Transform3 transform(
      zcore::Vec3(10.0F, 20.0F, 30.0F),
      zcore::Quat::Identity(),
      zcore::Vec3(2.0F, 3.0F, 4.0F));

  const zcore::Vec3 input(1.0F, 1.0F, 1.0F);
  const zcore::Vec3 transformed = transform.TransformPoint(input);
  EXPECT_EQ(transformed, zcore::Vec3(12.0F, 23.0F, 34.0F));

  const zcore::Mat4 matrix = transform.ToMatrix();
  const zcore::Vec4 out = matrix * zcore::Vec4(input.x, input.y, input.z, 1.0F);
  EXPECT_FLOAT_EQ(out.x, transformed.x);
  EXPECT_FLOAT_EQ(out.y, transformed.y);
  EXPECT_FLOAT_EQ(out.z, transformed.z);
  EXPECT_FLOAT_EQ(out.w, 1.0F);
}

}  // namespace

