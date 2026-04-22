# `Vec3`

## Purpose

`Vec3` is a deterministic 3D float vector primitive for runtime spatial math.

## Invariants and Contracts

- Value type: `{x, y, z}` (`float`, no allocation).
- Default state: `(0, 0, 0)`.
- `Cross` follows right-handed order: `UnitX().Cross(UnitY()) = UnitZ()`.
- `Hadamard` is component-wise multiplication.

## Usage Example

```cpp
#include <zcore/vec3.hpp>

const zcore::Vec3 normal = zcore::Vec3::UnitX().Cross(zcore::Vec3::UnitY());
```

## Warnings and Edge Cases

- Scalar division does not guard against zero divisors.
- Equality uses exact float comparison.

## Thread-Safety and Ownership Notes

- Pure value type; copy/share by value.

