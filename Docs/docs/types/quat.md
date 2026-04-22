# `Quat`

## Purpose

`Quat` is a deterministic quaternion primitive for representing and applying 3D rotations.

## Invariants and Contracts

- Value type: `{x, y, z, w}` (`float`, no allocation).
- Default state: identity rotation `(0, 0, 0, 1)`.
- Multiplication follows Hamilton product order.
- `RotateVector(v)` computes `q * (v,0) * conjugate(q)`.

## Usage Example

```cpp
#include <zcore/quat.hpp>

const zcore::Vec3 out = zcore::Quat::Identity().RotateVector(zcore::Vec3::UnitX());
```

## Warnings and Edge Cases

- Rotation assumes caller provides normalized quaternions for pure rotational behavior.
- Equality uses exact float comparison.

## Thread-Safety and Ownership Notes

- Pure value type; copy/share by value.

