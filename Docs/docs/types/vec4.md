# `Vec4`

## Purpose

`Vec4` is a deterministic 4D float vector primitive for homogeneous math and matrix operations.

## Invariants and Contracts

- Value type: `{x, y, z, w}` (`float`, no allocation).
- Default state: `(0, 0, 0, 0)`.
- Arithmetic is component-wise for `+`, `-`, scalar `*`/`/`.
- `Dot` and `LengthSquared` are deterministic pure operations.

## Usage Example

```cpp
#include <zcore/vec4.hpp>

const zcore::Vec4 p(1.0F, 2.0F, 3.0F, 1.0F);
```

## Warnings and Edge Cases

- Scalar division does not guard against zero divisors.
- Equality uses exact float comparison.

## Thread-Safety and Ownership Notes

- Pure value type; copy/share by value.

