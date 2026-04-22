# `Vec2`

## Purpose

`Vec2` is a deterministic 2D float vector primitive for geometry and math kernels.

## Invariants and Contracts

- Value type: `{x, y}` (`float`, no allocation).
- Default state: `(0, 0)`.
- Arithmetic is component-wise for `+`, `-`, scalar `*`/`/`.
- `Dot` and `LengthSquared` are deterministic pure operations.

## Usage Example

```cpp
#include <zcore/vec2.hpp>

const zcore::Vec2 v = zcore::Vec2(1.0F, 2.0F) + zcore::Vec2(3.0F, 4.0F);
```

## Warnings and Edge Cases

- Scalar division does not guard against zero divisors.
- Equality uses exact float comparison.

## Thread-Safety and Ownership Notes

- Pure value type; copy/share by value.

