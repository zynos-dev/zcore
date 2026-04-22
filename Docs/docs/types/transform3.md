# `Transform3`

## Purpose

`Transform3` is a deterministic 3D transform value type with explicit translation, rotation (`Quat`), and scale components.

## Invariants and Contracts

- Stored as TRS components: `Vec3 translation`, `Quat rotation`, `Vec3 scale`.
- Default state: identity transform (zero translation, identity rotation, unit scale).
- `TransformPoint` applies scale -> rotation -> translation.
- `ToMatrix` returns row-major `Mat4` matching `TransformPoint` semantics.

## Usage Example

```cpp
#include <zcore/transform3.hpp>

const zcore::Vec3 out = zcore::Transform3::Identity().TransformPoint(zcore::Vec3::UnitX());
```

## Warnings and Edge Cases

- Rotation quality depends on quaternion normalization supplied by caller.
- Equality uses exact float comparison.

## Thread-Safety and Ownership Notes

- Pure value type; copy/share by value.

