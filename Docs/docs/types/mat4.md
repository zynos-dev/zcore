# `Mat4`

## Purpose

`Mat4` is a deterministic row-major 4x4 float matrix primitive for homogeneous transforms.

## Invariants and Contracts

- Value type: four `Vec4` rows (`float`, no allocation).
- Default state: identity matrix.
- `Mat4 * Vec4` applies row-major matrix-vector multiplication.
- `Mat4 * Mat4` is deterministic row-major composition.

## Usage Example

```cpp
#include <zcore/mat4.hpp>

const zcore::Vec4 out = zcore::Mat4::Identity() * zcore::Vec4(1.0F, 2.0F, 3.0F, 1.0F);
```

## Warnings and Edge Cases

- Equality uses exact float comparison.

## Thread-Safety and Ownership Notes

- Pure value type; copy/share by value.

