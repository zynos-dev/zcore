# `Mat3`

## Purpose

`Mat3` is a deterministic row-major 3x3 float matrix primitive for 3D linear transforms without translation.

## Invariants and Contracts

- Value type: three `Vec3` rows (`float`, no allocation).
- Default state: identity matrix.
- `Mat3 * Vec3` applies row-major matrix-vector multiplication.
- `Mat3 * Mat3` is deterministic row-major composition.

## Usage Example

```cpp
#include <zcore/mat3.hpp>

const zcore::Vec3 out = zcore::Mat3::Identity() * zcore::Vec3(1.0F, 2.0F, 3.0F);
```

## Warnings and Edge Cases

- Equality uses exact float comparison.

## Thread-Safety and Ownership Notes

- Pure value type; copy/share by value.

